#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include "login.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    auto *login = engine.singletonInstance<Login *>("core", "Login");

    QQuickWindow *loginWindow = nullptr;
    QQuickWindow *mainWindow = nullptr;

    auto openLogin = [&] {
        if (!loginWindow) {
            engine.loadFromModule("ClearTone", "LoginWindow");
            loginWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().last());
        }
        loginWindow->setVisible(true);
        if (mainWindow) {
            mainWindow->deleteLater();
            mainWindow = nullptr;
        }
    };

    auto openMain = [&] {
        if (!mainWindow) {
            engine.loadFromModule("ClearTone", "Main");
            mainWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().last());
        }
        mainWindow->setVisible(true);
        if (loginWindow) {
            loginWindow->deleteLater();
            loginWindow = nullptr;
        }
    };

    QObject::connect(login, &Login::loginSucceeded, &engine, openMain);
    QObject::connect(login, &Login::loggedOut, &engine, openLogin);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    if (login->hasValidSession())
        openMain();
    else
        openLogin();
    return QGuiApplication::exec();
}
