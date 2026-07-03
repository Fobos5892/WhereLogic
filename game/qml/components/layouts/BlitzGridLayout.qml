import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

GridLayout {
    columns: 4
    rowSpacing: Theme.spacing
    columnSpacing: Theme.spacing

    Repeater {
        model: 8
        GameCard {
            width: 96
            height: 128
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: gameViewModel.quoteSlots.length > index ? gameViewModel.quoteSlots[index] : ""
        }
    }
}
