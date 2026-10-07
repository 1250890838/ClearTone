import QtQuick
import core

// @disable-check M300
FramelessWindow {
    id: window
    width: 1000
    height: 680
    minimumWidth: 480
    minimumHeight: 320
    visible: true
    title: qsTr("ClearTone")
    color: Theme.frameRadius > 0 && !window.maximized ? "transparent" : "#0B0C10"

    TitleBar {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: window.title
        cornerRadius: !window.maximized ? Theme.frameRadius : 0

        CaptionButton {
            role: WindowButton.Minimize
            height: parent.height
        }
        CaptionButton {
            role: WindowButton.Maximize
            height: parent.height
        }
        CaptionButton {
            role: WindowButton.Close
            height: parent.height
        }
    }

    Rectangle {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        radius: !window.maximized ? Theme.frameRadius : 0
        color: "#0B0C10"

        Rectangle {
            visible: content.radius > 0
            anchors.left: content.left
            anchors.right: content.right
            anchors.top: content.top
            height: content.height / 2
            color: content.color
        }

        MouseArea {
            anchors.fill: parent
            onClicked: Login.loggedOut()
        }
    }
}
