import QtQuick
import ".."
import "../components"
Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 640)
        spacing: Theme.spacing * 2

        Item {
            width: parent.width
            height: titleBlock.height + glowBed.height + Theme.spacing * 0.45

            Item {
                id: titleBlock
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: titleMain.implicitHeight

                Text {
                    id: titleShadow
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: Theme.spacing * 0.25
                    text: gameViewModel.label("ui.welcome.title")
                    color: Qt.rgba(0, 0, 0, 0.55)
                    font.pixelSize: Theme.fontSizeHero
                    font.bold: true
                    font.letterSpacing: 4
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    id: titleGlow
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    text: gameViewModel.label("ui.welcome.title")
                    color: Theme.primary
                    font.pixelSize: Theme.fontSizeHero + 4
                    font.bold: true
                    font.letterSpacing: 4
                    horizontalAlignment: Text.AlignHCenter
                    opacity: 0.22
                    scale: 1.02
                    transformOrigin: Item.Top

                    SequentialAnimation {
                        loops: Animation.Infinite
                        running: true
                        ParallelAnimation {
                            NumberAnimation { target: titleGlow; property: "opacity"; from: 0.14; to: 0.34; duration: 2200; easing.type: Easing.InOutSine }
                            NumberAnimation { target: titleGlow; property: "scale"; from: 1.01; to: 1.04; duration: 2200; easing.type: Easing.InOutSine }
                        }
                        ParallelAnimation {
                            NumberAnimation { target: titleGlow; property: "opacity"; from: 0.34; to: 0.14; duration: 2200; easing.type: Easing.InOutSine }
                            NumberAnimation { target: titleGlow; property: "scale"; from: 1.04; to: 1.01; duration: 2200; easing.type: Easing.InOutSine }
                        }
                    }
                }

                Text {
                    id: titleMain
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    text: gameViewModel.label("ui.welcome.title")
                    color: Theme.primary
                    font.pixelSize: Theme.fontSizeHero
                    font.bold: true
                    font.letterSpacing: 4
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Item {
                id: glowBed
                anchors.top: titleBlock.bottom
                anchors.topMargin: Theme.spacing * 0.55
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width * 0.72, titleMain.implicitWidth * 1.2)
                height: Theme.spacing * 1.6

                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width * 1.12
                    height: parent.height * 2.2
                    radius: height / 2
                    color: Theme.primary
                    opacity: 0.05
                }

                Rectangle {
                    id: glowHalo
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width * 1.02
                    height: parent.height * 1.55
                    radius: height / 2
                    color: Theme.primary
                    opacity: 0.1

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { from: 0.06; to: 0.16; duration: 2600; easing.type: Easing.InOutSine }
                        NumberAnimation { from: 0.16; to: 0.06; duration: 2600; easing.type: Easing.InOutSine }
                    }
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    height: Math.max(2, Theme.borderWidth)
                    radius: height / 2
                    color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.35)
                    opacity: 0.55
                }

                Item {
                    id: rayHost
                    anchors.fill: parent
                    clip: true

                    Repeater {
                        model: 4

                        Rectangle {
                            y: parent.height * 0.12
                            height: parent.height * 0.76
                            width: Math.max(3, glowBed.width * 0.045)
                            radius: width / 2
                            x: -width

                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0; color: "transparent" }
                                GradientStop { position: 0.35; color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.05) }
                                GradientStop { position: 0.5; color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, index % 2 === 0 ? 0.55 : 0.38) }
                                GradientStop { position: 0.65; color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.05) }
                                GradientStop { position: 1; color: "transparent" }
                            }

                            SequentialAnimation on x {
                                loops: Animation.Infinite
                                running: true
                                PauseAnimation { duration: index * 520 }
                                NumberAnimation {
                                    from: -width * 1.5
                                    to: rayHost.width + width * 1.5
                                    duration: 2800 + index * 180
                                    easing.type: Easing.InOutQuad
                                }
                                PauseAnimation { duration: 400 }
                            }

                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: true
                                PauseAnimation { duration: index * 520 + 400 }
                                NumberAnimation { from: 0.45; to: 1; duration: 1400; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 1; to: 0.45; duration: 1400; easing.type: Easing.InOutSine }
                            }
                        }
                    }
                }
            }
        }

        Text {
            width: parent.width
            text: gameViewModel.label("ui.welcome.subtitle")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeBody
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Item { width: 1; height: Theme.spacing }

        GameButton {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.5
            fillWidth: false
            visible: !gameViewModel.hasActiveSession
            text: gameViewModel.label("ui.welcome.start")
            gold: true
            onClicked: {
                if (adminViewModel.selectedPresetId > 0)
                    gameViewModel.startGame(adminViewModel.selectedPresetId)
                else
                    gameViewModel.startGame(1)
            }
        }

        GameButton {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.5
            fillWidth: false
            visible: gameViewModel.hasActiveSession
            text: gameViewModel.label("ui.welcome.continue")
            gold: true
            onClicked: gameViewModel.resumeSession()
        }

        GameButton {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.5
            fillWidth: false
            visible: gameViewModel.hasActiveSession
            text: gameViewModel.label("ui.welcome.new_game")
            primary: false
            outline: true
            onClicked: gameViewModel.clearSession()
        }
    }
}
