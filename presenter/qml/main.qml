import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import WhereLogic.Presenter 1.0
import "."

ApplicationWindow {
    id: root
    width: 480
    height: 854
    visible: true
    title: qsTr("WhereLogic Presenter")
    color: PresenterTheme.background
    contentOrientation: Qt.PortraitOrientation
    flags: Qt.Window | Qt.FramelessWindowHint
    visibility: Window.Windowed

    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        z: 50
        visible: root.visibility === Window.Windowed
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton)
                root.startSystemMove()
        }
    }

    PresenterViewModel {
        id: viewModel
    }

    FullscreenToggle {
        z: 200
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: PresenterTheme.margin
        window: root
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: PresenterTheme.margin
        anchors.topMargin: PresenterTheme.margin + 40
        spacing: PresenterTheme.spacing

        Text {
            Layout.fillWidth: true
            text: qsTr("Пульт ведущего")
            color: PresenterTheme.primary
            font.pixelSize: PresenterTheme.fontSizeTitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: viewModel.connectionState
            color: PresenterTheme.textSecondary
            font.pixelSize: PresenterTheme.fontSizeCaption
            horizontalAlignment: Text.AlignHCenter
        }

        PairingScreen {
            Layout.fillWidth: true
            viewModel: viewModel
            visible: viewModel.connectionState !== "connected"
        }

        DashboardScreen {
            Layout.fillWidth: true
            Layout.fillHeight: true
            viewModel: viewModel
            visible: viewModel.connectionState === "connected"
        }

        AnswerTextPanel {
            Layout.fillWidth: true
            viewModel: viewModel
            visible: viewModel.connectionState === "connected"
        }

        ControlPanel {
            Layout.fillWidth: true
            viewModel: viewModel
            visible: viewModel.connectionState === "connected"
        }
    }
}
