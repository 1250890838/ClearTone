import QtQuick

import core

FramelessWindow {
    id: root
    visible: true
    width: 420
    height: 668
    systemMenuEnabled: false
    snapLayoutsEnabled: false
    resizeEnabled: false
    color: Theme.frameRadius > 0 ? "transparent" : "white"

    Rectangle {
        id: container
        anchors.fill: parent
        radius: Theme.frameRadius
        color: Theme.primaryBackground
        MouseArea {
            anchors.fill: parent
            onClicked: Login.loginSucceeded()
        }
    }
}
