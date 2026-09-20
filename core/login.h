#ifndef LOGIN_H
#define LOGIN_H

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "core_global.h"

class HttpClient;

class CORE_EXPORT Login : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    explicit Login(QObject *parent = nullptr);

    bool hasValidSession();

signals:
    void loginSucceeded();
    void loggedOut();
    void loginFailed();

private:
    HttpClient *m_http = nullptr;
    QString m_token;
};

#endif // LOGIN_H
