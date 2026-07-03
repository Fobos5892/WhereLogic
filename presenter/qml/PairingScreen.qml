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
        text: qsTr("Pair with server")
        color: PresenterTheme.textPrimary
        font.pixelSize: PresenterTheme.fontSizeBody
    }

    TextField {
        Layout.fillWidth: true
        placeholderText: qsTr("http://192.168.0.1:8765")
        text: viewModel.serverHost
        onTextChanged: viewModel.serverHost = text
        color: PresenterTheme.textPrimary
        placeholderTextColor: PresenterTheme.textSecondary
        background: Rectangle {
            color: PresenterTheme.surface
            radius: 4
        }
    }

    TextField {
        Layout.fillWidth: true
        placeholderText: qsTr("PIN")
        text: viewModel.pin
        echoMode: TextInput.Password
        onTextChanged: viewModel.pin = text
        color: PresenterTheme.textPrimary
        placeholderTextColor: PresenterTheme.textSecondary
        background: Rectangle {
            color: PresenterTheme.surface
            radius: 4
        }
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Connect")
        enabled: viewModel.serverHost.length > 0 && viewModel.pin.length > 0
                 && viewModel.connectionState !== "connecting"
        onClicked: viewModel.connectToServer()
        contentItem: Text {
            text: parent.text
            color: PresenterTheme.background
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: parent.enabled ? PresenterTheme.primary : PresenterTheme.secondary
            radius: 4
        }
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Disconnect")
        visible: viewModel.connectionState === "connected"
                 || viewModel.connectionState === "error"
        onClicked: viewModel.disconnectFromServer()
        contentItem: Text {
            text: parent.text
            color: PresenterTheme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: PresenterTheme.surface
            radius: 4
        }
    }
}
