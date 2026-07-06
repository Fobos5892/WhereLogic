import QtQuick
import ".."
import "."

Rectangle {
    anchors.fill: parent
    color: "#DD0B0C10"

    NeonPanel {
        id: revealPanel
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 520)

        Column {
            width: revealPanel.innerWidth
            spacing: Theme.spacing * 1.5

            Text {
                width: parent.width
                text: gameViewModel.label("ui.reveal.title")
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                width: parent.width
                text: gameViewModel.missingRevealText
                color: Theme.primary
                font.pixelSize: Theme.fontSizeBody
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Rectangle {
                width: parent.width
                height: 220
                radius: Theme.radius
                color: Theme.surfaceAlt
                visible: gameViewModel.layoutType === "FULL_MASK"
                         && gameViewModel.currentPuzzleId > 0

                Image {
                    id: revealImage
                    anchors.fill: parent
                    anchors.margins: Theme.spacing
                    source: gameViewModel.puzzleImageUrl(0)
                    fillMode: Image.PreserveAspectFit
                    cache: false
                    opacity: status === Image.Loading ? 0.35 : 1
                }

                Image {
                    anchors.centerIn: parent
                    width: 44
                    height: 44
                    visible: revealImage.status === Image.Loading
                    source: "qrc:/qml/assets/spinner.svg"
                    fillMode: Image.PreserveAspectFit
                    transformOrigin: Item.Center
                    RotationAnimation on rotation {
                        from: 0
                        to: 360
                        duration: 850
                        loops: Animation.Infinite
                        running: revealImage.status === Image.Loading
                    }
                }
            }

            Text {
                width: parent.width
                visible: gameViewModel.revealedAnswer.length > 0
                text: gameViewModel.revealedAnswer
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            GameButton {
                width: parent.width
                fillWidth: true
                text: gameViewModel.label("ui.reveal.continue")
                gold: true
                onClicked: gameViewModel.finishMissingReveal()
            }
        }
    }
}
