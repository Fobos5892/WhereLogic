import QtQuick
import ".."

Item {
    id: root

    required property Item imageItem
    property bool active: true
    property real minDragPixels: 12
    property bool selecting: false
    readonly property bool pointerLocked: selecting || regionDrag.active

    signal regionSelected(real relX, real relY, real relW, real relH)

    property point startPoint
    property point endPoint
    property bool hasPendingRect: false
    property real pendingX: 0
    property real pendingY: 0
    property real pendingW: 0
    property real pendingH: 0

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

    function beginSelection(x, y) {
        hasPendingRect = false
        startPoint = Qt.point(x, y)
        endPoint = startPoint
        selecting = true
    }

    function endSelection(x, y) {
        if (!selecting) {
            return
        }
        endPoint = Qt.point(x, y)
        selecting = false
        commitSelection()
    }

    function storePendingRect(x1, y1, w, h) {
        pendingX = x1
        pendingY = y1
        pendingW = w
        pendingH = h
        hasPendingRect = w > 2 || h > 2
    }

    function commitSelection() {
        const layout = imageLayout()
        if (!layout) {
            return
        }

        const x1 = Math.min(startPoint.x, endPoint.x)
        const y1 = Math.min(startPoint.y, endPoint.y)
        const x2 = Math.max(startPoint.x, endPoint.x)
        const y2 = Math.max(startPoint.y, endPoint.y)

        const dragW = x2 - x1
        const dragH = y2 - y1
        storePendingRect(x1, y1, dragW, dragH)

        const nx1 = (x1 - layout.left) / layout.pw
        const ny1 = (y1 - layout.top) / layout.ph
        const nx2 = (x2 - layout.left) / layout.pw
        const ny2 = (y2 - layout.top) / layout.ph

        if (dragW < root.minDragPixels && dragH < root.minDragPixels) {
            const cx = (x1 + x2) * 0.5
            const cy = (y1 + y2) * 0.5
            const rx = (cx - layout.left) / layout.pw
            const ry = (cy - layout.top) / layout.ph
            if (rx >= 0 && rx <= 1 && ry >= 0 && ry <= 1) {
                root.regionSelected(rx, ry, 0, 0)
            }
            return
        }

        const relX = Math.max(0, Math.min(1, nx1))
        const relY = Math.max(0, Math.min(1, ny1))
        const relW = Math.max(0, Math.min(1 - relX, nx2 - nx1))
        const relH = Math.max(0, Math.min(1 - relY, ny2 - ny1))

        if (relW > 0.005 && relH > 0.005) {
            root.regionSelected(relX, relY, relW, relH)
        }
    }

    Connections {
        target: adminViewModel
        function onMasksChanged() {
            if (adminViewModel.maskEntryCount > 0 && !root.selecting) {
                hasPendingRect = false
            }
        }
        function onMaskProcessingChanged() {
            if (!adminViewModel.maskProcessing && !adminViewModel.hasMaskContour && !root.selecting) {
                hasPendingRect = false
            }
        }
    }

    DragHandler {
        id: regionDrag
        enabled: root.active
        target: null
        dragThreshold: 0
        grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType

        onActiveChanged: {
            const p = centroid.position
            if (active) {
                beginSelection(p.x, p.y)
            } else if (selecting) {
                endSelection(p.x, p.y)
            }
        }

        onCentroidChanged: {
            if (!active || !selecting) {
                return
            }
            endPoint = Qt.point(centroid.position.x, centroid.position.y)
        }
    }

    Rectangle {
        visible: selecting
                 && (Math.abs(endPoint.x - startPoint.x) > 2
                     || Math.abs(endPoint.y - startPoint.y) > 2)
        x: Math.min(startPoint.x, endPoint.x)
        y: Math.min(startPoint.y, endPoint.y)
        width: Math.abs(endPoint.x - startPoint.x)
        height: Math.abs(endPoint.y - startPoint.y)
        radius: 3
        color: "#33FFD700"
        border.color: Theme.gold
        border.width: 2
        z: 3

        Text {
            anchors.bottom: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 4
            visible: regionDrag.active
            text: adminViewModel.label("ui.editor.region_drag_hint")
            color: Theme.gold
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
        }
    }

    Rectangle {
        visible: !selecting && hasPendingRect && adminViewModel.maskProcessing
        x: pendingX
        y: pendingY
        width: pendingW
        height: pendingH
        radius: 3
        color: "#22FFFFFF"
        border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.85)
        border.width: 2
        z: 3
    }
}
