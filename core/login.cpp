#include "login.h"

#include <QDebug>
Login::Login(QObject *parent)
    : QObject{parent}
{}

bool Login::hasValidSession()
{
    return false;
}
