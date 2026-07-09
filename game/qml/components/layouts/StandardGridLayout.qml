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



    readonly property int cardCount: Math.max(3, Math.min(4,

        gameViewModel.quoteSlots.length > 0 ? gameViewModel.quoteSlots.length : 4))



    readonly property real layoutCardWidth: {

        if (grid.width <= 0 || grid.height <= 0)

            return Theme.slotSize * 3.6

        const rows = Math.ceil(cardCount / columns)

        const cellW = (grid.width - columnSpacing * (columns - 1)) / columns

        const cellH = (grid.height - rowSpacing * Math.max(0, rows - 1)) / rows

        return Math.max(Theme.slotSize * 2.8,

                        Math.min(cellW * 0.96, cellH * 0.94 * Theme.cardAspect))

    }



    Repeater {

        model: cardCount

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

                   : gameViewModel.label("ui.card.image_format").arg(index + 1)

        }

    }

}


