import QtQuick
import ".."

Item {
    implicitWidth: Math.round(Theme.chromeSize * 3.8)
    implicitHeight: Math.round(Theme.chromeSize * 1.65)

    readonly property int shownSeconds: gameViewModel.displayTimerSeconds
    readonly property bool grace: gameViewModel.gracePeriodActive
    readonly property bool critical: !grace && shownSeconds <= 10 && shownSeconds > 0

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Theme.radius
        color: grace
               ? Qt.rgba(Theme.gold.r, Theme.gold.g, Theme.gold.b, 0.22)
               : (critical
                  ? Qt.rgba(Theme.danger.r, Theme.danger.g, Theme.danger.b, 0.9)
                  : Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.95))
        border.color: grace ? Theme.gold : (critical ? "#FFD6D6" : Theme.primary)
        border.width: Theme.borderWidth

        SequentialAnimation on opacity {
            running: critical || grace
            loops: Animation.Infinite
            NumberAnimation { to: grace ? 0.75 : 0.65; duration: 400 }
            NumberAnimation { to: 1.0; duration: 400 }
        }
    }

    Text {
        anchors.centerIn: parent
        text: formatTime(shownSeconds)
        color: grace ? Theme.gold : (critical ? "#FFFFFF" : Theme.primary)
        font.pixelSize: Math.max(Theme.fontSizeBody + 2, Theme.fontSizeTitle - 1)
        font.bold: true
        font.family: "Consolas, monospace"
    }

    function formatTime(sec) {
        const normalized = Math.max(0, sec)
        const m = Math.floor(normalized / 60)
        const s = normalized % 60
        return String(m).padStart(2, "0") + ":" + String(s).padStart(2, "0")
    }
}
