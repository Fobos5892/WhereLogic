import QtQuick
import QtQuick.Controls
import ".."
import "../components"

Rectangle {
    id: root
    color: "#EE0B0C10"

    signal closeRequested()

    readonly property bool editorVisible: !adminViewModel.roundConfigOpen

    RoundConfigOverlay {
        anchors.fill: parent
        z: 10
    }

    Item {
        id: chromeBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.margin
        visible: root.editorVisible
        height: visible
              ? headerRow.height
                + (statusLabel.visible ? statusLabel.height + Theme.spacing * 0.5 : 0)
              : 0

        Item {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: Math.max(titleText.height, closeButton.height)

            GameButton {
                id: closeButton
                anchors.top: parent.top
                anchors.right: parent.right
                text: adminViewModel.label("ui.editor.close")
                primary: false
                outline: true
                onClicked: root.closeRequested()
            }

            Text {
                id: titleText
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.rightMargin: Theme.spacing
                anchors.verticalCenter: parent.verticalCenter
                text: adminViewModel.label("ui.editor.title")
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                wrapMode: Text.WordWrap
            }
        }

        Text {
            id: statusLabel
            anchors.top: headerRow.bottom
            anchors.topMargin: Theme.spacing * 0.5
            anchors.left: parent.left
            anchors.right: parent.right
            text: adminViewModel.statusMessage
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeCaption
            wrapMode: Text.WordWrap
            visible: adminViewModel.statusMessage.length > 0
        }

    }

    ScrollView {
        id: editorScroll
        anchors.top: chromeBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.margin
        anchors.topMargin: Theme.spacing * 0.5
        clip: true
        visible: root.editorVisible
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: ThemedScrollBar {
            policy: ScrollBar.AsNeeded
        }
        rightPadding: Theme.spacing * 0.35

        Column {
            id: scrollContent
            width: editorScroll.availableWidth - editorScroll.leftPadding - editorScroll.rightPadding
            spacing: Theme.spacing * 1.65

            NeonPanel {
                id: presetPanel
                width: scrollContent.width

                Column {
                    width: presetPanel.innerWidth
                    spacing: Theme.spacing * 1.1

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.preset_section")
                        color: Theme.primary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                    }

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.new_preset")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                    }

                    GameTextField {
                        width: parent.width
                        fillWidth: true
                        trailingAction: true
                        placeholderText: adminViewModel.label("ui.editor.preset_name")
                        text: adminViewModel.editPresetName
                        onTextEdited: function(t) { adminViewModel.editPresetName = t }
                        onTrailingActionClicked: adminViewModel.createPreset()
                    }

                    Item {
                        width: parent.width
                        height: Theme.spacing * 0.35
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Qt.rgba(Theme.textSecondary.r, Theme.textSecondary.g, Theme.textSecondary.b, 0.22)
                    }

                    Item {
                        width: parent.width
                        height: Theme.spacing * 0.65
                    }

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.preset_list")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                    }

                    Repeater {
                        model: adminViewModel.presets

                        Item {
                            width: parent.width
                            height: Theme.buttonHeight

                            readonly property bool presetSelected:
                                adminViewModel.selectedPresetId === modelData.id

                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radius
                                color: presetSelected
                                       ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.12)
                                       : Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.38)
                                border.width: 1
                                border.color: presetSelected
                                          ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.55)
                                          : Qt.rgba(Theme.textSecondary.r, Theme.textSecondary.g, Theme.textSecondary.b, 0.16)
                            }

                            Rectangle {
                                visible: presetSelected
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: Theme.spacing * 0.45
                                radius: 2
                                color: Theme.primary
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.right: trashBtn.left
                                anchors.leftMargin: Theme.spacing
                                anchors.rightMargin: Theme.spacing * 0.75
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.name
                                color: presetSelected ? Theme.primary : Theme.textPrimary
                                font.bold: presetSelected
                                font.pixelSize: Theme.fontSizeBody
                                elide: Text.ElideRight
                            }

                            TrashButton {
                                id: trashBtn
                                anchors.right: parent.right
                                anchors.rightMargin: Theme.spacing * 0.35
                                anchors.verticalCenter: parent.verticalCenter
                                height: parent.height - Theme.spacing * 0.6
                                width: height
                                onClicked: adminViewModel.deletePreset(modelData.id)
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.rightMargin: trashBtn.width + Theme.spacing
                                hoverEnabled: true
                                onClicked: adminViewModel.selectedPresetId = modelData.id
                            }
                        }
                    }

                    GameButton {
                        width: parent.width
                        fillWidth: true
                        text: adminViewModel.label("ui.editor.save_preset")
                        onClicked: adminViewModel.savePresetMeta()
                    }
                }
            }

            NeonPanel {
                id: roundsPanel
                width: scrollContent.width

                Column {
                    width: roundsPanel.innerWidth
                    spacing: Theme.spacing * 1.1

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.rounds_section")
                        color: Theme.primary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                    }

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.rounds_section_hint")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: adminViewModel.catalogRounds

                        Item {
                            width: parent.width
                            height: Math.max(Theme.buttonHeight, roundTitle.implicitHeight + Theme.spacing * 0.6)

                            readonly property int roundId: modelData.id
                            readonly property real rowPad: Theme.spacing * 0.65
                            readonly property bool roundEnabled: {
                                var ids = adminViewModel.presetRoundIds
                                for (var i = 0; i < ids.length; ++i) {
                                    if (ids[i] === roundId) {
                                        return true
                                    }
                                }
                                return false
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radius
                                color: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, roundEnabled ? 0.34 : 0.22)
                                border.width: 1
                                border.color: Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.25)
                            }

                            DataStatusIcon {
                                id: statusIcon
                                width: roundEnabled ? Theme.iconLg : 0
                                height: Theme.iconLg
                                anchors.left: parent.left
                                anchors.leftMargin: rowPad
                                anchors.verticalCenter: parent.verticalCenter
                                visible: roundEnabled
                                status: modelData.dataStatus !== undefined
                                        ? modelData.dataStatus
                                        : adminViewModel.roundTemplateStatus(roundId)
                            }

                            GameCheckBox {
                                id: roundCheck
                                anchors.left: statusIcon.right
                                anchors.leftMargin: roundEnabled ? Theme.spacing * 0.45 : rowPad
                                anchors.verticalCenter: parent.verticalCenter
                                checked: roundEnabled
                                onToggled: function(on) {
                                    adminViewModel.setRoundEnabled(roundId, on)
                                }
                            }

                            GameButton {
                                id: configureBtn
                                anchors.right: parent.right
                                anchors.rightMargin: rowPad
                                anchors.verticalCenter: parent.verticalCenter
                                text: adminViewModel.label("ui.editor.configure_round")
                                primary: false
                                outline: true
                                enabled: roundEnabled
                                       && adminViewModel.selectedPresetId > 0
                                onClicked: adminViewModel.openRoundConfig(roundId)
                            }

                            Text {
                                id: roundTitle
                                anchors.left: roundCheck.right
                                anchors.right: configureBtn.left
                                anchors.leftMargin: Theme.spacing * 0.5
                                anchors.rightMargin: Theme.spacing
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.title
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBody
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }
        }
    }
}
