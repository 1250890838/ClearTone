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
    // 必须先把还活着的 reply 删掉。QObject 的子对象是按添加顺序析构的，
    // 而 QNAM 直到第一次请求才被创建；不在这里处理的话，基类析构会先删掉 QNAM，
    // 之后才析构的 HttpReply 就会碰到已经不存在的 QNetworkReply。
    const QList<HttpReply *> replies = findChildren<HttpReply *>();
    for (HttpReply *reply : replies)
        delete reply;

    delete d;
    d = nullptr;
}

// ——— 配置 ————————————————————————————————————————————————

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

// ——— 内部 ————————————————————————————————————————————————

QUrl HttpClient::buildUrl(const HttpRequest &request) const
{
    const QString path = request.path;
    const bool absolute = path.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
                       || path.startsWith(QLatin1String("https://"), Qt::CaseInsensitive);

    QUrl url;
    if (absolute) {
        url = QUrl(path);
    } else {
        // 不能用 QUrl::resolved()：base 是 https://host/v1 时，resolved("/users")
        // 会得到 https://host/users，把 /v1 悄悄吃掉。这里手工拼路径。
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
        QUrlQuery query(url.query());   // 保留 baseUrl 自带的查询参数
        for (auto it = request.query.constBegin(); it != request.query.constEnd(); ++it)
            query.addQueryItem(it.key(), it.value().toString());
        url.setQuery(query);
    }

    return url;
}

QVariantMap HttpClient::mergedHeaders(const HttpRequest &request) const
{
    QVariantMap merged = d->defaultHeaders;

    // 注入点：每次请求前现算，token 刷新后立刻生效。
    // context 已被销毁就跳过，避免调用方 lambda 里捕获的 this 悬垂。
    const bool providerUsable = d->headerProvider
                                && (!d->providerHasContext || !d->providerContext.isNull());
    if (providerUsable) {
        const QVariantMap injected = d->headerProvider();
        for (auto it = injected.constBegin(); it != injected.constEnd(); ++it)
            merged.insert(it.key(), it.value());
    }

    // 单次请求显式指定的头优先级最高
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
        // 不装自定义 cookie jar：QNAM 默认那个已经会收 Set-Cookie 并在后续请求带上。
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

// ——— 请求 ————————————————————————————————————————————————

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
    request.retryable = true;   // GET 幂等，重试安全
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
    request.retryable = false;   // POST 默认不重试，避免重复提交
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
    request.retryable = true;    // PUT 幂等
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
    request.retryable = true;    // DELETE 幂等
    return send(request);
}

// ——— 文件传输 ————————————————————————————————————————————————

HttpReply *HttpClient::upload(const QString &path, const QString &localFilePath,
                              const QString &fieldName, const QVariantMap &extraFields)
{
    HttpRequest request;
    request.method = HttpRequest::Method::Post;
    request.path = path;
    request.uploadFilePath = localFilePath;
    request.uploadFieldName = fieldName.isEmpty() ? QStringLiteral("file") : fieldName;
    request.uploadFields = extraFields;
    request.retryable = false;   // multipart 里的 QFile 是一次性消费的，不能重放
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
    request.retryable = true;    // 重试会重开 .part 文件，从头写
    return send(request);
}
