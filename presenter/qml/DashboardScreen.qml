import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ColumnLayout {
    id: root
    required property var viewModel
    spacing: PresenterTheme.spacing

    Label {
        Layout.fillWidth: true
        text: viewModel.roundTitle.length > 0 ? viewModel.roundTitle : qsTr("Dashboard")
        color: PresenterTheme.primary
        font.pixelSize: PresenterTheme.fontSizeTitle
        wrapMode: Text.WordWrap
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        rowSpacing: PresenterTheme.spacing
        columnSpacing: PresenterTheme.spacing

        Label { text: qsTr("Puzzle #"); color: PresenterTheme.textSecondary }
        Label { text: viewModel.puzzleNum; color: PresenterTheme.textPrimary }

        Label { text: qsTr("Active team"); color: PresenterTheme.textSecondary }
        Label { text: viewModel.activeTeam || qsTr("—"); color: PresenterTheme.textPrimary }

        Label { text: qsTr("Stage"); color: PresenterTheme.textSecondary }
        Label { text: viewModel.gameStage || qsTr("—"); color: PresenterTheme.textPrimary }

        Label { text: qsTr("Layout"); color: PresenterTheme.textSecondary }
        Label { text: viewModel.layoutType || qsTr("—"); color: PresenterTheme.textPrimary }
    }

    Label {
        Layout.fillWidth: true
        text: viewModel.hintText.length > 0 ? viewModel.hintText : qsTr("No hint")
        color: PresenterTheme.textPrimary
        font.pixelSize: PresenterTheme.fontSizeBody
        wrapMode: Text.WordWrap
    }
}
