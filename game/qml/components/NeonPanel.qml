import QtQuick
import ".."
import "."

Item {
    id: panel

    property int pad: Math.round(Theme.spacing * 1.35)
    readonly property real innerWidth: Math.max(0, width - pad * 2)

    default property alias children: content.data

    implicitHeight: content.height + pad * 2

    CyberBillboard {
        anchors.fill: parent
        z: 0
        cornerRadius: Theme.radius
        glowColor: Theme.primary
        panelColor: Theme.surface
        panelOpacity: 0.98
        billboardGlow: true
    }

    Item {
        id: content
        z: 1
        x: pad
        y: pad
        width: panel.innerWidth
        height: childrenRect.height
    }
}
