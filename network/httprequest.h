#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
#include <QMetaType>

#include "network_global.h"

// 单次请求的描述符。HttpClient 的各动词方法内部构造它，
// 需要自定义头或非 JSON 请求体时可以直接填好再交给 HttpClient::send()。
class NETWORK_EXPORT HttpRequest
{
public:
    enum class Method { Get, Head, Post, Put, Patch, Delete };

    Method method = Method::Get;
    QString path;                       // 相对 baseUrl；以 http:// 或 https:// 开头则跳过 baseUrl
    QVariantMap query;                  // 拼到 URL 上
    QVariantMap headers;                // 本次独有，优先级最高
    QByteArray body;
    QByteArray contentType = QByteArrayLiteral("application/json");
    int timeoutMs = -1;                 // -1 表示用 client 的默认值
    int maxRetries = -1;                // -1 表示用 client 的默认值

    // 是否允许自动重试。动词方法按幂等性自动设置：GET/HEAD/PUT/DELETE 为 true，
    // POST/PATCH 为 false（避免重复提交）。手工构造时默认不重试。
    bool retryable = false;

    // 非空则本次是 multipart 上传，body/contentType 被忽略。
    QString uploadFilePath;
    QString uploadFieldName = QStringLiteral("file");
    QVariantMap uploadFields;           // 随文件一起提交的文本字段

    // 非空则把响应体流式写入该文件（先写 .part，成功后再改名）。
    QString saveToPath;

    static QByteArray methodName(Method method);
    static bool isIdempotent(Method method);

    void setJsonBody(const QJsonObject &object);
};

Q_DECLARE_METATYPE(HttpRequest)

#endif // HTTPREQUEST_H
