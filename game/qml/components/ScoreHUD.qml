import QtQuick
import QtQuick.Layouts
import ".."

Item {
    implicitHeight: Math.round(Theme.topBarHeight * 0.9)

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
        anchors.margins: Theme.spacing * 0.35
        spacing: Theme.spacing * 0.5

        Rectangle {
            id: leftTeamBox
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property bool active: gameViewModel.activeTeam === "Team_A"
            radius: Theme.radius * 0.75
            color: active
                   ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.3)
                   : "transparent"
            border.width: active ? Theme.borderWidth * 2 : Theme.borderWidth
            border.color: active
                          ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.75)
                          : Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.35)
            clip: true

            CometBorder {
                anchors.fill: parent
                anchors.margins: Theme.borderWidth
                running: leftTeamBox.active
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacing * 0.35
                spacing: Theme.spacing * 0.35

                Text {
                    Layout.fillWidth: true
                    text: gameViewModel.teamAName
                    color: leftTeamBox.active ? Qt.lighter(Theme.primary, 1.35) : Theme.textSecondary
                    font.bold: leftTeamBox.active
                    font.pixelSize: Theme.fontSizeCaption
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                StarRow {
                    filled: gameViewModel.roundScoreTeamA
                    total: gameViewModel.maxRoundStars
                    starSize: Theme.iconSm
                }
                Text {
                    text: gameViewModel.label("ui.score.round_points_format").arg(gameViewModel.totalScoreTeamA)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: Math.round(Theme.chromeSize * 1.65)
            Layout.fillHeight: true
            radius: Theme.radius * 0.75
            color: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.35)
            border.width: Theme.borderWidth
            border.color: Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.35)

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: gameViewModel.label("ui.score.round_label")
                    color: Theme.textSecondary
                    font.pixelSize: Math.max(Theme.borderWidth * 4, Theme.fontSizeCaption - 2)
                    font.letterSpacing: 1
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: gameViewModel.label("ui.score.round_progress_format")
                            .arg(gameViewModel.currentRoundNumber)
                            .arg(gameViewModel.totalRounds)
                    color: Theme.gold
                    font.bold: true
                    font.pixelSize: Theme.fontSizeBody
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: gameViewModel.label("ui.score.question")
                    color: Theme.textSecondary
                    font.pixelSize: Math.max(Theme.borderWidth * 4, Theme.fontSizeCaption - 2)
                    font.letterSpacing: 1
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: gameViewModel.puzzleNumber
                    color: Theme.primary
                    font.bold: true
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }

        Rectangle {
            id: rightTeamBox
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property bool active: gameViewModel.activeTeam === "Team_B"
            radius: Theme.radius * 0.75
            color: active
                   ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.3)
                   : "transparent"
            border.width: active ? Theme.borderWidth * 2 : Theme.borderWidth
            border.color: active
                          ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.75)
                          : Qt.rgba(Theme.secondary.r, Theme.secondary.g, Theme.secondary.b, 0.35)
            clip: true

            CometBorder {
                anchors.fill: parent
                anchors.margins: Theme.borderWidth
                running: rightTeamBox.active
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacing * 0.35
                spacing: Theme.spacing * 0.35
                layoutDirection: Qt.RightToLeft

                Text {
                    Layout.fillWidth: true
                    text: gameViewModel.teamBName
                    color: rightTeamBox.active ? Qt.lighter(Theme.primary, 1.35) : Theme.textSecondary
                    font.bold: rightTeamBox.active
                    font.pixelSize: Theme.fontSizeCaption
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideLeft
                    maximumLineCount: 1
                }
                StarRow {
                    filled: gameViewModel.roundScoreTeamB
                    total: gameViewModel.maxRoundStars
                    starSize: Theme.iconSm
                }
                Text {
                    text: gameViewModel.label("ui.score.round_points_format").arg(gameViewModel.totalScoreTeamB)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }
    }
}
