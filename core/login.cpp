#include "login.h"

#include "httpclient.h"

#include <QUrl>

Login::Login(QObject *parent)
    : QObject{parent}
    , m_http(new HttpClient(this))
{
    // 后端地址还没定，先用环境变量注入，不要写死真实地址。
    m_http->setBaseUrl(QUrl(qEnvironmentVariable(
        "CLEARTONE_API_BASE", QStringLiteral("http://127.0.0.1:8080/api/v1"))));
    m_http->setTimeout(15000);

    // 每次请求发出前取一次当前的 token，登录/登出后立刻生效。
    // context 传 this：Login 若先于 client 销毁，注入会被自动跳过。
    m_http->setHeaderProvider(this, [this] {
        QVariantMap headers;
        if (!m_token.isEmpty())
            headers.insert(QStringLiteral("Authorization"), QStringLiteral("Bearer ") + m_token);
        return headers;
    });
}

bool Login::hasValidSession()
{
    // 真实会话判断要等登录流程（凭据表单、token 落地）落地后再接
    return false;
}
