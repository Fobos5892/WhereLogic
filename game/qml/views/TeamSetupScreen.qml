import QtQuick
import ".."
import "../components"

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        width: parent.width * 0.75
        spacing: Theme.spacing * 1.5

        Text {
            width: parent.width
            text: gameViewModel.label("ui.team_setup.title")
            color: Theme.primary
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        EditorBoundField {
            id: teamAField
            width: parent.width
            fillWidth: true
            reloadKey: gameViewModel.currentPresetId
            readValue: function() { return gameViewModel.teamAName }
            placeholderText: gameViewModel.label("ui.team_setup.team_a")
        }

        EditorBoundField {
            id: teamBField
            width: parent.width
            fillWidth: true
            reloadKey: gameViewModel.currentPresetId
            readValue: function() { return gameViewModel.teamBName }
            placeholderText: gameViewModel.label("ui.team_setup.team_b")
        }

        GameButton {
            width: parent.width
            fillWidth: true
            text: gameViewModel.label("ui.team_setup.go")
            gold: true
            onClicked: {
                teamAField.commitInput()
                teamBField.commitInput()
                teamAField.focus = false
                teamBField.focus = false
                gameViewModel.configureTeams(teamAField.text, teamBField.text)
            }
        }
    }
}
