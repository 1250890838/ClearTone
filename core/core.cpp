#include "core.h"

#include <QDebug>
Core::Core(QObject *parent)
    : QObject{parent}
{}

void Core::song()
{
    qDebug() << "hello,hello,my friend!";
}
