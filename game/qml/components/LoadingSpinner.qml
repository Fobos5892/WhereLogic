import QtQuick
import ".."

Item {
    id: root

    property bool running: true
    property int size: Theme.iconLg * 1.5

    implicitWidth: size
    implicitHeight: size

    Image {
        id: spinnerArt
        anchors.centerIn: parent
        width: root.size
        height: root.size
        source: "qrc:/qml/assets/spinner.svg"
        fillMode: Image.PreserveAspectFit

        transformOrigin: Item.Center

        RotationAnimation on rotation {
            from: 0
            to: 360
            duration: Theme.animNormal * 2
            loops: Animation.Infinite
            running: root.running && root.visible
        }
    }
}
