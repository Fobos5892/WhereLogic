import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    readonly property bool roundWasTie: gameViewModel.roundScoreTeamA === gameViewModel.roundScoreTeamB
    readonly property bool teamAWonRound: gameViewModel.roundScoreTeamA > gameViewModel.roundScoreTeamB

    Column {
        anchors.centerIn: parent
        width: parent.width * 0.8
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

        Text {
            width: parent.width
            text: roundWasTie
                  ? gameViewModel.label("ui.inter_round.round_point_tie")
                  : gameViewModel.label("ui.inter_round.round_point_winner")
                        .arg(teamAWonRound ? gameViewModel.teamAName : gameViewModel.teamBName)
            color: Theme.primary
            font.pixelSize: Theme.fontSizeBody
            font.bold: true
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
                    total: gameViewModel.maxRoundStars
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: gameViewModel.label("ui.score.round_points_format")
                            .arg(gameViewModel.totalScoreTeamA)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2014"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeTitle
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
                    total: gameViewModel.maxRoundStars
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: gameViewModel.label("ui.score.round_points_format")
                            .arg(gameViewModel.totalScoreTeamB)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
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
