pragma Singleton

import QtQuick

QtObject {
    readonly property color primaryBackground: "#FFFFFF"
    readonly property color secondaryBackground: "#F6F7F9"

    readonly property real frameRadius: Qt.platform.os === "windows" ? 0 : 8
}
