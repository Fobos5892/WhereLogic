import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 640)
        spacing: Theme.spacing * 2

        Text {
            width: parent.width
            text: gameViewModel.label("ui.inter_round.completed")
            color: Theme.gold
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            font.letterSpacing: 2
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: gameViewModel.roundTitle
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.spacing * 3

            Column {
                spacing: Theme.spacing
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: gameViewModel.teamAName
                    color: Theme.primary
                    font.bold: true
                }
                StarRow {
                    anchors.horizontalCenter: parent.horizontalCenter
                    filled: gameViewModel.roundScoreTeamA
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2014"
                color: Theme.textSecondary
                font.pixelSize: 28
            }

            Column {
                spacing: Theme.spacing
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: gameViewModel.teamBName
                    color: Theme.primary
                    font.bold: true
                }
                StarRow {
                    anchors.horizontalCenter: parent.horizontalCenter
                    filled: gameViewModel.roundScoreTeamB
                }
            }
        }

        GameButton {
            width: parent.width
            fillWidth: true
            text: gameViewModel.label("ui.inter_round.next")
            gold: true
            onClicked: gameViewModel.advanceAfterRound()
        }
    }
}
