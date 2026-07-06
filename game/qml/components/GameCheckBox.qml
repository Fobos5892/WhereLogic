import QtQuick
import ".."
import "."

Item {
    id: control

    property bool checked: false

    signal toggled(bool checked)

    implicitWidth: Theme.touchMin
    implicitHeight: Theme.touchMin

    Rectangle {
        width: Theme.iconLg
        height: Theme.iconLg
        anchors.centerIn: parent
        radius: Theme.radius * 0.5
        color: Theme.surface
        border.color: control.checked ? Theme.gold : Theme.secondary
        border.width: Theme.borderWidth
    }

    HiResImage {
        anchors.centerIn: parent
        width: Theme.iconSm
        height: Theme.iconSm
        visible: control.checked
        source: "qrc:/qml/assets/icon-check.svg"
        resolutionScale: 3
    }

    MouseArea {
        anchors.fill: parent
        onClicked: control.toggled(!control.checked)
    }
}
