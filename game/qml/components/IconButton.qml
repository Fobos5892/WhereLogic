import QtQuick
import ".."
import "."

Item {
    id: root

    property alias iconSource: art.source
    property bool outline: true
    property bool primary: false

    signal clicked()

    implicitWidth: Theme.chromeSize
    implicitHeight: Theme.chromeSize

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: root.outline ? "transparent" : Theme.surface
        border.color: Theme.primary
        border.width: Theme.borderWidth
        opacity: mouse.pressed ? 0.85 : 1
        scale: mouse.pressed ? 0.94 : 1
        Behavior on scale { NumberAnimation { duration: Theme.animFast } }
    }

    HiResImage {
        id: art
        anchors.centerIn: parent
        width: Theme.iconMd
        height: Theme.iconMd
        resolutionScale: 3
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
