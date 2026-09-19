#ifndef LOGIN_H
#define LOGIN_H

#include <QObject>
#include <QQmlEngine>

#include "core_global.h"

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
};

#endif // LOGIN_H
