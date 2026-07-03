import QtQuick
import QtQuick.Window
import ".."

Item {
    id: control

    property Window window: null

    implicitWidth: 36
    implicitHeight: 36

    readonly property bool isFullscreen: window
            && (window.visibility === Window.FullScreen
                || window.visibility === Window.Maximized)

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.primary
        border.width: Theme.borderWidth
        opacity: mouse.pressed ? 0.8 : 1.0
        scale: mouse.pressed ? 0.94 : 1.0

        Behavior on scale { NumberAnimation { duration: Theme.animFast } }

        Text {
            anchors.centerIn: parent
            text: control.isFullscreen ? "\u29C9" : "\u26F6"
            color: Theme.primary
            font.pixelSize: 16
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (!control.window)
                return
            if (control.isFullscreen)
                control.window.visibility = Window.Windowed
            else
                control.window.visibility = Window.FullScreen
        }
    }
}
