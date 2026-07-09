import QtQuick
import "../.."

Item {
    id: root

    readonly property bool revealReady: gameViewModel.cardsFaceUp
    readonly property bool aggressiveMode: gameViewModel.hybridAnimationStyle === "aggressive"
    readonly property string mergedUrl: gameViewModel.puzzleImageUrl(0)
    readonly property string leftUrl: gameViewModel.puzzleImageUrl(1)
    readonly property string rightUrl: gameViewModel.puzzleImageUrl(2)
    readonly property int holdMs: aggressiveMode ? 120 : 180
    readonly property int splitMs: aggressiveMode ? Theme.animNormal * 2 : Theme.animNormal * 3

    property real splitProgress: 0
    property real mergedBreathScale: 1

    function resetRevealAnimation() {
        revealSequence.stop()
        splitProgress = 0
        mergedBreathScale = 1
    }

    function startRevealAnimation() {
        resetRevealAnimation()
        revealSequence.start()
    }

    function scheduleRevealAnimation() {
        revealArmTimer.restart()
    }

    Timer {
        id: revealArmTimer
        interval: 50
        repeat: false
        onTriggered: {
            if (root.revealReady && root.width > 0 && root.height > 0) {
                root.startRevealAnimation()
            }
        }
    }

    onRevealReadyChanged: {
        if (revealReady) {
            scheduleRevealAnimation()
        } else {
            resetRevealAnimation()
        }
    }

    Component.onCompleted: {
        if (revealReady) {
            scheduleRevealAnimation()
        }
    }

    SequentialAnimation {
        id: revealSequence

        ParallelAnimation {
            SequentialAnimation {
                NumberAnimation {
                    target: root
                    property: "mergedBreathScale"
                    from: 1
                    to: aggressiveMode ? 1.045 : 1.035
                    duration: holdMs * 0.55
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    target: root
                    property: "mergedBreathScale"
                    from: aggressiveMode ? 1.045 : 1.035
                    to: aggressiveMode ? 1.02 : 1.015
                    duration: holdMs * 0.45
                    easing.type: Easing.InOutSine
                }
            }
        }

        NumberAnimation {
            target: root
            property: "splitProgress"
            from: 0
            to: 1
            duration: splitMs
            easing.type: aggressiveMode ? Easing.OutBack : Easing.OutCubic
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.cardBack
        border.color: Theme.secondary
        border.width: Theme.borderWidth
    }

    readonly property real stageCardWidth: {
        if (width <= 0 || height <= 0)
            return Theme.slotSize * 2.4
        const maxByHeight = height * 0.92
        const maxByWidth = (width - Theme.spacing * 3) * 0.44
        return Math.max(Theme.slotSize * 1.8,
                        Math.min(maxByWidth, maxByHeight * Theme.cardAspect))
    }
    readonly property real stageCardHeight: stageCardWidth / Theme.cardAspect

    Item {
        id: stage
        anchors.centerIn: parent
        width: parent.width - Theme.spacing * 2
        height: parent.height - Theme.spacing * 2
        clip: true

        Rectangle {
            readonly property real centerX: (stage.width - width) * 0.5
            width: root.stageCardWidth
            height: root.stageCardHeight
            anchors.verticalCenter: parent.verticalCenter
            radius: Theme.radius
            color: Theme.cardFront
            border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.55)
            border.width: Theme.borderWidth
            opacity: root.splitProgress
            x: centerX * (1 - root.splitProgress)
            clip: true
            z: 1

            Image {
                anchors.fill: parent
                anchors.margins: Theme.spacing * 0.45
                source: root.leftUrl
                fillMode: Image.PreserveAspectFit
                cache: false
            }
        }

        Rectangle {
            readonly property real centerX: (stage.width - width) * 0.5
            width: root.stageCardWidth
            height: root.stageCardHeight
            anchors.verticalCenter: parent.verticalCenter
            x: centerX + centerX * root.splitProgress
            radius: Theme.radius
            color: Theme.cardFront
            border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.55)
            border.width: Theme.borderWidth
            opacity: root.splitProgress
            clip: true
            z: 1

            Image {
                anchors.fill: parent
                anchors.margins: Theme.spacing * 0.45
                source: root.rightUrl
                fillMode: Image.PreserveAspectFit
                cache: false
            }
        }

        Image {
            id: mergedImage
            anchors.centerIn: parent
            width: root.stageCardWidth
            height: root.stageCardHeight
            source: root.mergedUrl
            fillMode: Image.PreserveAspectFit
            cache: false
            opacity: 1 - root.splitProgress
            scale: root.splitProgress > 0
                   ? 1 + (1 - root.splitProgress) * (root.aggressiveMode ? 0.05 : 0.025)
                   : root.mergedBreathScale
            transformOrigin: Item.Center
            z: 2
        }

        Rectangle {
            width: Math.max(2, Theme.borderWidth * 2)
            height: stage.height
            x: (stage.width - width) * 0.5
            radius: width * 0.5
            color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.95)
            opacity: root.splitProgress > 0
                     ? Math.max(0, 1 - root.splitProgress * (root.aggressiveMode ? 1.8 : 1.45))
                     : 0
            visible: root.revealReady
            z: 3
        }

        Rectangle {
            width: stage.width * 0.12
            height: stage.height
            x: (stage.width - width) * 0.5
            color: "transparent"
            opacity: root.splitProgress > 0
                     ? Math.max(0, (root.aggressiveMode ? 0.9 : 0.7) - root.splitProgress)
                     : 0
            visible: root.revealReady
            z: 3

            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.45) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        radius: Theme.radius
        color: Theme.cardBack
        border.color: Theme.secondary
        border.width: Theme.borderWidth
        visible: !root.revealReady

        Text {
            anchors.centerIn: parent
            text: "?"
            color: Theme.primary
            font.pixelSize: Theme.fontSizeHero
            font.bold: true
        }
    }
}
