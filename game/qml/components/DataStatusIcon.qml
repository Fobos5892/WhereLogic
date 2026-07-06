import QtQuick
import ".."
import "."

Item {
    id: root

    property int status: 0

    implicitWidth: Theme.iconLg
    implicitHeight: Theme.iconLg

    readonly property string statusSource: {
        if (root.status === 2) {
            return "qrc:/qml/assets/icon-status-done.svg"
        }
        if (root.status === 1) {
            return "qrc:/qml/assets/icon-status-partial.svg"
        }
        return "qrc:/qml/assets/icon-status-empty.svg"
    }

    HiResImage {
        anchors.fill: parent
        source: root.statusSource
        resolutionScale: 3
    }
}
