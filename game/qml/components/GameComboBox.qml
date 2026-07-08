import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import ".."
import "."

ComboBox {
    id: control

    property bool fillWidth: false

    implicitHeight: Theme.buttonHeight
    width: fillWidth && parent ? parent.width : implicitWidth

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: Theme.buttonHeight
        radius: Theme.radius
        color: Theme.surface
        border.color: control.activeFocus ? Theme.primary : Theme.secondary
        border.width: Theme.borderWidth
    }

    contentItem: Text {
        leftPadding: Theme.spacing
        rightPadding: Theme.spacing + control.indicator.width
        text: control.displayText
        font.pixelSize: Theme.fontSizeBody
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    delegate: ItemDelegate {
        id: row
        width: control.width
        height: Theme.buttonHeight

        required property int index

        contentItem: Text {
            text: control.textAt(row.index)
            color: row.highlighted ? Theme.primary : Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: Theme.spacing
            rightPadding: Theme.spacing
        }

        background: Rectangle {
            color: row.highlighted
                   ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.14)
                   : "transparent"
            radius: Theme.radius * 0.5
        }
    }

    indicator: Text {
        text: "▾"
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeCaption
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacing
        anchors.verticalCenter: parent.verticalCenter
    }

    popup: Popup {
        y: control.height + 2
        width: control.width
        implicitHeight: Math.min(listView.contentHeight + padding * 2,
                                 Theme.buttonHeight * 6 + padding * 2)
        padding: Theme.spacing * 0.35

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            ScrollBar.vertical: ThemedScrollBar {}
        }

        background: Rectangle {
            color: Theme.surfaceAlt
            border.color: Theme.secondary
            border.width: Theme.borderWidth
            radius: Theme.radius
        }
    }
}
