import QtQuick
import ".."
import "."

Item {
    id: root

    property bool hasImage: false
    property bool loading: false
    property string imageSource: ""
    property bool selected: false

    signal clicked()

    implicitWidth: Theme.slotSize
    implicitHeight: Theme.slotSize

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: "transparent"
        border.width: root.selected ? 3 : 0
        border.color: Theme.gold
    }

    HiResImage {
        anchors.fill: parent
        visible: !root.hasImage && !root.loading
        source: "qrc:/qml/assets/slot-empty.svg"
        fillMode: Image.PreserveAspectFit
        resolutionScale: 4
    }

    Image {
        id: photo
        anchors.fill: parent
        anchors.margins: Theme.spacing * 0.35
        visible: root.hasImage
        source: root.imageSource
        fillMode: Image.PreserveAspectCrop
        cache: false
        smooth: true
        sourceSize: Qt.size(Math.ceil(width * 2), Math.ceil(height * 2))
        opacity: root.loading || status === Image.Loading ? 0.35 : 1
    }

    HiResImage {
        id: busySpinner
        anchors.centerIn: parent
        width: Theme.iconLg
        height: Theme.iconLg
        visible: root.loading || (root.hasImage && photo.status === Image.Loading)
        source: "qrc:/qml/assets/spinner.svg"
        fillMode: Image.PreserveAspectFit
        resolutionScale: 3

        RotationAnimation on rotation {
            from: 0
            to: 360
            duration: 850
            loops: Animation.Infinite
            running: busySpinner.visible
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.loading
        onClicked: root.clicked()
    }
}
