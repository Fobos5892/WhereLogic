import QtQuick
import ".."

Item {
    implicitWidth: 100
    implicitHeight: 56

    readonly property bool critical: gameViewModel.timerSeconds <= 10 && gameViewModel.timerSeconds > 0

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Theme.radius
        color: critical ? Theme.danger : Theme.surface
        border.color: critical ? Theme.danger : Theme.primary
        border.width: Theme.borderWidth

        SequentialAnimation on opacity {
            running: critical
            loops: Animation.Infinite
            NumberAnimation { to: 0.65; duration: 400 }
            NumberAnimation { to: 1.0; duration: 400 }
        }
    }

    Text {
        anchors.centerIn: parent
        text: gameViewModel.showMilliseconds
              ? gameViewModel.label("ui.timer.ms_format").arg(gameViewModel.timerSeconds)
                    .arg(String(gameViewModel.timerMilliseconds).padStart(2, "0"))
              : formatTime(gameViewModel.timerSeconds)
        color: critical ? "#FFFFFF" : Theme.primary
        font.pixelSize: Theme.fontSizeTitle
        font.bold: true
        font.family: "Consolas, monospace"
    }

    function formatTime(sec) {
        const m = Math.floor(sec / 60)
        const s = sec % 60
        return m > 0 ? (m + ":" + String(s).padStart(2, "0")) : String(s)
    }
}
