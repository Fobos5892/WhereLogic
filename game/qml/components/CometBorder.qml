import QtQuick
import ".."

Item {
    id: root

    property bool running: false
    property color cometColor: Qt.lighter(Theme.primary, 1.35)
    property int cometCount: 2
    property int trailLength: 6

    function pointOnBorder(t) {
        const norm = t - Math.floor(t)
        const w = root.width
        const h = root.height
        const seg = norm * 4
        const s = Math.min(3, Math.floor(seg))
        const f = seg - s

        if (s === 0)
            return Qt.point(f * w, 0)
        if (s === 1)
            return Qt.point(w, f * h)
        if (s === 2)
            return Qt.point(w - f * w, h)
        return Qt.point(0, h - f * h)
    }

    Repeater {
        model: root.cometCount

        Item {
            id: comet
            anchors.fill: parent

            property real phase: index / root.cometCount
            property real progress: phase

            SequentialAnimation on progress {
                loops: Animation.Infinite
                running: root.running && root.width > Theme.borderWidth * 4
                        && root.height > Theme.borderWidth * 4
                NumberAnimation {
                    from: comet.phase
                    to: comet.phase + 1.0
                    duration: 2100
                    easing.type: Easing.Linear
                }
            }

            Repeater {
                model: root.trailLength

                Rectangle {
                    readonly property real headSize: Math.max(Theme.borderWidth * 3.2, Theme.iconSm * 0.58)
                    readonly property real tailT: comet.progress - index * 0.013
                    readonly property point pos: root.pointOnBorder(tailT)

                    x: pos.x - width / 2
                    y: pos.y - height / 2
                    width: headSize * (1 - index * 0.1)
                    height: width
                    radius: width / 2
                    color: root.cometColor
                    opacity: Math.max(0, 0.98 - index * 0.16)
                }
            }
        }
    }
}
