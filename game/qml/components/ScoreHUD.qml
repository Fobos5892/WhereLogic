import QtQuick
import QtQuick.Layouts
import ".."

Item {
    implicitHeight: 88

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.primary
        border.width: Theme.borderWidth
        opacity: 0.95
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing

        // Team A
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: gameViewModel.teamAName || gameViewModel.label("ui.score.team_a")
                color: gameViewModel.activeTeam === "Team_A" ? Theme.primary : Theme.textSecondary
                font.bold: gameViewModel.activeTeam === "Team_A"
                font.pixelSize: Theme.fontSizeCaption
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            StarRow {
                filled: gameViewModel.roundScoreTeamA
            }
            Text {
                text: gameViewModel.label("ui.score.total_format").arg(gameViewModel.totalScoreTeamA)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 2

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: gameViewModel.label("ui.score.question")
                color: Theme.textSecondary
                font.pixelSize: 11
                font.letterSpacing: 2
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: gameViewModel.puzzleNumber
                color: Theme.gold
                font.bold: true
                font.pixelSize: Theme.fontSizeTitle
            }
        }

        // Team B
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.alignment: Qt.AlignRight
                text: gameViewModel.teamBName || gameViewModel.label("ui.score.team_b")
                color: gameViewModel.activeTeam === "Team_B" ? Theme.primary : Theme.textSecondary
                font.bold: gameViewModel.activeTeam === "Team_B"
                font.pixelSize: Theme.fontSizeCaption
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideLeft
                Layout.fillWidth: true
            }
            StarRow {
                Layout.alignment: Qt.AlignRight
                filled: gameViewModel.roundScoreTeamB
            }
            Text {
                Layout.alignment: Qt.AlignRight
                text: gameViewModel.label("ui.score.total_format").arg(gameViewModel.totalScoreTeamB)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
        }
    }
}
