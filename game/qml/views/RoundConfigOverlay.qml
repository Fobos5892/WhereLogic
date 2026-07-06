import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import ".."
import "../components"

Rectangle {
    id: root
    color: "#F00B0C10"
    visible: adminViewModel.roundConfigOpen
    z: 10

    FileDialog {
        id: imageFileDialog
        title: adminViewModel.label("ui.editor.pick_image")
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: adminViewModel.importPuzzleImage(selectedFile)
    }

    Item {
        id: configChrome
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.margin
        height: headerRow.height
            + (configStatus.visible ? configStatus.height + Theme.spacing * 0.5 : 0)

        Item {
            id: headerRow
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: Math.max(configTitle.height, backButton.height)

            GameButton {
                id: backButton
                anchors.top: parent.top
                anchors.right: parent.right
                text: adminViewModel.label("ui.editor.round_config_back")
                primary: false
                outline: true
                onClicked: adminViewModel.closeRoundConfig()
            }

            Text {
                id: configTitle
                anchors.left: parent.left
                anchors.right: backButton.left
                anchors.rightMargin: Theme.spacing
                anchors.verticalCenter: parent.verticalCenter
                text: adminViewModel.configRoundTitle
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                wrapMode: Text.WordWrap
            }
        }

        Text {
            id: configStatus
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
        id: configScroll
        anchors.top: configChrome.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.margin
        anchors.topMargin: Theme.spacing * 0.5
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: ThemedScrollBar {
            policy: ScrollBar.AsNeeded
        }
        rightPadding: Theme.spacing * 0.35

        Binding {
            target: configScroll.contentItem
            property: "interactive"
            value: !maskSelector.pointerLocked
            when: configScroll.contentItem
        }

        Column {
            width: configScroll.availableWidth - configScroll.leftPadding - configScroll.rightPadding
            spacing: Theme.spacing * 1.35

            NeonPanel {
                id: contentPanel
                width: parent.width

                Column {
                    id: contentCol
                    width: contentPanel.innerWidth
                    spacing: Theme.spacing

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.round_config_content")
                        color: Theme.primary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                    }

                    GameTextField {
                        width: parent.width
                        fillWidth: true
                        placeholderText: adminViewModel.label("ui.editor.answer")
                        text: adminViewModel.editAnswer
                        onTextEdited: function(t) { adminViewModel.editAnswer = t }
                    }

                    GameTextField {
                        width: parent.width
                        fillWidth: true
                        placeholderText: adminViewModel.label("ui.editor.hint")
                        text: adminViewModel.editHint
                        onTextEdited: function(t) { adminViewModel.editHint = t }
                    }

                    Text {
                        width: parent.width
                        visible: adminViewModel.configUsesTexts
                        text: adminViewModel.label("ui.editor.round_config_texts")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: adminViewModel.configTextSlotCount

                        GameTextField {
                            width: contentCol.width
                            fillWidth: true
                            placeholderText: adminViewModel.cardTextPlaceholder(index)
                            text: adminViewModel.cardTextAt(index)
                            onTextEdited: function(t) { adminViewModel.setCardTextAt(index, t) }
                        }
                    }

                    Text {
                        width: parent.width
                        visible: adminViewModel.configUsesImages
                        text: adminViewModel.isPhotoMaskRound
                              ? adminViewModel.label("ui.editor.photo_tap_hint")
                              : adminViewModel.label("ui.editor.round_config_images")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Flow {
                        width: parent.width
                        spacing: Theme.spacing
                        visible: adminViewModel.configUsesImages

                        Repeater {
                            model: adminViewModel.configImageSlotCount

                            ImageSlotPlaceholder {
                                hasImage: adminViewModel.slotHasImage(index)
                                imageSource: adminViewModel.slotThumbnailUrl(index)
                                selected: adminViewModel.selectedImageSlot === index
                                loading: adminViewModel.imageProcessing
                                         && adminViewModel.selectedImageSlot === index
                                onClicked: {
                                    adminViewModel.selectedImageSlot = index
                                    imageFileDialog.open()
                                }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        visible: adminViewModel.showGamePreview
                        text: adminViewModel.label("ui.editor.game_preview_hint")
                        color: Theme.gold
                        font.pixelSize: Theme.fontSizeCaption
                        font.italic: true
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        width: parent.width
                        height: 280
                        radius: Theme.radius
                        color: Theme.surfaceAlt
                        visible: adminViewModel.hasPreviewImage && adminViewModel.isPhotoMaskRound

                        Image {
                            id: puzzlePreview
                            anchors.fill: parent
                            anchors.margins: Theme.spacing
                            source: adminViewModel.previewImageUrl
                            fillMode: Image.PreserveAspectFit
                            cache: false
                            opacity: adminViewModel.maskProcessing ? 0.88 : 1

                            MaskRegionSelector {
                                id: maskSelector
                                anchors.fill: parent
                                imageItem: puzzlePreview
                                active: adminViewModel.isPhotoMaskRound
                                      && adminViewModel.selectedImageSlot === 0
                                onRegionSelected: function(relX, relY, relW, relH) {
                                    adminViewModel.markMissingRegion(relX, relY, relW, relH)
                                }
                            }

                            MaskContourOverlay {
                                anchors.fill: parent
                                imageItem: puzzlePreview
                                contourPoints: adminViewModel.maskContour
                                visible: adminViewModel.hasMaskContour
                                z: 4
                            }
                        }

                        Image {
                            id: previewSpinner
                            anchors.centerIn: parent
                            width: 48
                            height: 48
                            z: 0
                            visible: adminViewModel.maskProcessing && !maskSelector.pointerLocked
                            source: "qrc:/qml/assets/spinner.svg"
                            fillMode: Image.PreserveAspectFit
                            transformOrigin: Item.Center
                            RotationAnimation on rotation {
                                from: 0
                                to: 360
                                duration: 850
                                loops: Animation.Infinite
                                running: previewSpinner.visible
                            }
                        }
                    }

                    GameButton {
                        width: parent.width
                        fillWidth: true
                        visible: adminViewModel.isPhotoMaskRound
                                 && adminViewModel.hasMaskContour
                        text: adminViewModel.label("ui.editor.clear_mask")
                        primary: false
                        outline: true
                        onClicked: adminViewModel.clearMask()
                    }
                }
            }

            NeonPanel {
                id: optionsPanel
                width: parent.width

                Column {
                    id: optionsCol
                    width: optionsPanel.innerWidth
                    spacing: Theme.spacing

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.round_config_answers")
                        color: Theme.primary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                    }

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.round_config_answers_hint")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: adminViewModel.answerOptionCount

                        GameTextField {
                            width: optionsCol.width
                            fillWidth: true
                            placeholderText: adminViewModel.answerOptionPlaceholder()
                            text: adminViewModel.answerOptionAt(index)
                            onTextEdited: function(t) { adminViewModel.setAnswerOptionAt(index, t) }
                        }
                    }

                    GameButton {
                        width: parent.width
                        fillWidth: true
                        text: adminViewModel.label("ui.editor.add_answer_option")
                        primary: false
                        outline: true
                        enabled: adminViewModel.answerOptionCount < adminViewModel.maxAnswerOptions
                        onClicked: adminViewModel.addAnswerOption()
                    }
                }
            }

            GameButton {
                width: parent.width
                fillWidth: true
                text: adminViewModel.label("ui.editor.save_round_config")
                gold: true
                onClicked: adminViewModel.saveRoundConfig()
            }

            Item {
                width: 1
                height: Theme.spacing
            }
        }
    }
}
