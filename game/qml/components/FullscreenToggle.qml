import QtQuick
import QtQuick.Window
import ".."
import "."

Item {
    id: control

    property Window window: null

    implicitWidth: Theme.chromeSize
    implicitHeight: Theme.chromeSize

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
    }

    HiResImage {
        anchors.centerIn: parent
        width: Theme.iconSm
        height: Theme.iconSm
        source: control.isFullscreen
                ? "qrc:/qml/assets/icon-restore.svg"
                : "qrc:/qml/assets/icon-fullscreen.svg"
        resolutionScale: 3
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (!control.window) {
                return
            }
            if (control.isFullscreen) {
                control.window.visibility = Window.Windowed
            } else {
                control.window.visibility = Window.FullScreen
            }
        }
    }
}
