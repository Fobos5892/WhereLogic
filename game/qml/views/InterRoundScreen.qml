import QtQuick
import QtQuick.Layouts
import ".."
import "../components"

Item {
    ColumnLayout {
        anchors.centerIn: parent
        spacing: Theme.spacing * 2
        width: Math.min(parent.width * 0.8, 640)

        Text {
            Layout.fillWidth: true
            text: gameViewModel.label("ui.inter_round.completed")
            color: Theme.gold
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            font.letterSpacing: 2
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: gameViewModel.roundTitle
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacing * 3

            ColumnLayout {
                Text { text: gameViewModel.teamAName; color: Theme.primary; font.bold: true }
                StarRow { filled: gameViewModel.roundScoreTeamA }
            }
            Text { text: "—"; color: Theme.textSecondary; font.pixelSize: 28 }
            ColumnLayout {
                Text { text: gameViewModel.teamBName; color: Theme.primary; font.bold: true }
                StarRow { filled: gameViewModel.roundScoreTeamB }
            }
        }

        GameButton {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacing
            text: gameViewModel.label("ui.inter_round.next")
            gold: true
            onClicked: gameViewModel.advanceAfterRound()
        }
    }
}
