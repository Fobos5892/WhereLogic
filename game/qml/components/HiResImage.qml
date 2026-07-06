import QtQuick
import ".."

Item {
    id: root

    property alias source: inner.source
    property int fillMode: Image.PreserveAspectFit
    property real resolutionScale: 3
    property bool hiResSmooth: true

    clip: true

    readonly property real innerW: Math.max(1, width * resolutionScale)
    readonly property real innerH: Math.max(1, height * resolutionScale)

    Image {
        id: inner
        anchors.centerIn: parent
        width: root.innerW
        height: root.innerH
        sourceSize: Qt.size(Math.ceil(innerW), Math.ceil(innerH))
        fillMode: root.fillMode
        smooth: root.hiResSmooth
        antialiasing: true
        mipmap: true
        scale: 1 / root.resolutionScale
        transformOrigin: Item.Center
    }
}
