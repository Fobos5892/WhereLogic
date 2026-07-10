import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: control

    property int from: 1
    property int to: 10
    property int value: 4

    readonly property int stepCount: Math.max(1, control.to - control.from + 1)
    readonly property real handleSize: Theme.buttonHeight * 0.5
    readonly property real _ratio: control.stepCount <= 1
        ? 0
        : (control.value - control.from) / (control.to - control.from)

    implicitHeight: column.implicitHeight

    function snapValue(raw) {
        return Math.round(Math.min(control.to, Math.max(control.from, raw)))
    }

    function handleXForValue(stepValue) {
        const span = Math.max(0, trackRow.width - control.handleSize)
        if (span <= 0 || control.stepCount <= 1) {
            return 0
        }
        const ratio = (stepValue - control.from) / (control.to - control.from)
        return ratio * span
    }

    function valueFromHandleX(x) {
        const span = Math.max(0, trackRow.width - control.handleSize)
        if (span <= 0) {
            return control.from
        }
        const ratio = Math.min(1, Math.max(0, x / span))
        return control.snapValue(control.from + ratio * (control.to - control.from))
    }

    Column {
        id: column
        width: parent.width
        spacing: Theme.spacing * 0.35

        RowLayout {
            width: parent.width
            spacing: Theme.spacing

            Text {
                Layout.fillWidth: true
                text: adminViewModel.label("ui.editor.mask_precision_scale_soft_hint")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: adminViewModel.label("ui.editor.mask_precision_scale_tight_hint")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeCaption
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.WordWrap
            }
        }

        Item {
            id: trackRow
            width: parent.width
            height: control.handleSize + Theme.spacing * 0.75

            Repeater {
                model: control.stepCount

                Rectangle {
                    readonly property int stepValue: control.from + index
                    readonly property bool active: stepValue === control.value
                    x: control.handleXForValue(stepValue) + control.handleSize / 2 - width / 2
                    y: 0
                    width: active ? 3 : 2
                    height: active ? Theme.spacing * 0.9 : Theme.spacing * 0.55
                    radius: 1
                    color: active ? Theme.gold : Theme.textSecondary
                }
            }

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
                y: (trackRow.height - height) / 2
                width: control.handleSize
                height: width
                radius: Theme.radius
                color: Theme.gold
                border.color: Theme.primary
                border.width: Theme.borderWidth
                x: control.handleXForValue(control.value)

                MouseArea {
                    anchors.fill: parent
                    drag.target: handle
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: Math.max(0, track.width - handle.width)

                    function updateFromHandle() {
                        control.value = control.valueFromHandleX(handle.x)
                    }

                    onPositionChanged: {
                        if (!pressed) {
                            return
                        }
                        updateFromHandle()
                    }

                    onReleased: updateFromHandle()
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: function(mouse) {
                    const span = Math.max(0, track.width - handle.width)
                    if (span <= 0) {
                        return
                    }
                    const clickX = Math.min(Math.max(mouse.x - handle.width / 2, 0), span)
                    handle.x = clickX
                    control.value = control.valueFromHandleX(clickX)
                }
            }

            onWidthChanged: handle.x = control.handleXForValue(control.value)

            Connections {
                target: control
                function onValueChanged() {
                    handle.x = control.handleXForValue(control.value)
                }
            }
        }

        Item {
            id: numberRow
            width: parent.width
            height: Theme.fontSizeCaption + Theme.spacing * 0.15

            Repeater {
                model: control.stepCount

                Item {
                    readonly property int stepValue: control.from + index
                    readonly property bool active: stepValue === control.value
                    x: control.handleXForValue(stepValue)
                    width: control.handleSize
                    height: parent.height

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        text: String(stepValue)
                        color: active ? Theme.gold : Theme.textSecondary
                        font.pixelSize: Theme.fontSizeCaption
                        font.bold: active
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: control.value = stepValue
                    }
                }
            }
        }
    }
}
