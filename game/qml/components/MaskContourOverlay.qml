import QtQuick
import QtQuick.Shapes
import ".."

Item {
    id: root

    required property Item imageItem
    property string contourPoints: ""

    function imageLayout() {
        if (!imageItem || imageItem.paintedWidth <= 0 || imageItem.paintedHeight <= 0) {
            return null
        }
        return {
            pw: imageItem.paintedWidth,
            ph: imageItem.paintedHeight,
            left: (width - imageItem.paintedWidth) / 2,
            top: (height - imageItem.paintedHeight) / 2
        }
    }

    function contourPath() {
        const layout = imageLayout()
        if (!layout || !contourPoints || contourPoints.length === 0) {
            return ""
        }

        const pairs = contourPoints.split(";")
        if (pairs.length < 3) {
            return ""
        }

        let path = ""
        for (let i = 0; i < pairs.length; ++i) {
            const coords = pairs[i].split(",")
            if (coords.length !== 2) {
                continue
            }
            const x = layout.left + Number(coords[0]) * layout.pw
            const y = layout.top + Number(coords[1]) * layout.ph
            path += (i === 0 ? "M " : " L ") + x + " " + y
        }
        return path.length > 0 ? path + " Z" : ""
    }

    Shape {
        anchors.fill: parent
        visible: root.contourPath().length > 0
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.gold
            strokeWidth: 2
            fillColor: "transparent"
            strokeStyle: ShapePath.DashLine
            dashPattern: [7, 5]
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg {
                path: root.contourPath()
            }
        }
    }
}
