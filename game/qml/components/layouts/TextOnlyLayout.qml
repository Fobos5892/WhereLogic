import QtQuick
import QtQuick.Layouts
import ".."
import "../.."

Item {
    ColumnLayout {
        anchors.fill: parent
        width: parent.width
        spacing: Theme.spacing

        Repeater {
            model: gameViewModel.quoteSlots.length > 0
                   ? gameViewModel.quoteSlots.length
                   : 3
            delegate: NeonPanel {
                Layout.fillWidth: true

                Text {
                    width: parent.width
                    implicitHeight: Theme.buttonHeight * 1.4
                    text: gameViewModel.cardsFaceUp && gameViewModel.quoteSlots.length > index
                          ? gameViewModel.quoteSlots[index]
                          : gameViewModel.label("ui.card.quote_format").arg(index + 1)
                    color: gameViewModel.cardsFaceUp ? Theme.gold : Theme.textPrimary
                    font.pixelSize: Theme.fontSizeBody
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
