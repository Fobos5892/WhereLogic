import QtQuick
import QtQuick.Layouts
import ".."
import "."
Rectangle {
    anchors.fill: parent
    color: "#DD0B0C10"

    NeonPanel {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 520)
        implicitHeight: col.implicitHeight + Theme.spacing * 2

        ColumnLayout {
            id: col
            anchors.fill: parent
            spacing: Theme.spacing * 1.5

            Text {
                Layout.fillWidth: true
                text: gameViewModel.label("ui.reveal.title")
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: gameViewModel.missingRevealText
                color: Theme.primary
                font.pixelSize: Theme.fontSizeBody
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                radius: Theme.radius
                color: Theme.surfaceAlt
                visible: gameViewModel.layoutType === "FULL_MASK"
                         && gameViewModel.currentPuzzleId > 0

                Image {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing
                    source: gameViewModel.puzzleImageUrl(0)
                    fillMode: Image.PreserveAspectFit
                    cache: false
                }
            }

            Text {
                Layout.fillWidth: true
                visible: gameViewModel.revealedAnswer.length > 0
                text: gameViewModel.revealedAnswer
                color: Theme.gold
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            GameButton {
                Layout.fillWidth: true
                text: gameViewModel.label("ui.reveal.continue")
                gold: true
                onClicked: gameViewModel.finishMissingReveal()
            }
        }
    }
}
