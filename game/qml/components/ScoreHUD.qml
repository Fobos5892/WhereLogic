import QtQuick
import QtQuick.Layouts
import ".."
import "."

Item {
    id: root
    implicitHeight: Math.round(Theme.topBarHeight * 1.1)

    readonly property real hudMargin: Theme.spacing * 1.5
    readonly property real teamPadH: Theme.spacing * 2.2
    readonly property real teamPadV: Theme.spacing * 0.35
    readonly property real teamPanelWidth: Math.round(Theme.w * 0.28)
    readonly property real roundPanelWidth: Math.round(Theme.chromeSize * 2.0)
    readonly property real roundPadV: Theme.spacing * 0.15

    readonly property bool showRoundNumberOnly: {
        if (!gameViewModel)
            return true
        const stage = gameViewModel.currentStage || ""
        const rounds = gameViewModel.totalRounds || 0
        return rounds <= 1
                || stage === "STAGE_ROUND_ENDED"
                || stage === "STAGE_FINAL_VICTORY"
    }

    CyberBillboard {
        anchors.fill: parent
        z: 0
        cornerRadius: Theme.radius
        glowColor: Theme.primary
        panelColor: Theme.surface
        panelOpacity: 0.95
        billboardGlow: true
    }

    RowLayout {
        z: 1
        anchors.fill: parent
        anchors.leftMargin: root.hudMargin
        anchors.rightMargin: root.hudMargin
        anchors.topMargin: Theme.spacing * 0.35
        anchors.bottomMargin: Theme.spacing * 0.35
        spacing: Theme.spacing * 0.55

        CyberBillboard {
            id: leftTeamBox
            Layout.preferredWidth: root.teamPanelWidth
            Layout.maximumWidth: root.teamPanelWidth
            Layout.fillHeight: true
            readonly property bool active: gameViewModel.activeTeam === "Team_A"
            cornerRadius: Theme.radius
            glowColor: active ? Theme.primary : Theme.secondary
            panelColor: active
                        ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.22)
                        : Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.35)
            panelOpacity: 1
            showComet: active
            cometRunning: active
            billboardGlow: active

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.teamPadH
                anchors.rightMargin: root.teamPadH
                anchors.topMargin: root.teamPadV
                anchors.bottomMargin: root.teamPadV
                spacing: Theme.spacing * 0.5

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.spacing * 3
                    text: gameViewModel.teamAName
                    color: leftTeamBox.active ? Qt.lighter(Theme.primary, 1.35) : Theme.textPrimary
                    font.bold: leftTeamBox.active
                    font.pixelSize: Theme.fontSizeBody
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
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }

        Item { Layout.fillWidth: true; Layout.minimumWidth: Theme.spacing }

        CyberBillboard {
            Layout.preferredWidth: root.roundPanelWidth
            Layout.maximumWidth: root.roundPanelWidth
            Layout.fillHeight: true
            cornerRadius: Theme.radius
            glowColor: Theme.secondary
            panelColor: Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.45)
            panelOpacity: 1
            billboardGlow: true

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - Theme.spacing
                anchors.topMargin: root.roundPadV
                anchors.bottomMargin: root.roundPadV
                spacing: Theme.spacing * 0.06

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    text: gameViewModel.label("ui.score.round_label")
                    color: Theme.textPrimary
                    font.pixelSize: Math.max(Theme.borderWidth * 4, Theme.fontSizeCaption - 3)
                    font.letterSpacing: 1
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    text: root.showRoundNumberOnly
                          ? String(gameViewModel.currentRoundNumber || 1)
                          : gameViewModel.label("ui.score.round_progress_format")
                                .arg(gameViewModel.currentRoundNumber || 1)
                                .arg(gameViewModel.totalRounds || 1)
                    color: Theme.gold
                    font.bold: true
                    font.pixelSize: Theme.fontSizeBody - 1
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Item { Layout.fillWidth: true; Layout.minimumWidth: Theme.spacing }

        CyberBillboard {
            id: rightTeamBox
            Layout.preferredWidth: root.teamPanelWidth
            Layout.maximumWidth: root.teamPanelWidth
            Layout.fillHeight: true
            readonly property bool active: gameViewModel.activeTeam === "Team_B"
            cornerRadius: Theme.radius
            glowColor: active ? Theme.primary : Theme.secondary
            panelColor: active
                        ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.22)
                        : Qt.rgba(Theme.surfaceAlt.r, Theme.surfaceAlt.g, Theme.surfaceAlt.b, 0.35)
            panelOpacity: 1
            showComet: active
            cometRunning: active
            billboardGlow: active

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.teamPadH
                anchors.rightMargin: root.teamPadH
                anchors.topMargin: root.teamPadV
                anchors.bottomMargin: root.teamPadV
                spacing: Theme.spacing * 0.5
                layoutDirection: Qt.RightToLeft

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.spacing * 3
                    text: gameViewModel.teamBName
                    color: rightTeamBox.active ? Qt.lighter(Theme.primary, 1.35) : Theme.textPrimary
                    font.bold: rightTeamBox.active
                    font.pixelSize: Theme.fontSizeBody
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
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }
    }
}
