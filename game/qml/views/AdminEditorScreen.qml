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
                    Layout.fillWidth: true
                    fillWidth: true
                    text: adminViewModel.label("ui.editor.photo_quick_start")
                    gold: true
                    onClicked: adminViewModel.startPhotoPuzzle()
                }
            }
        }

        ScrollView {
            id: editorScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: editorScroll.availableWidth
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
                            fillWidth: true
                            placeholderText: adminViewModel.label("ui.editor.preset_name")
                            text: adminViewModel.editPresetName
                            onTextEdited: function(t) { adminViewModel.editPresetName = t }
                        }

                        Repeater {
                            model: adminViewModel.presets
                            delegate: Item {
                                Layout.fillWidth: true
                                implicitHeight: presetBtn.implicitHeight

                                GameButton {
                                    id: presetBtn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    fillWidth: true
                                    text: modelData.name
                                    primary: adminViewModel.selectedPresetId === modelData.id
                                    outline: adminViewModel.selectedPresetId !== modelData.id
                                    onClicked: adminViewModel.selectedPresetId = modelData.id
                                }
                            }
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            text: adminViewModel.label("ui.editor.new_preset")
                            onClicked: adminViewModel.createPreset()
                        }
                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            text: adminViewModel.label("ui.editor.save_preset")
                            onClicked: adminViewModel.savePresetMeta()
                        }
                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            text: adminViewModel.label("ui.editor.delete_preset")
                            primary: false
                            outline: true
                            onClicked: adminViewModel.deleteSelectedPreset()
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
                            delegate: Item {
                                Layout.fillWidth: true
                                implicitHeight: roundBtn.implicitHeight

                                GameButton {
                                    id: roundBtn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    fillWidth: true
                                    text: modelData.title + " (" + modelData.layoutType + ")"
                                    primary: root.roundEnabled(modelData.id)
                                    outline: !root.roundEnabled(modelData.id)
                                    onClicked: adminViewModel.togglePresetRound(modelData.id, !root.roundEnabled(modelData.id))
                                }
                            }
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
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

                        Repeater {
                            model: adminViewModel.catalogRounds
                            delegate: Item {
                                Layout.fillWidth: true
                                visible: root.roundEnabled(modelData.id)
                                implicitHeight: visible ? roundPickBtn.implicitHeight : 0

                                GameButton {
                                    id: roundPickBtn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    fillWidth: true
                                    text: modelData.title
                                    primary: adminViewModel.selectedRoundId === modelData.id
                                    outline: adminViewModel.selectedRoundId !== modelData.id
                                    onClicked: adminViewModel.selectedRoundId = modelData.id
                                }
                            }
                        }

                        Repeater {
                            model: adminViewModel.puzzles
                            delegate: Item {
                                Layout.fillWidth: true
                                visible: adminViewModel.selectedRoundId > 0
                                implicitHeight: visible ? puzzlePickBtn.implicitHeight : 0

                                GameButton {
                                    id: puzzlePickBtn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    fillWidth: true
                                    text: "#" + modelData.sortOrder + " " + modelData.answer
                                    primary: adminViewModel.selectedPuzzleId === modelData.id
                                    outline: adminViewModel.selectedPuzzleId !== modelData.id
                                    onClicked: adminViewModel.selectPuzzle(modelData.id)
                                }
                            }
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            visible: adminViewModel.selectedRoundId > 0
                            text: adminViewModel.label("ui.editor.new_puzzle")
                            onClicked: adminViewModel.createPuzzle()
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
                            fillWidth: true
                            placeholderText: adminViewModel.label("ui.editor.answer")
                            text: adminViewModel.editAnswer
                            onTextEdited: function(t) { adminViewModel.editAnswer = t }
                        }

                        GameTextField {
                            Layout.fillWidth: true
                            fillWidth: true
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
                            visible: adminViewModel.imageSlotCount > 0
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: adminViewModel.showGamePreview
                            text: adminViewModel.label("ui.editor.game_preview_hint")
                            color: Theme.gold
                            font.pixelSize: Theme.fontSizeCaption
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacing
                            visible: adminViewModel.imageSlotCount > 0

                            Repeater {
                                model: adminViewModel.imageSlotCount
                                delegate: Item {
                                    Layout.preferredWidth: 76
                                    Layout.preferredHeight: 76

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: Theme.radius
                                        color: Theme.surface
                                        border.width: adminViewModel.selectedImageSlot === index ? 3 : Theme.borderWidth
                                        border.color: adminViewModel.selectedImageSlot === index
                                                      ? Theme.gold : Theme.secondary

                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            source: adminViewModel.slotThumbnailUrl(index)
                                            fillMode: Image.PreserveAspectCrop
                                            visible: adminViewModel.slotHasImage(index)
                                            cache: false
                                        }

                                        Text {
                                            anchors.centerIn: parent
                                            visible: !adminViewModel.slotHasImage(index)
                                            text: (index + 1).toString()
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeTitle
                                            font.bold: true
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: adminViewModel.selectedImageSlot = index
                                        }
                                    }
                                }
                            }
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            visible: adminViewModel.imageSlotCount > 0
                            text: adminViewModel.label("ui.editor.pick_image")
                            enabled: adminViewModel.imageSlotCount > 0 && !adminViewModel.imageProcessing
                            onClicked: imageFileDialog.open()
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            text: adminViewModel.label("ui.editor.clear_mask")
                            primary: false
                            outline: true
                            visible: adminViewModel.isPhotoMaskRound
                                     && adminViewModel.selectedImageSlot === 0
                                     && adminViewModel.hasMaskContour
                            onClicked: adminViewModel.clearMask()
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
                                opacity: adminViewModel.imageProcessing ? 0.45 : 1

                                MaskRegionSelector {
                                    anchors.fill: parent
                                    imageItem: puzzlePreview
                                    active: adminViewModel.isPhotoMaskRound
                                          && adminViewModel.selectedImageSlot === 0
                                          && !adminViewModel.imageProcessing
                                    onRegionSelected: function(relX, relY, relW, relH) {
                                        adminViewModel.markMissingRegion(relX, relY, relW, relH)
                                    }
                                }
                            }

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: adminViewModel.imageProcessing
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: adminViewModel.imageSlotCount > 1
                            text: adminViewModel.label("ui.editor.slot_pick_hint")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeCaption
                            wrapMode: Text.WordWrap
                        }

                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
                            text: adminViewModel.label("ui.editor.save_puzzle")
                            gold: true
                            onClicked: adminViewModel.savePuzzle()
                        }
                        GameButton {
                            Layout.fillWidth: true
                            fillWidth: true
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
