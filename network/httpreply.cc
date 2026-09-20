#include "httpreply.h"

#include "httpclient.h"
#include "networkerror.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QTimer>

#include <chrono>

namespace {

constexpr int kMaxBackoffMs = 8000;

NetworkError::Kind transportKind(QNetworkReply::NetworkError code)
{
    switch (code) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::ProxyConnectionRefusedError:
        return NetworkError::Kind::ConnectionRefused;
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::ProxyNotFoundError:
        return NetworkError::Kind::HostNotFound;
    case QNetworkReply::TimeoutError:
    case QNetworkReply::ProxyTimeoutError:
        return NetworkError::Kind::Timeout;
    case QNetworkReply::SslHandshakeFailedError:
        return NetworkError::Kind::Ssl;
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::UnknownNetworkError:
        return NetworkError::Kind::Unreachable;
    default:
        return NetworkError::Kind::Unknown;
    }
}

// 只有这些算「换个时机可能就好了」。SSL 握手失败重试基本没用，不列入。
bool isRetriableKind(NetworkError::Kind kind)
{
    switch (kind) {
    case NetworkError::Kind::Unreachable:
    case NetworkError::Kind::ConnectionRefused:
    case NetworkError::Kind::HostNotFound:
    case NetworkError::Kind::Timeout:
        return true;
    default:
        return false;
    }
}

// 后端错误体没有统一标准，这里兼容最常见的几种：
//   {"message": "...", "code": "..."} / {"error": {"message": ..., "code": ...}} / {"error": "..."}
void fillFromErrorBody(NetworkError &error, const QByteArray &body)
{
    if (body.isEmpty())
        return;

    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject())
        return;

    const QJsonObject object = document.object();
    error.payload = object;

    const QJsonValue errorValue = object.value(QStringLiteral("error"));
    const QJsonObject nested = errorValue.toObject();

    const QString message = object.value(QStringLiteral("message")).toString();
    if (!message.isEmpty())
        error.message = message;
    else if (errorValue.isString())
        error.message = errorValue.toString();
    else if (!nested.value(QStringLiteral("message")).toString().isEmpty())
        error.message = nested.value(QStringLiteral("message")).toString();

    const QJsonValue code = object.value(QStringLiteral("code"));
    if (code.isString())
        error.serverCode = code.toString();
    else if (code.isDouble())
        error.serverCode = QString::number(code.toInt());
    else if (nested.value(QStringLiteral("code")).isString())
        error.serverCode = nested.value(QStringLiteral("code")).toString();
}

} // namespace

struct HttpReply::Private
{
    HttpReply *q = nullptr;
    HttpRequest request;
    HttpClient *client = nullptr;

    QPointer<QNetworkReply> networkReply;
    QTimer *inactivityTimer = nullptr;
    QTimer *retryTimer = nullptr;

    HttpResponse response;
    QElapsedTimer elapsed;
    int attempt = 0;
    int maxAttempts = 1;
    int backoffMs = 300;
    int timeoutMs = 0;

    bool finished = false;
    bool autoDelete = true;
    bool aborted = false;
    NetworkError::Kind abortReason = NetworkError::Kind::Cancelled;

    QSaveFile *saveFile = nullptr;
    QString savedPath;

    void startAttempt();
    void onNetworkReplyFinished();
    void drainToSaveFile();
    void restartInactivityTimer();
    bool shouldRetry() const;
    void scheduleRetry();
    void finish();
};


HttpReply::HttpReply(const HttpRequest &request, HttpClient *client)
    : QObject(client)
    , d(new Private)
{
    d->q = this;
    d->request = request;
    d->client = client;

    QTimer::singleShot(0, this, [this] { start(); });
}

HttpReply::~HttpReply()
{
    if (d->networkReply) {
        disconnect(d->networkReply, nullptr, this, nullptr);
        if (d->networkReply->isRunning())
            d->networkReply->abort();
        d->networkReply->deleteLater();
    }

    if (d->saveFile) {
        d->saveFile->cancelWriting();
        delete d->saveFile;
    }

    delete d;
    d = nullptr;
}

