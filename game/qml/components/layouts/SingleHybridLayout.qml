import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

RowLayout {
    spacing: Theme.spacing

    GameCard {
        Layout.fillHeight: true
        Layout.preferredWidth: 180
        faceUp: gameViewModel.cardsFaceUp
        label: gameViewModel.label("ui.layout.text")
    }

    GameCard {
        Layout.fillHeight: true
        Layout.fillWidth: true
        faceUp: gameViewModel.cardsFaceUp
        label: gameViewModel.label("ui.layout.image")
    }
}
