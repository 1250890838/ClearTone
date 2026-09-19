import QtQuick
import QtQuick.Window

import ClearTone

WindowButton {
    id: root

    implicitWidth: 46
    implicitHeight: 32

    property color glyphColor: "#D8D8D8"
    property color hoverGlyphColor: "#FFFFFF"
    property color hoverColor: "#2B2E36"
    property color pressedColor: "#21242B"
    property color closeHoverColor: "#C42B1C"
    property color closePressedColor: "#A5261A"
    property string glyphFont: "Segoe Fluent Icons"
    property int glyphPixelSize: 10

    property bool autoAction: true

    readonly property bool _isClose: role === WindowButton.Close

    readonly property bool _zoomed: {
        const w = root.Window.window;
        if (!w)
            return false;
        if (w.maximized !== undefined)
            return w.maximized;
        return w.visibility === Window.Maximized || w.visibility === Window.FullScreen;
    }

    readonly property string _glyph: {
        switch (role) {
        case WindowButton.Minimize:
            return "\uE921";
        case WindowButton.Maximize:
            return _zoomed ? "\uE923" : "\uE922";
        case WindowButton.Close:
            return "\uE8BB";
        }
        return "";
    }
    Rectangle {
        anchors.fill: parent

        color: {
            if (root._isClose)
                return root.pressed ? root.closePressedColor : (root.hovered ? root.closeHoverColor : "transparent");
            return root.pressed ? root.pressedColor : (root.hovered ? root.hoverColor : "transparent");
        }

        Behavior on color {
            ColorAnimation {
                duration: 90
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: root._glyph
        font.family: root.glyphFont
        font.pixelSize: root.glyphPixelSize
        renderType: Text.NativeRendering
        color: (root.hovered || root.pressed) ? (root._isClose ? "#FFFFFF" : root.hoverGlyphColor) : root.glyphColor
    }

    onClicked: {
        if (!root.autoAction)
            return;
        const w = root.Window.window;
        if (!w)
            return;
        switch (root.role) {
        case WindowButton.Minimize:
            w.showMinimized();
            break;
        case WindowButton.Maximize:
            if (typeof w.toggleMaximize === "function")
                w.toggleMaximize();
            else
                w.visibility === Window.Maximized ? w.showNormal() : w.showMaximized();
            break;
        case WindowButton.Close:
            w.close();
            break;
        }
    }
}
