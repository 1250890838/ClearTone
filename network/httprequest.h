#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
#include <QMetaType>

#include "network_global.h"

class NETWORK_EXPORT HttpRequest
{
public:
    enum class Method { Get, Head, Post, Put, Patch, Delete };

    Method method = Method::Get;
    QString path;
    QVariantMap query;
    QVariantMap headers;
    QByteArray body;
    QByteArray contentType = QByteArrayLiteral("application/json");
    int timeoutMs = -1;
    int maxRetries = -1;

    bool retryable = false;

    QString uploadFilePath;
    QString uploadFieldName = QStringLiteral("file");
    QVariantMap uploadFields;

    QString saveToPath;

    static QByteArray methodName(Method method);
    static bool isIdempotent(Method method);

    void setJsonBody(const QJsonObject &object);
};

Q_DECLARE_METATYPE(HttpRequest)

#endif // HTTPREQUEST_H
