import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "."
import "components"

ApplicationWindow {
    id: root
    width: 960
    height: 540
    minimumWidth: 480
    minimumHeight: 270
    visible: true
    title: gameViewModel.label("ui.app.title")
    color: Theme.background
    contentOrientation: Qt.LandscapeOrientation
    flags: Qt.Window | Qt.FramelessWindowHint
    visibility: Window.Windowed

    Component.onCompleted: Theme.window = root

    // Перетаскивание окна (когда не на весь экран)
    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.touchMin * 0.75
        z: 50
        visible: root.visibility === Window.Windowed
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton)
                root.startSystemMove()
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#0B0C10" }
            GradientStop { position: 0.5; color: "#12151C" }
            GradientStop { position: 1; color: "#0B0C10" }
        }
    }

    Repeater {
        model: 12
        Rectangle {
            x: (index % 4) * (root.width / 4)
            y: Math.floor(index / 4) * (root.height / 3)
            width: root.width / 4
            height: 1
            color: Theme.primary
            opacity: 0.04
        }
    }

    MainContainer {
        id: mainContainer
        anchors.fill: parent
        anchors.margins: Theme.margin * 0.5
    }

    FullscreenToggle {
        z: 200
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Theme.spacing
        visible: !mainContainer.overlaysOpen
        window: root
    }
}
