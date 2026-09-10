#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QQmlEngine>

class Core : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Test)
public:
    explicit Core(QObject *parent = nullptr);

    Q_INVOKABLE void song();
signals:
};

#endif // CORE_H
