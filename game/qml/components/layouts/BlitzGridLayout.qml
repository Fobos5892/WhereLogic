import QtQuick

import QtQuick.Layouts

import "../.."

import ".."



GridLayout {

    id: grid

    anchors.fill: parent

    columns: 4

    rowSpacing: Theme.spacing

    columnSpacing: Theme.spacing



    readonly property real layoutCardWidth: {

        if (grid.width <= 0 || grid.height <= 0)

            return Theme.slotSize * 1.4

        const rows = 2

        const cellW = (grid.width - columnSpacing * (columns - 1)) / columns

        const cellH = (grid.height - rowSpacing * (rows - 1)) / rows

        return Math.max(Theme.slotSize,

                        Math.min(cellW * 0.92, cellH * 0.9 * Theme.cardAspect))

    }



    Repeater {

        model: 8

        GameCard {

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            Layout.preferredWidth: grid.layoutCardWidth

            Layout.preferredHeight: grid.layoutCardWidth / Theme.cardAspect

            Layout.maximumWidth: grid.layoutCardWidth

            Layout.maximumHeight: grid.layoutCardWidth / Theme.cardAspect

            faceUp: gameViewModel.cardsFaceUp

            cardIndex: index

            label: gameViewModel.quoteSlots.length > index ? gameViewModel.quoteSlots[index] : ""

        }

    }

}


