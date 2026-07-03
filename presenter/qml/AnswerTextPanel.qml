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
        text: qsTr("Answer")
        color: PresenterTheme.secondary
        font.pixelSize: PresenterTheme.fontSizeBody
    }

    Label {
        Layout.fillWidth: true
        text: viewModel.submittedAnswer.length > 0
              ? viewModel.submittedAnswer
              : qsTr("No answer submitted")
        color: PresenterTheme.textPrimary
        font.pixelSize: PresenterTheme.fontSizeBody
        wrapMode: Text.WordWrap
    }

    Label {
        Layout.fillWidth: true
        visible: viewModel.missingRevealText.length > 0
        text: viewModel.missingRevealText
        color: PresenterTheme.warning
        font.pixelSize: PresenterTheme.fontSizeCaption
        wrapMode: Text.WordWrap
    }

    Label {
        Layout.fillWidth: true
        visible: viewModel.correctAnswerText.length > 0
        text: qsTr("Correct: %1").arg(viewModel.correctAnswerText)
        color: PresenterTheme.success
        font.pixelSize: PresenterTheme.fontSizeBody
        wrapMode: Text.WordWrap
    }

    Label {
        Layout.fillWidth: true
        visible: viewModel.submittedAnswer.length > 0
        text: viewModel.answerWasCorrect ? qsTr("Correct!") : qsTr("Incorrect")
        color: viewModel.answerWasCorrect ? PresenterTheme.success : PresenterTheme.accent
        font.pixelSize: PresenterTheme.fontSizeBody
    }
}
