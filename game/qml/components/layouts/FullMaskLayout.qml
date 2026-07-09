import QtQuick
import ".."
import "../.."

Item {
    GameCard {
        anchors.centerIn: parent
        width: parent.width * 0.72
        height: parent.height * 0.62
        faceUp: gameViewModel.cardsFaceUp
        cardIndex: 0
        label: gameViewModel.label("ui.layout.mask")
    }
}
