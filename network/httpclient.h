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

// HTTP 基础模块的唯一入口。只依赖 Qt，不含任何业务概念。
//
// 一个 HttpClient 对应一个 QNetworkAccessManager，因此：
//   - 必须在同一个线程里创建和使用（QNAM 有线程亲和性）；
//   - 会话 cookie 随它存活，同名 QNAM 复用连接池。
class NETWORK_EXPORT HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    // —— 配置 ——
    void setBaseUrl(const QUrl &baseUrl);
    QUrl baseUrl() const;

    void setDefaultHeaders(const QVariantMap &headers);
    QVariantMap defaultHeaders() const;

    void setTimeout(int ms);                    // 无数据往来的超时，默认 15000；<=0 表示不限
    int timeoutMs() const;

    void setRetries(int count, int backoffMs = 300);   // 默认 0（不重试）
    int retries() const;

    // —— 请求头注入点 ——
    // 每次请求发出前调用一次（不是存快照，所以 token 刷新后立刻生效）。
    // context 以 QPointer 持有：context 先被销毁后自动跳过注入，
    // 避免 lambda 里捕获的 this 悬垂。
    using HeaderProvider = std::function<QVariantMap()>;
    void setHeaderProvider(QObject *context, HeaderProvider provider);
    void clearHeaderProvider();

    // 合并优先级：defaultHeaders < 注入点 < 单次请求的 headers
    // —— 动词方法（按幂等性自动决定是否允许重试）
    HttpReply *send(const HttpRequest &request);

    HttpReply *get(const QString &path, const QVariantMap &query = {});
    HttpReply *post(const QString &path);
    HttpReply *post(const QString &path, const QJsonObject &body);
    HttpReply *post(const QString &path, const QByteArray &raw,
                    const QByteArray &contentType = QByteArrayLiteral("application/octet-stream"));
    HttpReply *put(const QString &path, const QJsonObject &body);
    HttpReply *patch(const QString &path, const QJsonObject &body);
    HttpReply *del(const QString &path, const QVariantMap &query = {});

    // —— 文件传输 ——
    HttpReply *upload(const QString &path, const QString &localFilePath,
                      const QString &fieldName = QStringLiteral("file"),
                      const QVariantMap &extraFields = {});
    HttpReply *download(const QString &path, const QString &saveToPath,
                        const QVariantMap &query = {});

    // —— 逃生舱口 ——
    QNetworkAccessManager *networkAccessManager() const;
    // Qt 自带的 cookie jar（服务端 Set-Cookie 由它自动收发）。
    // 退出登录时可以用 cookieJar()->setAllCookies({}) 清掉会话。
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
