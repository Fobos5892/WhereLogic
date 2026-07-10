import QtQuick
import ".."

Item {
    id: root

    property bool running: false
    property color cometColor: Qt.lighter(Theme.primary, 1.35)
    property int cometCount: 2
    property int trailLength: 6
    property int cornerRadius: Theme.radius

    function pointOnBorder(t) {
        const w = Math.max(1, root.width)
        const h = Math.max(1, root.height)
        const r = Math.min(cornerRadius, w * 0.5, h * 0.5)
        const norm = t - Math.floor(t)

        const topLen = Math.max(0, w - 2 * r)
        const sideLen = Math.max(0, h - 2 * r)
        const arcLen = Math.PI * r * 0.5
        const perim = 2 * topLen + 2 * sideLen + 4 * arcLen
        let d = norm * perim

        function corner(cx, cy, startRad, dist) {
            const a = startRad + (dist / arcLen) * (Math.PI / 2)
            return Qt.point(cx + r * Math.cos(a), cy + r * Math.sin(a))
        }

        if (d <= topLen)
            return Qt.point(r + d, 0)
        d -= topLen
        if (d <= arcLen)
            return corner(w - r, r, -Math.PI / 2, d)
        d -= arcLen
        if (d <= sideLen)
            return Qt.point(w, r + d)
        d -= sideLen
        if (d <= arcLen)
            return corner(w - r, h - r, 0, d)
        d -= arcLen
        if (d <= topLen)
            return Qt.point(w - r - d, h)
        d -= topLen
        if (d <= arcLen)
            return corner(r, h - r, Math.PI / 2, d)
        d -= arcLen
        if (d <= sideLen)
            return Qt.point(0, h - r - d)
        d -= sideLen
        return corner(r, r, Math.PI, d)
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
