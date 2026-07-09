import QtQuick
import ".."

Row {
    id: row
    property int filled: 0
    property int total: 5
    property int starSize: Theme.iconSm
    spacing: Math.max(1, Math.round(starSize * 0.15))

    Repeater {
        model: total
        Text {
            text: "\u2605"
            font.pixelSize: row.starSize
            color: index < row.filled ? Theme.gold : "#3A4450"
        }
    }
}
