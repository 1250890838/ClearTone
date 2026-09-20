#include "httpresponse.h"

bool HttpResponse::isSuccess() const
{
    return !error.isValid() && httpStatus >= 200 && httpStatus < 300;
}

bool HttpResponse::hasValidJson() const
{
    return !rawBody.isEmpty() && !QJsonDocument::fromJson(rawBody).isNull();
}

QJsonDocument HttpResponse::json() const
{
    return QJsonDocument::fromJson(rawBody);
}

QJsonObject HttpResponse::object() const
{
    return json().object();
}

QJsonArray HttpResponse::array() const
{
    return json().array();
}

QVariantMap HttpResponse::map() const
{
    return object().toVariantMap();
}

QString HttpResponse::text() const
{
    return QString::fromUtf8(rawBody);
}

QByteArray HttpResponse::header(const QByteArray &name) const
{
    for (const auto &pair : headers) {
        if (pair.first.compare(name, Qt::CaseInsensitive) == 0)
            return pair.second;
    }
    return {};
}

QString HttpResponse::contentType() const
{
    return QString::fromLatin1(header(QByteArrayLiteral("Content-Type")));
}
