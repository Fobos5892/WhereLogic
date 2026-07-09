import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../components"

Rectangle {
    id: root
    color: "#F00B0C10"
    visible: adminViewModel.roundConfigOpen
    z: 10
    focus: true
    property bool maskPointerLocked: false
    property string formPageState: adminViewModel.isPhotoMaskRound ? "photoMask" : "standard"
    readonly property bool textOnlyRound: adminViewModel.selectedRoundLayoutType === "TEXT_ONLY"
    property bool useAnswerOptionsMode: false
    property int hostHintFieldCount: 0
    property string hintOne: hintPart(0)
    property string hintTwo: hintPart(1)
    property string hintThree: hintPart(2)

    function hintPart(idx) {
        const parts = (adminViewModel.editHint || "").split("||")
        return idx < parts.length ? parts[idx].trim() : ""
    }

    function syncHostHintFieldCount() {
        const raw = (adminViewModel.editHint || "").trim()
        if (raw.length === 0) {
            root.hostHintFieldCount = 0
            return
        }
        const parts = adminViewModel.editHint.split("||")
        let count = Math.min(3, parts.length)
        while (count > 0 && parts[count - 1].trim().length === 0)
            count -= 1
        root.hostHintFieldCount = count
    }

    function saveHintParts() {
        const parts = []
        const first = root.hintOne.trim()
        const second = root.hintTwo.trim()
        const third = root.hintThree.trim()
        if (first.length > 0)
            parts.push(first)
        if (root.hostHintFieldCount >= 2 && second.length > 0)
            parts.push(second)
        if (root.hostHintFieldCount >= 3 && third.length > 0)
            parts.push(third)
        adminViewModel.editHint = parts.join("||")
    }

    function ensureQuestionMark(text) {
        const trimmed = text.trim()
        if (trimmed.length === 0) {
            return trimmed
        }
        if (trimmed.endsWith("?")) {
            return trimmed
        }
        return trimmed + "?"
    }

    Component.onCompleted: syncHostHintFieldCount()

    Connections {
        target: adminViewModel
        function onEditHintChanged() {
            root.hintOne = root.hintPart(0)
            root.hintTwo = root.hintPart(1)
            root.hintThree = root.hintPart(2)
            root.syncHostHintFieldCount()
        }
        function onSelectedPuzzleIdChanged() {
            root.hintOne = root.hintPart(0)
            root.hintTwo = root.hintPart(1)
            root.hintThree = root.hintPart(2)
            root.syncHostHintFieldCount()
            root.useAnswerOptionsMode = !adminViewModel.isPhotoMaskRound
                                        && adminViewModel.answerOptionCount > 0
            root.focus = true
        }
        function onSelectedRoundIdChanged() {
            root.syncHostHintFieldCount()
            root.useAnswerOptionsMode = !adminViewModel.isPhotoMaskRound
                                        && adminViewModel.answerOptionCount > 0
        }
    }

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
        height: headerBlock.height
            + (configStatus.visible ? configStatus.height + Theme.spacing * 0.5 : 0)

        Column {
            id: headerBlock
            width: parent.width
            spacing: Theme.spacing * 0.45

            Item {
                id: headerRow
                width: parent.width
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
                id: configRule
                width: parent.width
                text: adminViewModel.configRoundRule
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                font.italic: true
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }
        }

        Text {
            id: configStatus
            anchors.top: headerBlock.bottom
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
            value: !root.maskPointerLocked
            when: configScroll.contentItem
        }

        EditorAutoSaveScope {
            width: configScroll.availableWidth - configScroll.leftPadding - configScroll.rightPadding
            implicitHeight: configColumn.height
            onSave: adminViewModel.autosaveEditorDraft()

        Column {
            id: configColumn
            width: parent.width
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
                        text: adminViewModel.label("ui.editor.question_picker")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                    }

                    GameComboBox {
                        id: puzzlePicker
                        fillWidth: true
                        model: adminViewModel.puzzles
                        textRole: "title"
                        valueRole: "id"

                        function puzzleAt(index) {
                            var items = adminViewModel.puzzles
                            if (index < 0 || index >= items.length)
                                return null
                            return items[index]
                        }

                        function syncSelection() {
                            var items = adminViewModel.puzzles
                            var idx = -1
                            for (var i = 0; i < items.length; ++i) {
                                if (items[i].id === adminViewModel.selectedPuzzleId) {
                                    idx = i
                                    break
                                }
                            }
                            if (currentIndex !== idx)
                                currentIndex = idx
                        }

                        onActivated: function(index) {
                            var item = puzzleAt(index)
                            if (!item)
                                return
                            if (item.id === adminViewModel.selectedPuzzleId)
                                return
                            root.focus = true
                            adminViewModel.selectPuzzle(item.id)
                        }

                        Connections {
                            target: adminViewModel
                            function onPuzzlesChanged() { puzzlePicker.syncSelection() }
                            function onSelectedPuzzleIdChanged() { puzzlePicker.syncSelection() }
                        }

                        Component.onCompleted: syncSelection()
                    }

                    Loader {
                        id: formPageLoader
                        width: parent.width
                        sourceComponent: standardFormPage
                        onSourceComponentChanged: root.maskPointerLocked = false
                    }

                    states: [
                        State {
                            name: "standard"
                            when: root.formPageState === "standard"
                            PropertyChanges { target: formPageLoader; sourceComponent: standardFormPage }
                        },
                        State {
                            name: "photoMask"
                            when: root.formPageState === "photoMask"
                            PropertyChanges { target: formPageLoader; sourceComponent: photoMaskFormPage }
                        }
                    ]

                    Component {
                        id: standardFormPage
                        Column {
                            width: contentCol.width
                            spacing: Theme.spacing

                            Text {
                                width: parent.width
                                text: adminViewModel.label("ui.editor.round_config_content")
                                color: Theme.primary
                                font.bold: true
                                font.pixelSize: Theme.fontSizeBody
                            }

                            Text {
                                width: parent.width
                                visible: adminViewModel.selectedRoundLayoutType === "SINGLE_HYBRID"
                                text: adminViewModel.label("ui.editor.hybrid_anim_title")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                            }

                            Row {
                                width: parent.width
                                spacing: Theme.spacing * 0.6
                                visible: adminViewModel.selectedRoundLayoutType === "SINGLE_HYBRID"

                                GameButton {
                                    width: (parent.width - parent.spacing) * 0.5
                                    fillWidth: false
                                    text: adminViewModel.label("ui.editor.hybrid_anim_soft")
                                    primary: adminViewModel.hybridAnimationStyle !== "aggressive"
                                    outline: adminViewModel.hybridAnimationStyle === "aggressive"
                                    onClicked: adminViewModel.hybridAnimationStyle = "soft"
                                }

                                GameButton {
                                    width: (parent.width - parent.spacing) * 0.5
                                    fillWidth: false
                                    text: adminViewModel.label("ui.editor.hybrid_anim_aggressive")
                                    primary: adminViewModel.hybridAnimationStyle === "aggressive"
                                    outline: adminViewModel.hybridAnimationStyle !== "aggressive"
                                    onClicked: adminViewModel.hybridAnimationStyle = "aggressive"
                                }
                            }

                            Text {
                                width: parent.width
                                visible: root.textOnlyRound
                                text: adminViewModel.label("ui.editor.question_label")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                            }

                            EditorBoundField {
                                id: firstMainField
                                width: parent.width
                                fillWidth: true
                                visible: root.textOnlyRound
                                reloadKey: adminViewModel.selectedPuzzleId
                                readValue: function() { return adminViewModel.hostHintAt(0) }
                                placeholderText: adminViewModel.label("ui.editor.question_placeholder")
                                onTextEdited: function(t) { adminViewModel.setHostHintAt(0, t) }
                                onFocusLost: {
                                    const normalized = root.ensureQuestionMark(text)
                                    if (normalized !== text)
                                        adminViewModel.setHostHintAt(0, normalized)
                                }
                            }

                            Text {
                                width: parent.width
                                visible: root.textOnlyRound
                                text: adminViewModel.label("ui.editor.answer_label")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                            }

                            EditorBoundField {
                                width: parent.width
                                fillWidth: true
                                visible: root.textOnlyRound
                                reloadKey: adminViewModel.selectedPuzzleId
                                readValue: function() { return adminViewModel.editAnswer }
                                placeholderText: adminViewModel.label("ui.editor.answer")
                                onTextEdited: function(t) { adminViewModel.editAnswer = t }
                            }

                            Text {
                                width: parent.width
                                visible: root.textOnlyRound
                                text: adminViewModel.label("ui.editor.hint_2_label")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                            }

                            EditorBoundField {
                                width: parent.width
                                fillWidth: true
                                visible: root.textOnlyRound
                                reloadKey: adminViewModel.selectedPuzzleId
                                readValue: function() { return adminViewModel.hostHintAt(1) }
                                placeholderText: adminViewModel.label("ui.editor.hint_2_placeholder")
                                onTextEdited: function(t) { adminViewModel.setHostHintAt(1, t) }
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

                                EditorBoundField {
                                    width: contentCol.width
                                    fillWidth: true
                                    reloadKey: adminViewModel.selectedPuzzleId
                                    readValue: function() { return adminViewModel.cardTextAt(index) }
                                    placeholderText: adminViewModel.cardTextPlaceholder(index)
                                    onTextEdited: function(t) { adminViewModel.setCardTextAt(index, t) }
                                    onFocusLost: {
                                        if (!root.textOnlyRound)
                                            return
                                        const normalized = root.ensureQuestionMark(text)
                                        if (normalized !== text)
                                            adminViewModel.setCardTextAt(index, normalized)
                                    }
                                }
                            }

                            Text {
                                width: parent.width
                                visible: adminViewModel.configUsesImages
                                text: adminViewModel.selectedRoundLayoutType === "EQUATION"
                                      ? adminViewModel.label("ui.editor.round_config_equation_images")
                                      : adminViewModel.label("ui.editor.round_config_images")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                                wrapMode: Text.WordWrap
                            }

                            Loader {
                                width: parent.width
                                sourceComponent: imageSlotsSection
                            }

                            Loader {
                                width: parent.width
                                visible: !root.textOnlyRound
                                sourceComponent: hostHintsSection
                            }
                        }
                    }

                    Component {
                        id: photoMaskFormPage
                        Column {
                            width: contentCol.width
                            spacing: Theme.spacing

                            Text {
                                width: parent.width
                                text: adminViewModel.label("ui.editor.round_config_content")
                                color: Theme.primary
                                font.bold: true
                                font.pixelSize: Theme.fontSizeBody
                            }

                            Text {
                                width: parent.width
                                text: adminViewModel.label("ui.editor.mask_list_hint")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                                wrapMode: Text.WordWrap
                            }

                            Repeater {
                                model: adminViewModel.maskEntryCount

                                NeonPanel {
                                    width: parent.width

                                    Item {
                                        width: parent.width - Theme.spacing * 2
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        height: maskTitle.implicitHeight + Theme.spacing * 0.5 + maskAnswerField.height

                                        Text {
                                            id: maskTitle
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            text: adminViewModel.label("ui.editor.mask_item_title").arg(adminViewModel.maskNumberAt(index))
                                            color: Theme.gold
                                            font.bold: true
                                            font.pixelSize: Theme.fontSizeBody
                                        }

                                        TrashButton {
                                            id: removeMaskButton
                                            anchors.right: parent.right
                                            anchors.top: maskTitle.bottom
                                            anchors.topMargin: Theme.spacing * 0.5
                                            onClicked: adminViewModel.removeMaskAt(index)
                                        }

                                        EditorBoundField {
                                            id: maskAnswerField
                                            anchors.left: parent.left
                                            anchors.right: removeMaskButton.left
                                            anchors.rightMargin: Theme.spacing
                                            anchors.verticalCenter: removeMaskButton.verticalCenter
                                            fillWidth: true
                                            reloadKey: adminViewModel.selectedPuzzleId
                                            readValue: function() { return adminViewModel.maskAnswerAt(index) }
                                            placeholderText: adminViewModel.label("ui.editor.mask_answer_placeholder")
                                            onTextEdited: function(t) { adminViewModel.setMaskAnswerAt(index, t) }
                                        }
                                    }
                                }
                            }

                            Text {
                                width: parent.width
                                text: adminViewModel.label("ui.editor.photo_tap_hint")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeCaption
                                wrapMode: Text.WordWrap
                            }

                            Loader {
                                width: parent.width
                                sourceComponent: imageSlotsSection
                            }

                            Loader {
                                width: parent.width
                                sourceComponent: hostHintsSection
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
                                height: parent.width * Theme.cardAspect
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
                                    opacity: adminViewModel.maskProcessing ? 0.88 : 1

                                    MaskRegionSelector {
                                        id: maskSelector
                                        anchors.fill: parent
                                        imageItem: puzzlePreview
                                        active: adminViewModel.selectedImageSlot === 0
                                        onRegionSelected: function(relX, relY, relW, relH) {
                                            adminViewModel.markMissingRegion(relX, relY, relW, relH)
                                        }
                                        onPointerLockedChanged: root.maskPointerLocked = pointerLocked
                                    }

                                    Repeater {
                                        model: adminViewModel.maskEntryCount

                                        Item {
                                            required property int index

                                            MaskContourOverlay {
                                                anchors.fill: parent
                                                imageItem: puzzlePreview
                                                contourPoints: adminViewModel.maskContourAt(index)
                                                visible: adminViewModel.maskContourAt(index).length > 0
                                                z: 4
                                            }
                                        }
                                    }
                                }

                                Image {
                                    id: previewSpinner
                                    anchors.centerIn: parent
                                    width: Theme.iconLg * 1.5
                                    height: width
                                    z: 0
                                    visible: adminViewModel.maskProcessing && !root.maskPointerLocked
                                    source: "qrc:/qml/assets/spinner.svg"
                                    fillMode: Image.PreserveAspectFit
                                    transformOrigin: Item.Center
                                    RotationAnimation on rotation {
                                        from: 0
                                        to: 360
                                        duration: Theme.animNormal * 2
                                        loops: Animation.Infinite
                                        running: previewSpinner.visible
                                    }
                                }
                            }

                            GameButton {
                                width: parent.width
                                fillWidth: true
                                visible: adminViewModel.hasMaskContour
                                text: adminViewModel.label("ui.editor.clear_mask")
                                primary: false
                                outline: true
                                onClicked: adminViewModel.clearMask()
                            }
                        }
                    }
                }
            }

            Loader {
                id: bottomControlsLoader
                width: parent.width
                sourceComponent: bottomControlsSection
            }

            Item {
                width: 1
                height: Theme.spacing
            }
        }
        }
    }

    Component {
        id: bottomControlsSection
        Column {
            width: configScroll.availableWidth - configScroll.leftPadding - configScroll.rightPadding
            spacing: Theme.spacing * 1.35

            NeonPanel {
                id: answerModePanel
                width: parent.width
                visible: !root.textOnlyRound && !adminViewModel.isPhotoMaskRound

                Column {
                    width: answerModePanel.innerWidth
                    spacing: Theme.spacing

                    Text {
                        width: parent.width
                        text: adminViewModel.label("ui.editor.answer_label")
                        color: Theme.primary
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBody
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.spacing * 0.6

                        GameButton {
                            width: (parent.width - parent.spacing) * 0.5
                            fillWidth: false
                            text: adminViewModel.label("ui.editor.mode_single_answer")
                            primary: !root.useAnswerOptionsMode
                            outline: root.useAnswerOptionsMode
                            onClicked: root.useAnswerOptionsMode = false
                        }

                        GameButton {
                            width: (parent.width - parent.spacing) * 0.5
                            fillWidth: false
                            text: adminViewModel.label("ui.editor.mode_options")
                            primary: root.useAnswerOptionsMode
                            outline: !root.useAnswerOptionsMode
                            onClicked: {
                                root.useAnswerOptionsMode = true
                                if (adminViewModel.answerOptionCount === 0) {
                                    adminViewModel.addAnswerOption()
                                }
                            }
                        }
                    }

                    EditorBoundField {
                        width: parent.width
                        fillWidth: true
                        visible: !root.useAnswerOptionsMode
                        reloadKey: adminViewModel.selectedPuzzleId
                        readValue: function() { return adminViewModel.editAnswer }
                        placeholderText: adminViewModel.label("ui.editor.answer")
                        onTextEdited: function(t) { adminViewModel.editAnswer = t }
                    }
                }
            }

            NeonPanel {
                id: optionsPanel
                width: parent.width
                visible: adminViewModel.isPhotoMaskRound || root.useAnswerOptionsMode

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
                        text: adminViewModel.isPhotoMaskRound
                              ? adminViewModel.label("ui.editor.round_config_answers_grouped_hint")
                              : adminViewModel.label("ui.editor.round_config_answers_hint")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: adminViewModel.answerOptionCount

                        RowLayout {
                            width: optionsCol.width
                            spacing: Theme.spacing

                            EditorBoundField {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                readOnly: adminViewModel.isPhotoMaskRound && adminViewModel.masksAutoGroupAnswers
                                reloadKey: adminViewModel.isPhotoMaskRound && adminViewModel.masksAutoGroupAnswers
                                           ? adminViewModel.selectedPuzzleId * 1000 + adminViewModel.maskEntryCount
                                           : adminViewModel.selectedPuzzleId
                                readValue: function() { return adminViewModel.answerOptionAt(index) }
                                placeholderText: adminViewModel.answerOptionPlaceholder()
                                onTextEdited: function(t) {
                                    if (!adminViewModel.isPhotoMaskRound || !adminViewModel.masksAutoGroupAnswers) {
                                        adminViewModel.setAnswerOptionAt(index, t)
                                    }
                                }
                            }

                            TrashButton {
                                Layout.preferredWidth: Theme.chromeSize
                                Layout.preferredHeight: Theme.chromeSize
                                visible: !adminViewModel.isPhotoMaskRound || !adminViewModel.masksAutoGroupAnswers
                                onClicked: {
                                    adminViewModel.removeAnswerOptionAt(index)
                                    if (adminViewModel.answerOptionCount === 0)
                                        root.useAnswerOptionsMode = false
                                }
                            }
                        }
                    }

                    GameButton {
                        width: parent.width
                        fillWidth: true
                        visible: !adminViewModel.isPhotoMaskRound || !adminViewModel.masksAutoGroupAnswers
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
                text: adminViewModel.puzzleSaving
                      ? "Сохранение..."
                      : adminViewModel.label("ui.editor.save_round_config")
                gold: true
                enabled: !adminViewModel.puzzleSaving
                onClicked: adminViewModel.saveRoundConfig()
            }
        }
    }

    Component {
        id: hostHintsSection
        Column {
            width: contentCol.width
            spacing: Theme.spacing * 0.65

            Text {
                width: parent.width
                text: adminViewModel.label("ui.editor.host_hints_title")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                font.bold: true
            }

            Text {
                width: parent.width
                text: adminViewModel.label("ui.editor.host_hints_hint")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.hostHintFieldCount

                RowLayout {
                    width: contentCol.width
                    spacing: Theme.spacing

                    EditorBoundField {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        reloadKey: adminViewModel.selectedPuzzleId
                        readValue: function() { return adminViewModel.hostHintAt(index) }
                        placeholderText: adminViewModel.label("ui.editor.host_hint_format").arg(index + 1)
                        onTextEdited: function(t) { adminViewModel.setHostHintAt(index, t) }
                    }

                    TrashButton {
                        Layout.preferredWidth: Theme.chromeSize
                        Layout.preferredHeight: Theme.chromeSize
                        onClicked: {
                            const parts = (adminViewModel.editHint || "").split("||")
                            if (index < parts.length) {
                                adminViewModel.removeHostHintAt(index)
                                root.syncHostHintFieldCount()
                            } else if (root.hostHintFieldCount > 0) {
                                root.hostHintFieldCount -= 1
                            }
                        }
                    }
                }
            }

            Row {
                spacing: Theme.spacing
                visible: root.hostHintFieldCount < 3

                IconButton {
                    iconSource: "qrc:/qml/assets/icon-add.svg"
                    onClicked: {
                        if (root.hostHintFieldCount < 3)
                            root.hostHintFieldCount += 1
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: adminViewModel.label("ui.editor.add_host_hint")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }
    }

    Component {
        id: imageSlotsSection
        Item {
            width: contentCol.width
            implicitHeight: slotsGrid.height

            GridLayout {
                id: slotsGrid
                anchors.horizontalCenter: parent.horizontalCenter
                columns: Math.min(4, Math.max(1, adminViewModel.configImageSlotCount))
                rowSpacing: Theme.spacing
                columnSpacing: Theme.spacing
                visible: adminViewModel.configUsesImages

                Repeater {
                    model: adminViewModel.configImageSlotCount

                    Column {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: Theme.spacing * 0.35

                        Text {
                            width: Theme.slotSize
                            text: adminViewModel.imageSlotLabel(index)
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeCaption
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        ImageSlotPlaceholder {
                            readonly property int slotIndex: index
                            hasImage: {
                                adminViewModel.slotImageRevision
                                return adminViewModel.slotHasImage(slotIndex)
                            }
                            imageSource: {
                                adminViewModel.slotImageRevision
                                adminViewModel.previewImageUrl
                                adminViewModel.puzzleImageUrl
                                return adminViewModel.slotThumbnailUrl(slotIndex)
                            }
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
            }
        }
    }
}
