import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

Flow {
    width: parent.width
    spacing: Theme.spacing

    Repeater {
        model: 4
        GameCard {
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: gameViewModel.quoteSlots.length > index
                   ? gameViewModel.quoteSlots[index]
                   : gameViewModel.label("ui.card.chronology_format").arg(index + 1)
        }
    }
}
