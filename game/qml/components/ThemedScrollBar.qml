import QtQuick
import QtQuick.Controls.Basic
import ".."

ScrollBar {
    id: bar

    implicitWidth: Math.max(4, Math.round(5 * Theme.scale))
    padding: Theme.spacing * 0.4

    contentItem: Rectangle {
        implicitWidth: bar.implicitWidth
        radius: width / 2
        color: bar.pressed
               ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.72)
               : bar.hovered
                 ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.58)
                 : Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.38)
    }

    background: Rectangle {
        implicitWidth: bar.implicitWidth
        radius: width / 2
        color: Qt.rgba(Theme.textSecondary.r, Theme.textSecondary.g, Theme.textSecondary.b, 0.14)
    }
}
