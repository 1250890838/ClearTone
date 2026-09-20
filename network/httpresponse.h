#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMetaType>
#include <QPair>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "network_global.h"
#include "networkerror.h"

// 一次请求的完整结果。纯值类型，可以脱离 HttpReply 存活、可以拷贝、可以跨线程排队传递。
class NETWORK_EXPORT HttpResponse
{
public:
    int httpStatus = 0;
    QUrl requestUrl;                                // 实际请求的 URL（重定向后为最终地址）
    QList<QPair<QByteArray, QByteArray>> headers;   // 保留重复的头（比如多个 Set-Cookie）
    QByteArray rawBody;
    NetworkError error;
    qint64 elapsedMs = 0;

    // 没有错误且状态码 2xx。注意下载落盘失败时这里也会是 false。
    bool isSuccess() const;

    // 响应体是合法 JSON 才为真；空体（如 204）不算。
    bool hasValidJson() const;

    QJsonDocument json() const;     // 解析失败返回空 document
    QJsonObject object() const;     // 解析失败或不是对象时返回空对象
    QJsonArray array() const;
    QVariantMap map() const;
    QString text() const;           // 按 UTF-8 解码

    QByteArray header(const QByteArray &name) const;    // 大小写不敏感
    QString contentType() const;
};

Q_DECLARE_METATYPE(HttpResponse)

#endif // HTTPRESPONSE_H
