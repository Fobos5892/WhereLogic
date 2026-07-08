import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
        leftPadding: Theme.spacing * 0.35
        rightPadding: Theme.spacing * 0.35

        EditorAutoSaveScope {
            width: editorScroll.availableWidth - editorScroll.leftPadding - editorScroll.rightPadding
            implicitHeight: scrollContent.height
            onSave: adminViewModel.autosaveEditorDraft()

        Column {
            id: scrollContent
            width: parent.width
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

                    RowLayout {
                        width: parent.width
                        height: Theme.buttonHeight
                        spacing: Theme.spacing * 0.35

                        SettingsTextField {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            fillWidth: false
                            trailingAction: false
                            placeholderText: adminViewModel.label("ui.editor.preset_name")
                            text: adminViewModel.editPresetName
                            onTextEdited: function(t) { adminViewModel.editPresetName = t }
                        }

                        IconButton {
                            Layout.preferredWidth: Theme.chromeSize
                            Layout.preferredHeight: Theme.chromeSize
                            Layout.alignment: Qt.AlignVCenter
                            z: 2
                            iconSource: "qrc:/qml/assets/icon-add.svg"
                            outline: true
                            onClicked: adminViewModel.createPreset()
                        }
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

                        RowLayout {
                            width: parent.width
                            height: Theme.buttonHeight
                            spacing: Theme.spacing * 0.35

                            readonly property bool presetSelected:
                                adminViewModel.selectedPresetId === modelData.id

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                clip: true

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
                                    anchors.right: parent.right
                                    anchors.leftMargin: Theme.spacing
                                    anchors.rightMargin: Theme.spacing * 0.5
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.name
                                    color: presetSelected ? Theme.primary : Theme.textPrimary
                                    font.bold: presetSelected
                                    font.pixelSize: Theme.fontSizeBody
                                    elide: Text.ElideRight
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: adminViewModel.selectedPresetId = modelData.id
                                }
                            }

                            TrashButton {
                                Layout.preferredWidth: Theme.chromeSize
                                Layout.preferredHeight: Theme.chromeSize
                                Layout.alignment: Qt.AlignVCenter
                                z: 2
                                onClicked: adminViewModel.deletePreset(modelData.id)
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

                        RowLayout {
                            width: parent.width
                            readonly property bool hasRule: modelData.rule && modelData.rule.length > 0
                            height: hasRule
                                    ? Theme.buttonHeight + Theme.fontSizeCaption + Theme.spacing * 0.35
                                    : Theme.buttonHeight
                            spacing: Theme.spacing * 0.35

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

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                clip: true

                                Rectangle {
                                    anchors.fill: parent
                                    radius: Theme.radius
                                    color: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, roundEnabled ? 0.34 : 0.22)
                                    border.width: 1
                                    border.color: Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.25)
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: rowPad
                                    anchors.rightMargin: Theme.spacing * 0.35
                                    spacing: Theme.spacing * 0.45

                                    DataStatusIcon {
                                        Layout.preferredWidth: roundEnabled ? Theme.iconLg : 0
                                        Layout.preferredHeight: Theme.iconLg
                                        Layout.alignment: Qt.AlignVCenter
                                        visible: roundEnabled
                                        status: modelData.dataStatus !== undefined
                                                ? modelData.dataStatus
                                                : adminViewModel.roundTemplateStatus(roundId)
                                    }

                                    GameCheckBox {
                                        Layout.alignment: Qt.AlignVCenter
                                        checked: roundEnabled
                                        onToggled: function(on) {
                                            adminViewModel.setRoundEnabled(roundId, on)
                                        }
                                    }

                                Column {
                                    id: roundTitleRow
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: Theme.spacing * 0.2

                                    Text {
                                        width: parent.width
                                        text: modelData.title
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeBody
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        width: parent.width
                                        text: modelData.rule
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeCaption
                                        elide: Text.ElideRight
                                        visible: text.length > 0
                                    }
                                }
                                }
                            }

                            IconButton {
                                Layout.preferredWidth: Theme.chromeSize
                                Layout.preferredHeight: Theme.chromeSize
                                Layout.alignment: Qt.AlignVCenter
                                z: 2
                                iconSource: "qrc:/qml/assets/icon-settings.svg"
                                outline: true
                                onClicked: adminViewModel.openRoundConfig(roundId)
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
}
