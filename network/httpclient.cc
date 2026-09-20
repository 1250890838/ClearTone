#include "httpclient.h"

#include "httpreply.h"
#include "httprequest.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

struct HttpClient::Private
{
    QUrl baseUrl;
    QVariantMap defaultHeaders;
    int timeoutMs = 15000;
    int retries = 0;
    int backoffMs = 300;

    QPointer<QObject> providerContext;
    bool providerHasContext = false;
    HttpClient::HeaderProvider headerProvider;

    QNetworkAccessManager *nam = nullptr;
};

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

HttpClient::~HttpClient()
{
    const QList<HttpReply *> replies = findChildren<HttpReply *>();
    for (HttpReply *reply : replies)
        delete reply;

    delete d;
    d = nullptr;
}

void HttpClient::setBaseUrl(const QUrl &baseUrl)
{
    d->baseUrl = baseUrl;
}

QUrl HttpClient::baseUrl() const
{
    return d->baseUrl;
}

void HttpClient::setDefaultHeaders(const QVariantMap &headers)
{
    d->defaultHeaders = headers;
}

QVariantMap HttpClient::defaultHeaders() const
{
    return d->defaultHeaders;
}

void HttpClient::setTimeout(int ms)
{
    d->timeoutMs = ms;
}

int HttpClient::timeoutMs() const
{
    return d->timeoutMs;
}

void HttpClient::setRetries(int count, int backoffMs)
{
    d->retries = qMax(0, count);
    d->backoffMs = qMax(1, backoffMs);
}

int HttpClient::retries() const
{
    return d->retries;
}

void HttpClient::setHeaderProvider(QObject *context, HeaderProvider provider)
{
    d->headerProvider = std::move(provider);
    d->providerContext = context;
    d->providerHasContext = (context != nullptr);
}

void HttpClient::clearHeaderProvider()
{
    d->headerProvider = nullptr;
    d->providerContext = nullptr;
    d->providerHasContext = false;
}

QUrl HttpClient::buildUrl(const HttpRequest &request) const
{
    const QString path = request.path;
    const bool absolute = path.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
                       || path.startsWith(QLatin1String("https://"), Qt::CaseInsensitive);

    QUrl url;
    if (absolute) {
        url = QUrl(path);
    } else {
        url = d->baseUrl;

        QString basePath = url.path();
        while (basePath.endsWith(QLatin1Char('/')))
            basePath.chop(1);

        QString relative = path;
        while (relative.startsWith(QLatin1Char('/')))
            relative.remove(0, 1);

        if (!relative.isEmpty())
            url.setPath(basePath + QLatin1Char('/') + relative);
        else if (!basePath.isEmpty())
            url.setPath(basePath);
    }

    if (!request.query.isEmpty()) {
        QUrlQuery query(url.query());
        for (auto it = request.query.constBegin(); it != request.query.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value().toString());
        url.setQuery(query);
    }

    return url;
}

QVariantMap HttpClient::mergedHeaders(const HttpRequest &request) const
{
    QVariantMap merged = d->defaultHeaders;

    const bool providerUsable = d->headerProvider
                                && (!d->providerHasContext || !d->providerContext.isNull());
    if (providerUsable) {
        const QVariantMap injected = d->headerProvider();
        for (auto it = injected.constBegin(); it != injected.constEnd(); ++it)
            merged.insert(it.key(), it.value());
    }

    for (auto it = request.headers.constBegin(); it != request.headers.constEnd(); ++it)
        merged.insert(it.key(), it.value());

    return merged;
}

int HttpClient::effectiveTimeoutMs(const HttpRequest &request) const
{
    return request.timeoutMs > 0 ? request.timeoutMs : d->timeoutMs;
}

int HttpClient::effectiveMaxAttempts(const HttpRequest &request) const
{
    const int retries = request.maxRetries >= 0 ? request.maxRetries : d->retries;
    return qMax(1, retries + 1);
}

int HttpClient::backoffMs() const
{
    return d->backoffMs;
}

QNetworkAccessManager *HttpClient::ensureNetworkAccessManager()
{
    if (!d->nam) {
        d->nam = new QNetworkAccessManager(this);
    }
    return d->nam;
}

QNetworkAccessManager *HttpClient::networkAccessManager() const
{
    return d->nam;
}

QNetworkCookieJar *HttpClient::cookieJar() const
{
    return d->nam ? d->nam->cookieJar() : nullptr;
}

HttpReply *HttpClient::send(const HttpRequest &request)
{
    Q_ASSERT(thread() == QThread::currentThread());
    return new HttpReply(request, this);
}

HttpReply *HttpClient::get(const QString &path, const QVariantMap &query)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Get;
    request.path = path;
    request.query = query;
    request.retryable = true;
    return send(request);
}

HttpReply *HttpClient::post(const QString &path)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Post;
    request.path = path;
    request.retryable = false;
    return send(request);
}

HttpReply *HttpClient::post(const QString &path, const QJsonObject &body)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Post;
    request.path = path;
    request.setJsonBody(body);
    request.retryable = false;
    return send(request);
}

HttpReply *HttpClient::post(const QString &path, const QByteArray &raw, const QByteArray &contentType)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Post;
    request.path = path;
    request.body = raw;
    request.contentType = contentType;
    request.retryable = false;
    return send(request);
}

HttpReply *HttpClient::put(const QString &path, const QJsonObject &body)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Put;
    request.path = path;
    request.setJsonBody(body);
    request.retryable = true;
    return send(request);
}

HttpReply *HttpClient::patch(const QString &path, const QJsonObject &body)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Patch;
    request.path = path;
    request.setJsonBody(body);
    request.retryable = false;
    return send(request);
}

HttpReply *HttpClient::del(const QString &path, const QVariantMap &query)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Delete;
    request.path = path;
    request.query = query;
    request.retryable = true;
    return send(request);
}


HttpReply *HttpClient::upload(const QString &path, const QString &localFilePath,
                              const QString &fieldName, const QVariantMap &extraFields)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Post;
    request.path = path;
    request.uploadFilePath = localFilePath;
    request.uploadFieldName = fieldName.isEmpty() ? QStringLiteral("file") : fieldName;
    request.uploadFields = extraFields;
    request.retryable = false;
    return send(request);
}

HttpReply *HttpClient::download(const QString &path, const QString &saveToPath,
                                const QVariantMap &query)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Get;
    request.path = path;
    request.query = query;
    request.saveToPath = saveToPath;
    request.retryable = true;
    return send(request);
}
