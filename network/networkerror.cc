#include "networkerror.h"

namespace {

QString kindName(NetworkError::Kind kind)
{
    switch (kind) {
    case NetworkError::Kind::None:              return QStringLiteral("无错误");
    case NetworkError::Kind::Unreachable:       return QStringLiteral("网络不可用");
    case NetworkError::Kind::ConnectionRefused: return QStringLiteral("连接被拒绝");
    case NetworkError::Kind::HostNotFound:      return QStringLiteral("域名解析失败");
    case NetworkError::Kind::Timeout:           return QStringLiteral("超时");
    case NetworkError::Kind::Ssl:               return QStringLiteral("SSL 错误");
    case NetworkError::Kind::Cancelled:         return QStringLiteral("已取消");
    case NetworkError::Kind::HttpStatus:        return QStringLiteral("HTTP 错误");
    case NetworkError::Kind::Parse:             return QStringLiteral("响应格式错误");
    case NetworkError::Kind::FileWrite:         return QStringLiteral("文件写入失败");
    case NetworkError::Kind::InvalidRequest:    return QStringLiteral("请求非法");
    case NetworkError::Kind::Unknown:           return QStringLiteral("未知错误");
    }
    return QStringLiteral("未知错误");
}

} // namespace

QString NetworkError::toString() const
{
    if (kind == Kind::None)
        return kindName(kind);

    QString text = kindName(kind);
    if (kind == Kind::HttpStatus && httpStatus > 0)
        text += QStringLiteral(" %1").arg(httpStatus);
    if (!message.isEmpty())
        text += QStringLiteral(": ") + message;
    return text;
}
