import QtQuick
import ".."

Item {
    id: root

    required property Item imageItem
    property bool active: true
    property real minDragPixels: 12

    signal regionSelected(real relX, real relY, real relW, real relH)

    property point startPoint
    property point endPoint
    property bool selecting: false
    property bool touchHoldActive: false

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

    function isTouchPointer(mouse) {
        return mouse.pointingDevice
               && mouse.pointingDevice.pointerType === PointerDevice.Touch
    }

    function beginSelection(x, y) {
        startPoint = Qt.point(x, y)
        endPoint = startPoint
        selecting = true
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

    MouseArea {
        id: touchArea
        anchors.fill: parent
        enabled: root.active
        pressAndHoldInterval: 420
        preventStealing: true

        onPressed: function(mouse) {
            if (isTouchPointer(mouse)) {
                touchHoldActive = false
                return
            }
            beginSelection(mouse.x, mouse.y)
        }

        onPressAndHold: function(mouse) {
            touchHoldActive = true
            beginSelection(mouse.x, mouse.y)
        }

        onPositionChanged: function(mouse) {
            if (!selecting) {
                return
            }
            if (isTouchPointer(mouse) && !touchHoldActive) {
                return
            }
            endPoint = Qt.point(mouse.x, mouse.y)
        }

        onReleased: function(mouse) {
            if (!selecting) {
                touchHoldActive = false
                return
            }
            selecting = false
            touchHoldActive = false
            endPoint = Qt.point(mouse.x, mouse.y)
            commitSelection()
        }

        onCanceled: {
            selecting = false
            touchHoldActive = false
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

        Text {
            anchors.bottom: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 4
            visible: touchHoldActive || touchArea.pressed
            text: adminViewModel.label("ui.editor.region_drag_hint")
            color: Theme.gold
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
        }
    }
}
