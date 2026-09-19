import QtQuick

Rectangle {
    id: root

    FramelessWindow.dragRegion: true

    implicitHeight: 32
    color: "#14161C"

    property string title: ""
    property color titleColor: "#9AA0AC"
    property alias titleItem: titleLabel

    default property alias controls: controlRow.data

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
