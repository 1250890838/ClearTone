#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <QJsonObject>
#include <QMetaType>
#include <QString>

#include "network_global.h"

class NETWORK_EXPORT NetworkError
{
public:
    enum class Kind {
        None,
        Unreachable,
        ConnectionRefused,
        HostNotFound,
        Timeout,
        Ssl,
        Cancelled,
        HttpStatus,
        Parse,
        FileWrite,
        InvalidRequest,
        Unknown,
    };

    Kind kind = Kind::None;
    int httpStatus = 0;
    QString message;
    QString serverCode;
    QJsonObject payload;

    bool isValid() const { return kind != Kind::None; }
    QString toString() const;
};

Q_DECLARE_METATYPE(NetworkError)

#endif
