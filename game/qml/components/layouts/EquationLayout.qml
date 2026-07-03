import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

RowLayout {
    spacing: Theme.spacing

    Repeater {
        model: 3
        GameCard {
            Layout.fillHeight: true
            Layout.preferredWidth: 140
            faceUp: gameViewModel.cardsFaceUp
            cardIndex: index
            label: String(index + 1)
        }
    }

    Label {
        Layout.alignment: Qt.AlignVCenter
        text: "="
        color: Theme.primary
        font.pixelSize: Theme.fontSizeHero
    }

    GameCard {
        Layout.fillHeight: true
        Layout.preferredWidth: 140
        faceUp: gameViewModel.cardsFaceUp
        label: "?"
    }
}
