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

class NETWORK_EXPORT HttpResponse
{
public:
    int httpStatus = 0;
    QUrl requestUrl;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray rawBody;
    NetworkError error;
    qint64 elapsedMs = 0;

    bool isSuccess() const;

    bool hasValidJson() const;

    QJsonDocument json() const;
    QJsonObject object() const;
    QJsonArray array() const;
    QVariantMap map() const;
    QString text() const;

    QByteArray header(const QByteArray &name) const;
    QString contentType() const;
};

Q_DECLARE_METATYPE(HttpResponse)

#endif // HTTPRESPONSE_H
