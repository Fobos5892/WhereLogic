import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: control

    property int from: 0
    property int to: 100
    property int value: 50

    implicitHeight: Theme.buttonHeight

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Slider {
            id: slider
            Layout.fillWidth: true
            from: control.from
            to: control.to
            value: control.value
            onMoved: control.value = Math.round(value)
            onPressedChanged: {
                if (!pressed) {
                    control.value = Math.round(value)
                }
            }

            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 6
                radius: 3
                color: Theme.surfaceAlt

                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: Theme.primary
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 22
                height: 22
                radius: Theme.radius
                color: Theme.gold
                border.color: Theme.primary
                border.width: Theme.borderWidth
            }
        }

        Text {
            text: control.value + "%"
            color: Theme.primary
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
            Layout.preferredWidth: 48
            horizontalAlignment: Text.AlignRight
        }
    }
}
