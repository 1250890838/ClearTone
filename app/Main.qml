import QtQuick

FramelessWindow {
    id: window

    width: 1000
    height: 680
    minimumWidth: 480
    minimumHeight: 320
    visible: true
    title: qsTr("ClearTone")
    color: "#0B0C10"

    TitleBar {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: window.title

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
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        color: "#0B0C10"
    }
}
