import QtQuick
import ".."
import "."

Item {
    id: root

    signal clicked()

    implicitHeight: Theme.buttonHeight

    HiResImage {
        anchors.fill: parent
        source: "qrc:/qml/assets/slot-add-wide.svg"
        fillMode: Image.Stretch
        resolutionScale: 4
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