void HttpReply::start()
{
    if (d->finished)
        return;

    d->maxAttempts = effectiveMaxAttempts();
    d->backoffMs = clientBackoffMs();
    d->timeoutMs = effectiveTimeout();

    const QString path = d->request.path;
    const bool absolute = path.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
                       || path.startsWith(QLatin1String("https://"), Qt::CaseInsensitive);

    if (!absolute && !d->client->baseUrl().isValid()) {
        d->response.error.kind = NetworkError::Kind::InvalidRequest;
        d->response.error.message =
            QStringLiteral("HttpClient::setBaseUrl() 未设置，无法解析相对路径 \"%1\"").arg(path);
        d->finish();
        return;
    }

    d->startAttempt();
}

bool HttpReply::isFinished() const
{
    return d->finished;
}

bool HttpReply::isRunning() const
{
    if (d->finished)
        return false;
    return d->networkReply != nullptr || (d->retryTimer && d->retryTimer->isActive());
}

void HttpReply::cancel()
{
    if (d->finished)
        return;

    d->aborted = true;
    d->abortReason = NetworkError::Kind::Cancelled;

    if (d->retryTimer)
        d->retryTimer->stop();

    if (d->networkReply) {
        disconnect(d->networkReply, nullptr, this, nullptr);
        d->networkReply->abort();
        d->networkReply->deleteLater();
        d->networkReply.clear();
    }

    // 上面已经断开了连接，onNetworkReplyFinished 不会再跑，错误映射得在这里补上，
    // 否则调用方拿到的是一个「没错误、没状态码」的空响应。
    d->response.error.kind = NetworkError::Kind::Cancelled;
    d->response.error.message = QStringLiteral("request cancelled by caller");

    d->finish();
}

QUrl HttpReply::url() const
{
    return d->response.requestUrl;
}

qint64 HttpReply::bytesReceived() const
{
    return d->networkReply ? d->networkReply->bytesAvailable() : 0;
}

qint64 HttpReply::bytesTotal() const
{
    return d->networkReply
        ? d->networkReply->header(QNetworkRequest::ContentLengthHeader).toLongLong()
        : -1;
}

QString HttpReply::savedFilePath() const
{
    return d->savedPath;
}

int HttpReply::attemptCount() const
{
    return d->attempt;
}

void HttpReply::setAutoDelete(bool on)
{
    d->autoDelete = on;
}

bool HttpReply::autoDelete() const
{
    return d->autoDelete;
}

// ——— 转发到 HttpClient 的私有实现 ————————————————————————————————

QUrl HttpReply::buildUrlForRequest() const
{
    return d->client->buildUrl(d->request);
}

QVariantMap HttpReply::mergedHeadersForRequest() const
{
    return d->client->mergedHeaders(d->request);
}

int HttpReply::effectiveTimeout() const
{
    return d->client->effectiveTimeoutMs(d->request);
}

int HttpReply::effectiveMaxAttempts() const
{
    return d->client->effectiveMaxAttempts(d->request);
}

int HttpReply::clientBackoffMs() const
{
    return d->client->backoffMs();
}

QNetworkAccessManager *HttpReply::networkAccessManagerForRequest() const
{
    return d->client->ensureNetworkAccessManager();
}

// ——— Private ————————————————————————————————————————————————

void HttpReply::Private::restartInactivityTimer()
{
    if (timeoutMs <= 0) {
        if (inactivityTimer)
            inactivityTimer->stop();
        return;
    }

    if (!inactivityTimer) {
        inactivityTimer = new QTimer(q);
        inactivityTimer->setSingleShot(true);
        connect(inactivityTimer, &QTimer::timeout, q, [this] {
            aborted = true;
            abortReason = NetworkError::Kind::Timeout;
            if (networkReply)
                networkReply->abort();   // 同步触发 finished()，交给 onNetworkReplyFinished
            else
                finish();
        });
    }
    inactivityTimer->start(timeoutMs);
}

void HttpReply::Private::drainToSaveFile()
{
    if (!saveFile || !networkReply)
        return;

    const QByteArray chunk = networkReply->readAll();
    if (chunk.isEmpty())
        return;

    restartInactivityTimer();

    if (saveFile->write(chunk) != chunk.size()) {
        aborted = true;
        abortReason = NetworkError::Kind::FileWrite;
        networkReply->abort();
    }
}

