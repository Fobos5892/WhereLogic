import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.75, 520)
        spacing: Theme.spacing * 1.5

        Text {
            width: parent.width
            text: gameViewModel.label("ui.team_setup.title")
            color: Theme.primary
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        GameTextField {
            id: teamAField
            width: parent.width
            fillWidth: true
            placeholderText: gameViewModel.label("ui.team_setup.team_a")
            text: gameViewModel.teamAName
        }

        GameTextField {
            id: teamBField
            width: parent.width
            fillWidth: true
            placeholderText: gameViewModel.label("ui.team_setup.team_b")
            text: gameViewModel.teamBName
        }

        GameButton {
            width: parent.width
            fillWidth: true
            text: gameViewModel.label("ui.team_setup.go")
            gold: true
            onClicked: gameViewModel.configureTeams(teamAField.text, teamBField.text)
        }
    }
}
