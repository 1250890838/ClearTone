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

    Rectangle {
        id: container
        anchors.fill: parent
        color: Theme.primaryBackground
        MouseArea {
            anchors.fill: parent
            onClicked: Login.loginSucceeded()
        }
    }
}
