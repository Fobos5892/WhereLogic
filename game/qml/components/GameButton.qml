import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: control

    property string text: ""
    property bool primary: true
    property bool gold: false
    property bool outline: false
    property bool fillWidth: false

    signal clicked()

    implicitWidth: fillWidth ? parent.width : Math.max(Theme.touchMin * 2.2, label.implicitWidth + Theme.spacing * 3)
    implicitHeight: Theme.buttonHeight

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Theme.radius
        color: {
            if (outline)
                return "transparent"
            if (gold)
                return Theme.gold
            if (primary)
                return Theme.primary
            return Theme.surfaceAlt
        }
        border.width: Theme.borderWidth
        border.color: gold ? Theme.gold : (primary ? Theme.primary : Theme.secondary)
        scale: mouse.pressed ? 0.96 : 1.0
        opacity: enabled ? 1.0 : 0.45

        Behavior on scale { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutQuad } }

        Text {
            id: label
            anchors.centerIn: parent
            text: control.text
            font.bold: true
            font.pixelSize: Theme.fontSizeButton
            color: outline ? Theme.primary : Theme.textOnAccent
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: control.enabled
        onClicked: control.clicked()
    }
}
