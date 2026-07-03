import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Rectangle {
    id: root
    color: "#EE0B0C10"

    signal closeRequested()

    function roundEnabled(roundId) {
        for (let i = 0; i < adminViewModel.presetRoundIds.length; ++i) {
            if (adminViewModel.presetRoundIds[i] === roundId) {
                return true
            }
        }
        return false
    }

    FileDialog {
        id: imageFileDialog
        title: adminViewModel.label("ui.editor.pick_image")
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: adminViewModel.importPuzzleImage(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.margin
        spacing: Theme.spacing

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: adminViewModel.label("ui.editor.title")
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
            }

            GameButton {
                text: adminViewModel.label("ui.editor.close")
                primary: false
                outline: true
                onClicked: root.closeRequested()
            }
        }

        Text {
            Layout.fillWidth: true
            text: adminViewModel.statusMessage
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeCaption
            wrapMode: Text.WordWrap
            visible: adminViewModel.statusMessage.length > 0
        }

        NeonPanel {
            Layout.fillWidth: true
            implicitHeight: photoQuickCol.implicitHeight + Theme.spacing * 2

            ColumnLayout {
                id: photoQuickCol
                anchors.fill: parent
                spacing: Theme.spacing

                Text {
                    Layout.fillWidth: true
                    text: adminViewModel.label("ui.editor.photo_quick_hint")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.WordWrap
                }

                GameButton {
                    text: adminViewModel.label("ui.editor.photo_quick_start")
                    gold: true
                    onClicked: adminViewModel.startPhotoPuzzle()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.min(root.width - Theme.margin * 2, 960)
                spacing: Theme.spacing * 1.5

                NeonPanel {
                    Layout.fillWidth: true
                    implicitHeight: presetCol.implicitHeight + Theme.spacing * 2

                    ColumnLayout {
                        id: presetCol
                        anchors.fill: parent
                        spacing: Theme.spacing

                        Text {
                            Layout.fillWidth: true
                            text: adminViewModel.label("ui.editor.preset_section")
                            color: Theme.primary
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBody
                        }

                        GameTextField {
                            Layout.fillWidth: true
                            placeholderText: adminViewModel.label("ui.editor.preset_name")
                            text: adminViewModel.editPresetName
                            onTextEdited: function(t) { adminViewModel.editPresetName = t }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing

                            Repeater {
                                model: adminViewModel.presets
                                delegate: GameButton {
                                    text: modelData.name
                                    primary: adminViewModel.selectedPresetId === modelData.id
                                    outline: adminViewModel.selectedPresetId !== modelData.id
                                    onClicked: adminViewModel.selectedPresetId = modelData.id
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing

                            GameButton {
                                text: adminViewModel.label("ui.editor.new_preset")
                                onClicked: adminViewModel.createPreset()
                            }
                            GameButton {
                                text: adminViewModel.label("ui.editor.save_preset")
                                onClicked: adminViewModel.savePresetMeta()
                            }
                            GameButton {
                                text: adminViewModel.label("ui.editor.delete_preset")
                                primary: false
                                outline: true
                                onClicked: adminViewModel.deleteSelectedPreset()
                            }
                        }
                    }
                }

                NeonPanel {
                    Layout.fillWidth: true
                    implicitHeight: roundsCol.implicitHeight + Theme.spacing * 2

                    ColumnLayout {
                        id: roundsCol
                        anchors.fill: parent
                        spacing: Theme.spacing

                        Text {
                            Layout.fillWidth: true
                            text: adminViewModel.label("ui.editor.rounds_section")
                            color: Theme.primary
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBody
                        }

                        Repeater {
                            model: adminViewModel.catalogRounds
                            delegate: GameButton {
                                Layout.fillWidth: true
                                text: modelData.title + " (" + modelData.layoutType + ")"
                                primary: root.roundEnabled(modelData.id)
                                outline: !root.roundEnabled(modelData.id)
                                onClicked: adminViewModel.togglePresetRound(modelData.id, !root.roundEnabled(modelData.id))
                            }
                        }

                        GameButton {
                            Layout.fillWidth: true
                            text: adminViewModel.label("ui.editor.save_rounds")
                            gold: true
                            onClicked: adminViewModel.savePresetRounds()
                        }
                    }
                }

                NeonPanel {
                    Layout.fillWidth: true
                    implicitHeight: puzzleCol.implicitHeight + Theme.spacing * 2

                    ColumnLayout {
                        id: puzzleCol
                        anchors.fill: parent
                        spacing: Theme.spacing

                        Text {
                            Layout.fillWidth: true
                            text: adminViewModel.label("ui.editor.puzzles_section")
                            color: Theme.primary
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBody
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing

                            Repeater {
                                model: adminViewModel.catalogRounds
                                delegate: GameButton {
                                    text: modelData.title
                                    primary: adminViewModel.selectedRoundId === modelData.id
                                    outline: adminViewModel.selectedRoundId !== modelData.id
                                    enabled: root.roundEnabled(modelData.id)
                                    onClicked: adminViewModel.selectedRoundId = modelData.id
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing
                            visible: adminViewModel.selectedRoundId > 0

                            Repeater {
                                model: adminViewModel.puzzles
                                delegate: GameButton {
                                    text: "#" + modelData.sortOrder + " " + modelData.answer
                                    primary: adminViewModel.selectedPuzzleId === modelData.id
                                    outline: adminViewModel.selectedPuzzleId !== modelData.id
                                    onClicked: adminViewModel.selectPuzzle(modelData.id)
                                }
                            }

                            GameButton {
                                text: adminViewModel.label("ui.editor.new_puzzle")
                                onClicked: adminViewModel.createPuzzle()
                            }
                        }
                    }
                }

                NeonPanel {
                    Layout.fillWidth: true
                    visible: adminViewModel.selectedPuzzleId > 0
                    implicitHeight: editorCol.implicitHeight + Theme.spacing * 2

                    ColumnLayout {
                        id: editorCol
                        anchors.fill: parent
                        spacing: Theme.spacing

                        Text {
                            Layout.fillWidth: true
                            text: adminViewModel.label("ui.editor.puzzle_section")
                            color: Theme.gold
                            font.bold: true
                            font.pixelSize: Theme.fontSizeBody
                        }

                        GameTextField {
                            Layout.fillWidth: true
                            placeholderText: adminViewModel.label("ui.editor.answer")
                            text: adminViewModel.editAnswer
                            onTextEdited: function(t) { adminViewModel.editAnswer = t }
                        }

                        GameTextField {
                            Layout.fillWidth: true
                            placeholderText: adminViewModel.label("ui.editor.hint")
                            text: adminViewModel.editHint
                            onTextEdited: function(t) { adminViewModel.editHint = t }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: !adminViewModel.isPhotoMaskRound
                            text: adminViewModel.label("ui.editor.quotes_hint")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeCaption
                            wrapMode: Text.WordWrap
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 120
                            visible: !adminViewModel.isPhotoMaskRound
                            radius: Theme.radius
                            color: Theme.surface
                            border.color: Theme.secondary
                            border.width: Theme.borderWidth

                            ScrollView {
                                anchors.fill: parent
                                anchors.margins: Theme.spacing
                                TextArea {
                                    text: adminViewModel.editQuotes
                                    placeholderText: adminViewModel.label("ui.editor.quotes_placeholder")
                                    color: Theme.textPrimary
                                    placeholderTextColor: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeBody
                                    wrapMode: TextArea.Wrap
                                    background: null
                                    onTextChanged: adminViewModel.editQuotes = text
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: adminViewModel.isPhotoMaskRound
                                  ? adminViewModel.label("ui.editor.photo_tap_hint")
                                  : adminViewModel.label("ui.editor.image_hint")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeCaption
                            wrapMode: Text.WordWrap
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing

                            GameButton {
                                text: adminViewModel.label("ui.editor.pick_image")
                                onClicked: imageFileDialog.open()
                            }
                            GameButton {
                                text: adminViewModel.label("ui.editor.clear_mask")
                                primary: false
                                outline: true
                                visible: adminViewModel.hasMaskContour
                                onClicked: adminViewModel.clearMask()
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 280
                            radius: Theme.radius
                            color: Theme.surfaceAlt
                            visible: adminViewModel.hasPreviewImage

                            Image {
                                id: puzzlePreview
                                anchors.fill: parent
                                anchors.margins: Theme.spacing
                                source: adminViewModel.previewImageUrl
                                fillMode: Image.PreserveAspectFit
                                cache: false

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: function(mouse) {
                                        const pw = puzzlePreview.paintedWidth
                                        const ph = puzzlePreview.paintedHeight
                                        if (pw <= 0 || ph <= 0) {
                                            return
                                        }
                                        const left = (puzzlePreview.width - pw) / 2
                                        const top = (puzzlePreview.height - ph) / 2
                                        const rx = (mouse.x - left) / pw
                                        const ry = (mouse.y - top) / ph
                                        if (rx >= 0 && rx <= 1 && ry >= 0 && ry <= 1) {
                                            adminViewModel.markMissingArea(rx, ry)
                                        }
                                    }
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: Theme.spacing

                            GameButton {
                                text: adminViewModel.label("ui.editor.save_puzzle")
                                gold: true
                                onClicked: adminViewModel.savePuzzle()
                            }
                            GameButton {
                                text: adminViewModel.label("ui.editor.delete_puzzle")
                                primary: false
                                outline: true
                                onClicked: adminViewModel.deleteSelectedPuzzle()
                            }
                        }
                    }
                }
            }
        }
    }
}
