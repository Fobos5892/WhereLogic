import QtQuick
import ".."
import "."

Item {
    id: root

    signal clicked()

    implicitWidth: Theme.buttonHeight
    implicitHeight: Theme.buttonHeight

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: "#B71C1C"
    }

    HiResImage {
        anchors.centerIn: parent
        width: Theme.iconMd
        height: Theme.iconMd
        source: "qrc:/qml/assets/icon-trash.svg"
        resolutionScale: 3
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
