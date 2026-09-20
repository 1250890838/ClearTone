#ifndef HTTPREPLY_H
#define HTTPREPLY_H

#include <QObject>
#include <QUrl>

#include "httprequest.h"
#include "httpresponse.h"
#include "network_global.h"

class HttpClient;
class QNetworkAccessManager;

// 一次请求的句柄。由 HttpClient 的动词方法返回，调用方通过信号拿结果。
//
// 生命周期规则（唯一需要留意的点）：
//   发出 finished() 之后会自己 deleteLater()，所以「连完即忘」不会漏对象。
//   调用方拿到指针后必须在同一个事件循环轮次内 connect（同步返回，所以总是来得及），
//   不要把这个裸指针存起来跨回调用。确实需要持有就先 setAutoDelete(false) 并自行接管。
class NETWORK_EXPORT HttpReply : public QObject
{
    Q_OBJECT
public:
    ~HttpReply() override;

    bool isFinished() const;
    bool isRunning() const;             // 包含重试等待中

    void cancel();                      // 结果为 NetworkError::Kind::Cancelled

    QUrl url() const;
    qint64 bytesReceived() const;
    qint64 bytesTotal() const;          // -1 表示长度未知
    QString savedFilePath() const;      // 仅 download() 成功后有值
    int attemptCount() const;           // 已经发出的尝试次数（含重试）

    void setAutoDelete(bool on);
    bool autoDelete() const;

signals:
    void finished(const HttpResponse &response);    // 成功失败都发，且只发一次
    void succeeded(const HttpResponse &response);   // 仅 2xx 且无错
    void failed(const NetworkError &error);         // 非 2xx、传输失败、被取消
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    friend class HttpClient;
    HttpReply(const HttpRequest &request, HttpClient *client);

    void start();

    // 转发给 HttpClient 的私有实现；因为 friend 关系不传递给嵌套类，
    // 所以必须经由 HttpReply 自己的成员函数中转。
    QUrl buildUrlForRequest() const;
    QVariantMap mergedHeadersForRequest() const;
    int effectiveTimeout() const;
    int effectiveMaxAttempts() const;
    int clientBackoffMs() const;
    QNetworkAccessManager *networkAccessManagerForRequest() const;

    struct Private;
    Private *d = nullptr;
};

#endif // HTTPREPLY_H
