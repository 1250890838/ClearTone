import QtQuick
import QtQuick.Controls

import Core

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Button{
        width: 40
        height: 40
        onClicked: Test.song()
    }
}
