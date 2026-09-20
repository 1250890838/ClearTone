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
        Unreachable,        // 网络本身不可用：断开、DNS 之外的路由问题
        ConnectionRefused,  // 端口没人监听
        HostNotFound,       // DNS 解析失败
        Timeout,            // 长时间没有数据往来
        Ssl,                // 握手失败、证书不被信任
        Cancelled,          // 调用方主动 cancel()，或 client 先被销毁
        HttpStatus,         // 连上了，但状态码非 2xx
        Parse,              // 声明是 JSON，但解析失败
        FileWrite,          // 下载落盘失败
        InvalidRequest,     // 客户端用法错误：baseUrl 没设、路径非法
        Unknown,
    };

    Kind kind = Kind::None;
    int httpStatus = 0;         // kind == HttpStatus 时有意义
    QString message;            // 面向人的描述，优先取后端返回的 message 字段
    QString serverCode;         // 后端业务错误码，若有
    QJsonObject payload;        // 解析出的错误响应体，若有

    bool isValid() const { return kind != Kind::None; }
    QString toString() const;
};

Q_DECLARE_METATYPE(NetworkError)

#endif // NETWORKERROR_H
