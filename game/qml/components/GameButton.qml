import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: control

    property string text: ""
    property bool primary: true
    property bool gold: false
    property bool success: false
    property bool outline: false
    property bool fillWidth: false
    property bool elideText: false
    property real horizontalPadding: Theme.spacing * 3

    signal clicked()

    implicitWidth: Math.max(Theme.touchMin * 2.2, label.implicitWidth + horizontalPadding)
    implicitHeight: Theme.buttonHeight
    width: fillWidth && parent ? parent.width : implicitWidth

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Theme.radius
        color: {
            if (outline)
                return Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.35)
            if (gold)
                return Theme.gold
            if (success)
                return Theme.success
            if (primary)
                return Theme.primary
            return Qt.lighter(Theme.surfaceAlt, 1.08)
        }
        border.width: Theme.borderWidth
        border.color: gold ? Theme.gold
                           : (success ? Theme.success : (primary ? Theme.primary : Theme.secondary))
        scale: mouse.pressed ? 0.96 : 1.0
        opacity: enabled ? (mouse.containsMouse ? 1.0 : 0.96) : 0.45

        Behavior on scale { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutQuad } }

        Text {
            id: label
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacing
            text: control.text
            font.bold: true
            font.pixelSize: Theme.fontSizeButton
            color: outline ? Qt.lighter(Theme.primary, 1.08) : Theme.textOnAccent
            horizontalAlignment: Text.AlignHCenter
            elide: control.elideText ? Text.ElideRight : Text.ElideNone
            wrapMode: Text.NoWrap
            clip: true
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: control.enabled
        onClicked: control.clicked()
    }
}
