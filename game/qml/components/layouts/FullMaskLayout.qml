import QtQuick
import ".."
import "../.."

Item {
    GameCard {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.5, 320)
        height: Math.min(parent.height * 0.7, 420)
        faceUp: gameViewModel.cardsFaceUp
        cardIndex: 0
        label: gameViewModel.label("ui.layout.mask")
    }
}
