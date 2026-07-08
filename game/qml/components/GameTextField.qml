import QtQuick
import QtQuick.Controls
import ".."
import "."

Item {
    id: control

    property string placeholderText: ""
    property string text: ""
    property alias readOnly: field.readOnly

    property bool fillWidth: true
    property bool trailingAction: false
    property bool autosaveOnFocusLost: false

    signal textEdited(string text)
    signal trailingActionClicked()
    signal focusLost()

    function nearestAutosaveScope() {
        var node = control.parent
        while (node) {
            if (typeof node.requestSave === "function")
                return node
            node = node.parent
        }
        return null
    }

    function notifyAutosave() {
        if (!control.autosaveOnFocusLost)
            return
        const scope = control.nearestAutosaveScope()
        if (scope)
            scope.requestSave()
    }

    implicitWidth: 280
    implicitHeight: Theme.buttonHeight

    onTextChanged: {
        if (!field.activeFocus && field.text !== control.text)
            field.text = control.text
    }

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
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: Theme.spacing * 0.3
            anchors.bottomMargin: Theme.spacing * 0.3
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
                width: Theme.iconMd
                height: Theme.iconMd
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
            onTextEdited: control.textEdited(text)
            onActiveFocusChanged: {
                if (!activeFocus) {
                    control.focusLost()
                    control.notifyAutosave()
                }
            }
        }

        Binding {
            target: field
            property: "text"
            value: control.text
            when: !field.activeFocus
        }
    }
}
