import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: control

    property int from: 0
    property int to: 100
    property int value: 50

    implicitHeight: Theme.buttonHeight

    readonly property real _ratio: control.to === control.from
        ? 0
        : (control.value - control.from) / (control.to - control.from)

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Item {
            id: trackHost
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.buttonHeight * 0.55

            Rectangle {
                id: track
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: Theme.borderWidth * 3
                radius: height * 0.5
                color: Theme.surfaceAlt

                Rectangle {
                    width: control._ratio * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: Theme.primary
                }
            }

            Rectangle {
                id: handle
                y: (trackHost.height - height) / 2
                width: Theme.buttonHeight * 0.55
                height: width
                radius: Theme.radius
                color: Theme.gold
                border.color: Theme.primary
                border.width: Theme.borderWidth
                x: control._ratio * Math.max(0, track.width - width)

                MouseArea {
                    anchors.fill: parent
                    drag.target: handle
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: track.width - handle.width

                    onPositionChanged: {
                        if (!pressed) {
                            return
                        }
                        const span = track.width - handle.width
                        if (span <= 0) {
                            return
                        }
                        const ratio = handle.x / span
                        control.value = Math.round(control.from + ratio * (control.to - control.from))
                    }

                    onReleased: {
                        const span = track.width - handle.width
                        if (span <= 0) {
                            return
                        }
                        const ratio = handle.x / span
                        control.value = Math.round(control.from + ratio * (control.to - control.from))
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: function(mouse) {
                    const span = track.width - handle.width
                    if (span <= 0) {
                        return
                    }
                    const clickX = Math.min(Math.max(mouse.x - handle.width / 2, 0), span)
                    handle.x = clickX
                    const ratio = clickX / span
                    control.value = Math.round(control.from + ratio * (control.to - control.from))
                }
            }

            onWidthChanged: {
                const span = track.width - handle.width
                if (span <= 0) {
                    return
                }
                handle.x = control._ratio * span
            }

            Connections {
                target: control
                function onValueChanged() {
                    const span = track.width - handle.width
                    if (span <= 0) {
                        return
                    }
                    handle.x = control._ratio * span
                }
            }
        }

        Text {
            text: control.value + "%"
            color: Theme.primary
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
            Layout.preferredWidth: Theme.fontSizeBody * 3
            horizontalAlignment: Text.AlignRight
        }
    }
}
