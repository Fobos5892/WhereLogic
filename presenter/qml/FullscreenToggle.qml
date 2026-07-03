import QtQuick
import QtQuick.Window

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
        radius: 8
        color: PresenterTheme.surface
        border.color: PresenterTheme.primary
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: control.isFullscreen ? "\u29C9" : "\u26F6"
            color: PresenterTheme.primary
            font.pixelSize: 16
        }
    }

    MouseArea {
        anchors.fill: parent
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
