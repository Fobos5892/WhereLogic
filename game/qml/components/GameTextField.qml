import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: control

    property string placeholderText: ""
    property alias text: field.text
    property alias readOnly: field.readOnly

    property bool fillWidth: true

    signal textEdited(string text)

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
            anchors.leftMargin: Theme.spacing
            visible: field.text.length === 0 && !field.activeFocus
            text: control.placeholderText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
        }

        TextInput {
            id: field
            anchors.fill: parent
            anchors.margins: Theme.spacing
            verticalAlignment: Text.AlignVCenter
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            clip: true
            selectByMouse: true
            onTextChanged: control.textEdited(text)
        }
    }
}
