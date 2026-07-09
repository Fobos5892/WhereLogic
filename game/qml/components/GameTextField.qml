import QtQuick
import QtQuick.Controls
import ".."
import "."

Item {
    id: control

    property string placeholderText: ""
    property string prefixText: ""
    property string text: ""
    property alias readOnly: field.readOnly

    property bool fillWidth: true
    property bool trailingAction: false
    property bool autosaveOnFocusLost: false
    property int horizontalAlignment: Text.AlignLeft
    readonly property bool hasPrefix: control.prefixText.length > 0

    readonly property alias inputActiveFocus: field.activeFocus

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

    function commitInput() {
        control.text = field.text
        control.textEdited(field.text)
    }

    implicitWidth: Theme.w * 0.35
    implicitHeight: Theme.buttonHeight

    onTextChanged: {
        if (field.text !== control.text)
            field.text = control.text
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface
        border.color: field.activeFocus ? Theme.primary : Theme.secondary
        border.width: Theme.borderWidth

        Row {
            id: inputRow
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: control.horizontalAlignment === Text.AlignHCenter ? undefined : parent.left
            anchors.right: trailingBtn.visible ? trailingBtn.left : parent.right
            anchors.horizontalCenter: control.horizontalAlignment === Text.AlignHCenter ? parent.horizontalCenter : undefined
            anchors.leftMargin: control.horizontalAlignment === Text.AlignHCenter ? 0 : Theme.spacing
            anchors.rightMargin: Theme.spacing
            spacing: Theme.spacing * 0.45
            width: control.horizontalAlignment === Text.AlignHCenter
                   ? Math.min(implicitWidth, trailingBtn.visible ? parent.width - trailingBtn.width - Theme.spacing : parent.width)
                   : undefined

            Text {
                id: prefixLabel
                visible: control.hasPrefix
                text: control.prefixText
                color: Theme.gold
                font.pixelSize: Theme.fontSizeBody
                font.bold: true
            }

            Item {
                width: control.hasPrefix ? Math.max(0, inputRow.width - prefixLabel.width - inputRow.spacing) : inputRow.width
                height: parent.height

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    visible: field.text.length === 0 && !field.activeFocus
                    text: control.placeholderText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody
                    horizontalAlignment: control.hasPrefix ? Text.AlignLeft : control.horizontalAlignment
                }

                TextInput {
                    id: field
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: control.hasPrefix ? Text.AlignLeft : control.horizontalAlignment
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeBody
                    clip: true
                    selectByMouse: true
                    onTextEdited: {
                        control.text = text
                        control.textEdited(text)
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus) {
                            control.commitInput()
                            control.focusLost()
                            control.notifyAutosave()
                        }
                    }
                }
            }
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

    }
}
