pragma Singleton

import QtQuick

QtObject {
    readonly property color primaryBackground: "#FFFFFF"
    readonly property color secondaryBackground: "#F6F7F9"

    // Linux 无 DWM 圆角，由 QML 自绘；Windows 交给 DWM，保持 0
    readonly property real frameRadius: Qt.platform.os === "windows" ? 0 : 8
}