void HttpReply::Private::startAttempt()
{
    ++attempt;
    aborted = false;
    response = HttpResponse{};
    elapsed.start();

    delete saveFile;
    saveFile = nullptr;
    savedPath.clear();

    if (!request.saveToPath.isEmpty()) {
        saveFile = new QSaveFile(request.saveToPath);
        if (!saveFile->open(QIODevice::WriteOnly)) {
            const QString reason = saveFile->errorString();
            delete saveFile;
            saveFile = nullptr;
            response.error.kind = NetworkError::Kind::FileWrite;
            response.error.message = reason;
            finish();
            return;
        }
    }

    QNetworkAccessManager *manager = q->networkAccessManagerForRequest();
    if (!manager) {
        response.error.kind = NetworkError::Kind::InvalidRequest;
        response.error.message = QStringLiteral("network access manager unavailable");
        finish();
        return;
    }

    const QUrl url = q->buildUrlForRequest();
    response.requestUrl = url;

    QNetworkRequest networkRequest(url);
    if (timeoutMs > 0)
        networkRequest.setTransferTimeout(std::chrono::milliseconds(timeoutMs));

    const QVariantMap headers = q->mergedHeadersForRequest();
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        if (it.key().compare(QLatin1String("Content-Type"), Qt::CaseInsensitive) == 0)
            continue;
        networkRequest.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }

    QNetworkReply *reply = nullptr;

    if (!request.uploadFilePath.isEmpty()) {
        auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        for (auto it = request.uploadFields.constBegin(); it != request.uploadFields.constEnd(); ++it) {
            QHttpPart part;
            part.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QStringLiteral("form-data; name=\"%1\"").arg(it.key()));
            part.setBody(it.value().toString().toUtf8());
            multiPart->append(part);
        }

        auto *file = new QFile(request.uploadFilePath, multiPart);   // 由 multiPart 负责释放
        if (!file->open(QIODevice::ReadOnly)) {
            const QString reason = file->errorString();
            delete multiPart;
            response.error.kind = NetworkError::Kind::InvalidRequest;
            response.error.message = QStringLiteral("无法打开上传文件 %1: %2")
                                         .arg(request.uploadFilePath, reason);
            finish();
            return;
        }

        const QString fieldName = request.uploadFieldName.isEmpty()
                                      ? QStringLiteral("file")
                                      : request.uploadFieldName;

        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                               .arg(fieldName, QFileInfo(request.uploadFilePath).fileName()));
        filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                           QMimeDatabase().mimeTypeForFile(request.uploadFilePath).name());
        filePart.setBodyDevice(file);
        multiPart->append(filePart);

        reply = manager->post(networkRequest, multiPart);
        multiPart->setParent(reply);
    } else {
        switch (request.method) {
        case HttpRequest::Method::Get:
            reply = manager->get(networkRequest);
            break;
        case HttpRequest::Method::Head:
            reply = manager->head(networkRequest);
            break;
        case HttpRequest::Method::Post:
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, request.contentType);
            reply = manager->post(networkRequest, request.body);
            break;
        case HttpRequest::Method::Put:
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, request.contentType);
            reply = manager->put(networkRequest, request.body);
            break;
        case HttpRequest::Method::Patch:
            networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, request.contentType);
            reply = manager->sendCustomRequest(networkRequest, QByteArrayLiteral("PATCH"), request.body);
            break;
        case HttpRequest::Method::Delete:
            // QNAM 没有带 body 的 deleteResource 重载，有 body 时只能走 sendCustomRequest
            if (request.body.isEmpty()) {
                reply = manager->deleteResource(networkRequest);
            } else {
                networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, request.contentType);
                reply = manager->sendCustomRequest(networkRequest, QByteArrayLiteral("DELETE"), request.body);
            }
            break;
        }
    }

    if (!reply) {
        response.error.kind = NetworkError::Kind::InvalidRequest;
        response.error.message = QStringLiteral("failed to dispatch request");
        finish();
        return;
    }

    networkReply = reply;

    connect(reply, &QNetworkReply::finished, q, [this] { onNetworkReplyFinished(); });
    connect(reply, &QNetworkReply::readyRead, q, [this] { drainToSaveFile(); });
    connect(reply, &QNetworkReply::downloadProgress, q, [this](qint64 received, qint64 total) {
        restartInactivityTimer();
        emit q->downloadProgress(received, total);
    });
    connect(reply, &QNetworkReply::uploadProgress, q, [this](qint64 sent, qint64 total) {
        restartInactivityTimer();
        emit q->uploadProgress(sent, total);
    });

    restartInactivityTimer();
}

