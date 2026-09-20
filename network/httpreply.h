#ifndef HTTPREPLY_H
#define HTTPREPLY_H

#include <QObject>
#include <QUrl>

#include "httprequest.h"
#include "httpresponse.h"
#include "network_global.h"

class HttpClient;
class QNetworkAccessManager;

class NETWORK_EXPORT HttpReply : public QObject
{
    Q_OBJECT
public:
    ~HttpReply() override;

    bool isFinished() const;
    bool isRunning() const;

    void cancel();

    QUrl url() const;
    qint64 bytesReceived() const;
    qint64 bytesTotal() const;
    QString savedFilePath() const;
    int attemptCount() const;

    void setAutoDelete(bool on);
    bool autoDelete() const;

signals:
    void finished(const HttpResponse &response);
    void succeeded(const HttpResponse &response);
    void failed(const NetworkError &error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    friend class HttpClient;
    HttpReply(const HttpRequest &request, HttpClient *client);

    void start();

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
