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

            text: gameViewModel.label("ui.victory.title")

            color: Theme.gold

            font.pixelSize: Theme.fontSizeHero

            font.bold: true

            horizontalAlignment: Text.AlignHCenter

        }



        Text {

            Layout.fillWidth: true

            text: gameViewModel.totalScoreTeamA >= gameViewModel.totalScoreTeamB

                  ? gameViewModel.teamAName

                  : gameViewModel.teamBName

            color: Theme.primary

            font.pixelSize: Theme.fontSizeTitle

            font.bold: true

            horizontalAlignment: Text.AlignHCenter

        }



        Text {

            Layout.fillWidth: true

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

            Layout.fillWidth: true

            text: gameViewModel.label("ui.victory.play_again")

            gold: true

            onClicked: gameViewModel.clearSession()

        }

    }

}


