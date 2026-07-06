import QtQuick
import QtQuick.Controls
import ".."
import "."

Item {
    id: control

    property string placeholderText: ""
    property alias text: field.text
    property alias readOnly: field.readOnly

    property bool fillWidth: true
    property bool trailingAction: false

    signal textEdited(string text)
    signal trailingActionClicked()

    implicitWidth: 280
    implicitHeight: Theme.buttonHeight
    width: fillWidth && parent ? parent.width : implicitWidth

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface
        border.color: field.activeFocus ? Theme.primary : Theme.secondary
        border.width: Theme.borderWidth

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: trailingBtn.visible ? trailingBtn.left : parent.right
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: Theme.spacing
            visible: field.text.length === 0 && !field.activeFocus
            text: control.placeholderText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
        }

        Item {
            id: trailingBtn
            visible: control.trailingAction
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacing * 0.35
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height - Theme.spacing * 0.7
            width: height

            Rectangle {
                anchors.fill: parent
                radius: Theme.radius * 0.75
                color: trailMouse.containsMouse
                       ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.18)
                       : Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.55)
                border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.55)
                border.width: Theme.borderWidth
            }

            HiResImage {
                anchors.centerIn: parent
                width: Theme.iconSm
                height: Theme.iconSm
                source: "qrc:/qml/assets/icon-add.svg"
                resolutionScale: 3
            }

            MouseArea {
                id: trailMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: control.trailingActionClicked()
            }
        }

        TextInput {
            id: field
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: trailingBtn.visible ? trailingBtn.left : parent.right
            anchors.leftMargin: Theme.spacing
            anchors.rightMargin: trailingBtn.visible ? Theme.spacing * 0.35 : Theme.spacing
            verticalAlignment: Text.AlignVCenter
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            clip: true
            selectByMouse: true
            onTextChanged: control.textEdited(text)
        }
    }
}
