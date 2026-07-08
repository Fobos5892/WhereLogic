import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

RowLayout {
    spacing: Theme.spacing

    GameCard {
        Layout.fillHeight: true
        Layout.preferredWidth: 140
        faceUp: gameViewModel.cardsFaceUp
        cardIndex: 0
        label: gameViewModel.label("ui.layout.equation_operand").arg(1)
    }

    Text {
        Layout.alignment: Qt.AlignVCenter
        text: "+"
        color: Theme.primary
        font.pixelSize: Theme.fontSizeHero
        font.bold: true
    }

    GameCard {
        Layout.fillHeight: true
        Layout.preferredWidth: 140
        faceUp: gameViewModel.cardsFaceUp
        cardIndex: 1
        label: gameViewModel.label("ui.layout.equation_operand").arg(2)
    }

    Text {
        Layout.alignment: Qt.AlignVCenter
        text: "="
        color: Theme.primary
        font.pixelSize: Theme.fontSizeHero
        font.bold: true
    }

    GameCard {
        Layout.fillHeight: true
        Layout.preferredWidth: 140
        faceUp: gameViewModel.cardsFaceUp
        cardIndex: 2
        hideAnswer: true
        answerRevealed: gameViewModel.currentStage === "STAGE_MISSING_REVEAL"
                        || gameViewModel.currentStage === "STAGE_RESOLUTION"
    }
}
