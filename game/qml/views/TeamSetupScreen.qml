import QtQuick

import QtQuick.Layouts

import ".."

import "../components"



Item {

    ColumnLayout {

        anchors.centerIn: parent

        width: Math.min(parent.width * 0.75, 520)

        spacing: Theme.spacing * 1.5



        Text {

            Layout.fillWidth: true

            text: gameViewModel.label("ui.team_setup.title")

            color: Theme.primary

            font.pixelSize: Theme.fontSizeTitle

            font.bold: true

            horizontalAlignment: Text.AlignHCenter

        }



        GameTextField {

            id: teamAField

            Layout.fillWidth: true

            placeholderText: gameViewModel.label("ui.team_setup.team_a")

            text: gameViewModel.teamAName

        }



        GameTextField {

            id: teamBField

            Layout.fillWidth: true

            placeholderText: gameViewModel.label("ui.team_setup.team_b")

            text: gameViewModel.teamBName

        }



        GameButton {

            Layout.fillWidth: true

            Layout.topMargin: Theme.spacing

            text: gameViewModel.label("ui.team_setup.go")

            gold: true

            onClicked: gameViewModel.configureTeams(teamAField.text, teamBField.text)

        }

    }

}


