#include "login.h"

#include "httpclient.h"

#include <QUrl>

Login::Login(QObject *parent)
    : QObject{parent}
    , m_http(new HttpClient(this))
{
    m_http->setBaseUrl(QUrl(qEnvironmentVariable("CLEARTONE_API_BASE",
                                                 QStringLiteral("http://127.0.0.1:8080/api/v1"))));
    m_http->setTimeout(15000);

    m_http->setHeaderProvider(this, [this] {
        QVariantMap headers;
        if (!m_token.isEmpty())
            headers.insert(QStringLiteral("Authorization"), QStringLiteral("Bearer ") + m_token);
        return headers;
    });
}

bool Login::hasValidSession()
{
    return false;
}
