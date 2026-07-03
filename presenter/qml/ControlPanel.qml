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
        text: qsTr("Controls")
        color: PresenterTheme.secondary
        font.pixelSize: PresenterTheme.fontSizeBody
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        rowSpacing: PresenterTheme.spacing
        columnSpacing: PresenterTheme.spacing

        Button {
            Layout.fillWidth: true
            text: qsTr("Ready")
            onClicked: viewModel.ready()
            contentItem: Text {
                text: parent.text
                color: PresenterTheme.background
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle { color: PresenterTheme.primary; radius: 4 }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Transfer turn")
            onClicked: viewModel.transferTurn()
            contentItem: Text {
                text: parent.text
                color: PresenterTheme.background
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle { color: PresenterTheme.primary; radius: 4 }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Resolve A")
            onClicked: viewModel.resolveA()
            contentItem: Text {
                text: parent.text
                color: PresenterTheme.background
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle { color: PresenterTheme.secondary; radius: 4 }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Resolve B")
            onClicked: viewModel.resolveB()
            contentItem: Text {
                text: parent.text
                color: PresenterTheme.background
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle { color: PresenterTheme.secondary; radius: 4 }
        }

        Button {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: qsTr("Reject all")
            onClicked: viewModel.rejectAll()
            contentItem: Text {
                text: parent.text
                color: PresenterTheme.textPrimary
                horizontalAlignment: Text.AlignHCenter
            }
            background: Rectangle { color: PresenterTheme.surface; radius: 4 }
        }
    }

    TextField {
        id: answerField
        Layout.fillWidth: true
        placeholderText: qsTr("Type answer…")
        color: PresenterTheme.textPrimary
        placeholderTextColor: PresenterTheme.textSecondary
        background: Rectangle {
            color: PresenterTheme.surface
            radius: 4
        }
    }

    Button {
        Layout.fillWidth: true
        text: qsTr("Submit answer")
        enabled: answerField.text.length > 0
        onClicked: {
            viewModel.submitAnswer(answerField.text)
            answerField.text = ""
        }
        contentItem: Text {
            text: parent.text
            color: PresenterTheme.background
            horizontalAlignment: Text.AlignHCenter
        }
        background: Rectangle {
            color: parent.enabled ? PresenterTheme.success : PresenterTheme.surface
            radius: 4
        }
    }
}
