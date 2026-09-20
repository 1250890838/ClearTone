#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariantMap>

#include <functional>

#include "httprequest.h"
#include "network_global.h"

class HttpReply;
class QNetworkAccessManager;
class QNetworkCookieJar;

class NETWORK_EXPORT HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void setBaseUrl(const QUrl &baseUrl);
    QUrl baseUrl() const;

    void setDefaultHeaders(const QVariantMap &headers);
    QVariantMap defaultHeaders() const;

    void setTimeout(int ms);
    int timeoutMs() const;

    void setRetries(int count, int backoffMs = 300);
    int retries() const;

    using HeaderProvider = std::function<QVariantMap()>;
    void setHeaderProvider(QObject *context, HeaderProvider provider);
    void clearHeaderProvider();

    HttpReply *send(const HttpRequest &request);

    HttpReply *get(const QString &path, const QVariantMap &query = {});
    HttpReply *post(const QString &path);
    HttpReply *post(const QString &path, const QJsonObject &body);
    HttpReply *post(const QString &path, const QByteArray &raw,
                    const QByteArray &contentType = QByteArrayLiteral("application/octet-stream"));
    HttpReply *put(const QString &path, const QJsonObject &body);
    HttpReply *patch(const QString &path, const QJsonObject &body);
    HttpReply *del(const QString &path, const QVariantMap &query = {});

    HttpReply *upload(const QString &path,
                      const QString &localFilePath,
                      const QString &fieldName = QStringLiteral("file"),
                      const QVariantMap &extraFields = {});
    HttpReply *download(const QString &path, const QString &saveToPath,
                        const QVariantMap &query = {});

    QNetworkAccessManager *networkAccessManager() const;

    QNetworkCookieJar *cookieJar() const;

private:
    friend class HttpReply;

    QUrl buildUrl(const HttpRequest &request) const;
    QVariantMap mergedHeaders(const HttpRequest &request) const;
    int effectiveTimeoutMs(const HttpRequest &request) const;
    int effectiveMaxAttempts(const HttpRequest &request) const;
    int backoffMs() const;
    QNetworkAccessManager *ensureNetworkAccessManager();

    struct Private;
    Private *d = nullptr;
};

#endif // HTTPCLIENT_H
