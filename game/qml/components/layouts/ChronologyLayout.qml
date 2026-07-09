import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

GridLayout {
    id: grid
    anchors.fill: parent
    columns: 2
    rowSpacing: Theme.spacing * 1.5
    columnSpacing: Theme.spacing * 1.5

    readonly property real layoutCardWidth: {
        if (grid.width <= 0 || grid.height <= 0)
            return Theme.slotSize * 1.6
        const rows = 2
        const cellW = (grid.width - columnSpacing * (columns - 1)) / columns
        const cellH = (grid.height - rowSpacing * (rows - 1)) / rows
        return Math.max(Theme.slotSize * 1.3,
                        Math.min(cellW * 0.92, cellH * 0.9 * Theme.cardAspect))
    }

    Repeater {
        model: 4
        GameCard {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.preferredWidth: grid.layoutCardWidth
            Layout.preferredHeight: grid.layoutCardWidth / Theme.cardAspect
            Layout.maximumWidth: grid.layoutCardWidth
            Layout.maximumHeight: grid.layoutCardWidth / Theme.cardAspect
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: gameViewModel.quoteSlots.length > index
                   ? gameViewModel.quoteSlots[index]
                   : gameViewModel.label("ui.card.chronology_format").arg(index + 1)
        }
    }
}
