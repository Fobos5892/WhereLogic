import QtQuick
import ".."

Row {
    id: row
    property int filled: 0
    property int total: 5
    spacing: 4

    Repeater {
        model: total
        Text {
            text: "\u2605"
            font.pixelSize: 22
            color: index < row.filled ? Theme.gold : "#3A4450"
        }
    }
}
