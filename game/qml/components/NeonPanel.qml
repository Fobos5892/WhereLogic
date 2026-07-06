import QtQuick
import ".."

Rectangle {
    id: panel

    property int pad: Math.round(Theme.spacing * 1.35)
    readonly property real innerWidth: Math.max(0, width - pad * 2)

    default property alias children: content.data

    color: Theme.surface
    radius: Theme.radius
    border.width: Theme.borderWidth
    border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.72)
    opacity: 0.98

    Rectangle {
        anchors.fill: parent
        anchors.margins: Theme.borderWidth
        radius: Math.max(1, Theme.radius - Theme.borderWidth)
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(Theme.textSecondary.r, Theme.textSecondary.g, Theme.textSecondary.b, 0.15)
    }

    implicitHeight: content.height + pad * 2

    Item {
        id: content
        x: pad
        y: pad
        width: panel.innerWidth
        height: childrenRect.height
    }
}
