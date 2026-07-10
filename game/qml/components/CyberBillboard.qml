import QtQuick
import ".."

Item {
    id: root

    property int cornerRadius: Theme.radius
    property color glowColor: Theme.primary
    property color panelColor: Theme.surface
    property real panelOpacity: 0.96
    property bool showComet: false
    property bool cometRunning: false
    property string svgSource: ""
    property bool billboardGlow: true

    default property alias children: content.data

    readonly property real r: Math.min(cornerRadius, width * 0.5, height * 0.5)

    clip: true

    Rectangle {
        id: panel
        anchors.fill: parent
        radius: root.r
        color: root.panelColor
        opacity: root.panelOpacity
        clip: true

        Repeater {
            model: root.billboardGlow ? 2 : 0

            Rectangle {
                anchors.fill: parent
                anchors.margins: index * Theme.borderWidth
                radius: Math.max(0, root.r - index * Theme.borderWidth)
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(root.glowColor.r, root.glowColor.g, root.glowColor.b,
                                     0.12 - index * 0.04)
            }
        }

        Image {
            anchors.fill: parent
            visible: root.svgSource.length > 0
            source: root.svgSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            antialiasing: true
            mipmap: true
            asynchronous: true
        }

        Rectangle {
            anchors.fill: parent
            radius: root.r
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(root.glowColor.r, root.glowColor.g, root.glowColor.b, 0.06) }
                GradientStop { position: 0.5; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(root.glowColor.r, root.glowColor.g, root.glowColor.b, 0.04) }
            }
            visible: root.svgSource.length === 0
        }

        Rectangle {
            anchors.fill: parent
            radius: root.r
            color: "transparent"
            border.width: Theme.borderWidth
            border.color: root.glowColor
            visible: root.svgSource.length === 0
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.borderWidth * 1.4
            radius: Math.max(0, root.r - Theme.borderWidth * 1.4)
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(root.glowColor.r, root.glowColor.g, root.glowColor.b, 0.24)
            visible: root.svgSource.length === 0
        }

        Item {
            id: content
            anchors.fill: parent
            clip: true
        }
    }

    CometBorder {
        anchors.fill: parent
        anchors.margins: Theme.borderWidth
        visible: root.showComet && root.cometRunning
        cornerRadius: Math.max(0, root.r - Theme.borderWidth)
        cometColor: Qt.lighter(root.glowColor, 1.25)
        running: root.showComet && root.cometRunning
    }
}