void HttpReply::Private::onNetworkReplyFinished()
{
    if (finished)
        return;

    if (inactivityTimer)
        inactivityTimer->stop();

    QNetworkReply *reply = networkReply;
    if (!reply) {
        finish();
        return;
    }

    drainToSaveFile();

    response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.headers = reply->rawHeaderPairs();

    if (!saveFile)
        response.rawBody = reply->readAll();

    const QNetworkReply::NetworkError nativeError = reply->error();
    const QString nativeMessage = reply->errorString();

    if (aborted) {
        response.error.kind = abortReason;
        response.error.message = nativeMessage;
    } else if (nativeError == QNetworkReply::NoError) {
        response.error.kind = NetworkError::Kind::None;
    } else if (nativeError == QNetworkReply::OperationCanceledError) {
        response.error.kind = NetworkError::Kind::Cancelled;
        response.error.message = nativeMessage;
    } else if (response.httpStatus >= 400) {
        response.error.kind = NetworkError::Kind::HttpStatus;
        response.error.httpStatus = response.httpStatus;
        response.error.message = nativeMessage;
    } else {
        response.error.kind = transportKind(nativeError);
        response.error.message = nativeMessage;
    }

    if (response.error.isValid()) {
        fillFromErrorBody(response.error, response.rawBody);
    } else if (!response.rawBody.isEmpty()
               && response.contentType().contains(QLatin1String("json"), Qt::CaseInsensitive)
               && !response.hasValidJson()) {
        response.error.kind = NetworkError::Kind::Parse;
        response.error.message = QStringLiteral("响应体声明是 JSON 但解析失败");
    }

    disconnect(reply, nullptr, q, nullptr);
    reply->deleteLater();
    networkReply.clear();

    if (shouldRetry()) {
        scheduleRetry();
        return;
    }

    finish();
}

bool HttpReply::Private::shouldRetry() const
{
    if (aborted || finished)
        return false;
    if (attempt >= maxAttempts)
        return false;
    if (!request.retryable)
        return false;

    const NetworkError::Kind kind = response.error.kind;
    if (isRetriableKind(kind))
        return true;
    if (kind == NetworkError::Kind::HttpStatus)
        return response.httpStatus == 429 || response.httpStatus >= 500;

    return false;
}

void HttpReply::Private::scheduleRetry()
{
    int delay = backoffMs;
    for (int i = 1; i < attempt && delay < kMaxBackoffMs; ++i)
        delay *= 2;
    delay = qMin(delay, kMaxBackoffMs);
    delay += QRandomGenerator::global()->bounded(qMax(1, delay / 4));

    if (!retryTimer) {
        retryTimer = new QTimer(q);
        retryTimer->setSingleShot(true);
        connect(retryTimer, &QTimer::timeout, q, [this] { startAttempt(); });
    }
    retryTimer->start(delay);
}

void HttpReply::Private::finish()
{
    if (finished)
        return;
    finished = true;

    if (inactivityTimer)
        inactivityTimer->stop();
    if (retryTimer)
        retryTimer->stop();

    if (saveFile) {
        if (response.isSuccess()) {
            if (saveFile->commit()) {
                savedPath = saveFile->fileName();
            } else {
                const QString reason = saveFile->errorString();
                saveFile->cancelWriting();
                response.error.kind = NetworkError::Kind::FileWrite;
                response.error.message = reason;
            }
        } else {
            saveFile->cancelWriting();
        }
        delete saveFile;
        saveFile = nullptr;
    }

    if (elapsed.isValid())
        response.elapsedMs = elapsed.elapsed();

    QPointer<HttpReply> guard(q);

    if (response.isSuccess())
        emit q->succeeded(response);
    else
        emit q->failed(response.error);

    if (guard.isNull())
        return;

    emit q->finished(response);

    if (guard.isNull())
        return;

    if (autoDelete)
        q->deleteLater();
}
