import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

GridLayout {
    columns: 2
    rowSpacing: Theme.spacing * 1.5
    columnSpacing: Theme.spacing * 1.5

    readonly property int cardCount: Math.max(3, Math.min(4,
        gameViewModel.quoteSlots.length > 0 ? gameViewModel.quoteSlots.length : 4))

    Repeater {
        model: cardCount
        GameCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 120
            Layout.minimumHeight: 140
            Layout.preferredWidth: 160
            Layout.preferredHeight: 200
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: gameViewModel.quoteSlots.length > index
                   ? gameViewModel.quoteSlots[index]
                   : (gameViewModel.hintText.length > 0
                      ? gameViewModel.hintText
                      : gameViewModel.label("ui.card.image_format").arg(index + 1))
        }
    }
}
