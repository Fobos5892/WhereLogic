import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        width: parent.width * 0.8
        spacing: Theme.spacing * 2

        Text {
            width: parent.width
            text: gameViewModel.totalScoreTeamA === gameViewModel.totalScoreTeamB
                  ? gameViewModel.label("ui.victory.title_tie")
                  : gameViewModel.label("ui.victory.title")
            color: Theme.gold
            font.pixelSize: Theme.fontSizeHero
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: gameViewModel.totalScoreTeamA === gameViewModel.totalScoreTeamB
                  ? gameViewModel.label("ui.victory.tie")
                  : (gameViewModel.totalScoreTeamA > gameViewModel.totalScoreTeamB
                     ? gameViewModel.teamAName
                     : gameViewModel.teamBName)
            color: Theme.primary
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: gameViewModel.label("ui.victory.score_format")
                    .arg(gameViewModel.teamAName)
                    .arg(gameViewModel.totalScoreTeamA)
                    .arg(gameViewModel.teamBName)
                    .arg(gameViewModel.totalScoreTeamB)
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            horizontalAlignment: Text.AlignHCenter
        }

        GameButton {
            width: parent.width
            fillWidth: true
            text: gameViewModel.label("ui.victory.play_again")
            gold: true
            onClicked: gameViewModel.clearSession()
        }
    }
}
