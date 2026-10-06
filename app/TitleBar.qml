import QtQuick

Rectangle {
    id: root

    FramelessWindow.dragRegion: true

    implicitHeight: 32
    color: "#14161C"

    // Linux 上窗口背景透明，标题栏自带顶部圆角；底部的圆角用同色矩形盖住
    property real cornerRadius: 0
    radius: cornerRadius

    property string title: ""
    property color titleColor: "#9AA0AC"
    property alias titleItem: titleLabel

    default property alias controls: controlRow.data

    Rectangle {
        visible: root.cornerRadius > 0
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom
        height: root.height / 2
        color: root.color
    }

    Text {
        id: titleLabel
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: controlRow.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        text: root.title
        color: root.titleColor
        visible: text.length > 0
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    Row {
        id: controlRow
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
}
