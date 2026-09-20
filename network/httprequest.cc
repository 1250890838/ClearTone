#include "httprequest.h"

QByteArray HttpRequest::methodName(Method method)
{
    switch (method) {
    case Method::Get:    return QByteArrayLiteral("GET");
    case Method::Head:   return QByteArrayLiteral("HEAD");
    case Method::Post:   return QByteArrayLiteral("POST");
    case Method::Put:    return QByteArrayLiteral("PUT");
    case Method::Patch:  return QByteArrayLiteral("PATCH");
    case Method::Delete: return QByteArrayLiteral("DELETE");
    }
    return QByteArrayLiteral("GET");
}

bool HttpRequest::isIdempotent(Method method)
{
    switch (method) {
    case Method::Get:
    case Method::Head:
    case Method::Put:
    case Method::Delete:
        return true;
    case Method::Post:
    case Method::Patch:
        return false;
    }
    return false;
}

void HttpRequest::setJsonBody(const QJsonObject &object)
{
    body = QJsonDocument(object).toJson(QJsonDocument::Compact);
    contentType = QByteArrayLiteral("application/json");
}
