import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

Flow {
    width: parent.width
    spacing: Theme.spacing

    Repeater {
        model: Math.max(gameViewModel.quoteSlots.length, 4)
        GameCard {
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: gameViewModel.quoteSlots.length > index ? gameViewModel.quoteSlots[index] : String(index + 1)
        }
    }
}
