import QtQuick
import ".."

Rectangle {
    id: panel

    property alias content: contentItem.data
    default property alias children: contentItem.data

    color: Theme.surface
    radius: Theme.radius
    border.width: Theme.borderWidth
    border.color: Theme.primary
    opacity: 0.96

    Item {
        id: contentItem
        anchors.fill: parent
        anchors.margins: Theme.spacing
    }
}
