import QtQuick
import ".."

Item {
    id: root

    property alias source: inner.source
    property int fillMode: Image.PreserveAspectFit
    property real resolutionScale: 1
    property bool hiResSmooth: true
    property int maxBufferEdge: 800
    property int maxBufferHeight: 600

    clip: true

    readonly property real effectiveScale: {
        if (width <= 0 || height <= 0)
            return Math.max(1, resolutionScale)

        const wantW = width * resolutionScale
        const wantH = height * resolutionScale
        if (wantW <= maxBufferEdge && wantH <= maxBufferHeight)
            return resolutionScale

        const fit = Math.min(maxBufferEdge / wantW, maxBufferHeight / wantH)
        return Math.max(1, resolutionScale * fit)
    }

    readonly property real innerW: Math.max(1, Math.min(width * effectiveScale, maxBufferEdge))
    readonly property real innerH: Math.max(1, Math.min(height * effectiveScale, maxBufferHeight))
    readonly property real displayScale: width > 0 ? width / innerW : 1

    Image {
        id: inner
        anchors.centerIn: parent
        width: root.innerW
        height: root.innerH
        sourceSize: Qt.size(Math.ceil(root.innerW), Math.ceil(root.innerH))
        fillMode: root.fillMode
        smooth: root.hiResSmooth
        antialiasing: true
        mipmap: true
        asynchronous: true
        scale: root.displayScale
        transformOrigin: Item.Center
    }
}
