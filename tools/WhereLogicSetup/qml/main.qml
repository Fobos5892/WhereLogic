import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 720
    height: 640
    visible: true
    title: qsTr("WhereLogic Setup")

    Theme { id: theme }
    color: theme.background

    header: Rectangle {
        height: 72
        color: theme.surface
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: theme.margin
            spacing: 4
            Label {
                text: qsTr("Загрузка зависимостей")
                font.pixelSize: 24
                font.bold: true
                color: theme.primary
            }
            Label {
                text: setup.repoRoot
                font.pixelSize: 12
                color: theme.textSecondary
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.margin
        spacing: theme.spacing

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 1
            value: setup.totalProgress
            indeterminate: setup.busy && setup.totalProgress < 0.01
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: theme.spacing
            Button {
                text: qsTr("Обновить")
                enabled: !setup.busy
                onClicked: setup.refresh()
            }
            Button {
                text: qsTr("Скачать выбранное")
                enabled: !setup.busy
                highlighted: true
                onClicked: setup.fetchSelected()
            }
            Button {
                text: qsTr("Скачать всё")
                enabled: !setup.busy
                onClicked: setup.fetchAll()
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: setup.items
            delegate: Rectangle {
                width: list.width
                height: content.implicitHeight + 20
                radius: theme.radius
                color: theme.surface
                border.color: installed ? theme.success : theme.secondary
                border.width: 1

                required property int index
                required property string depId
                required property string label
                required property string kind
                required property string status
                required property string statusText
                required property real progress
                required property bool installed
                required property string manualHint
                required property bool selected

                ColumnLayout {
                    id: content
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            checked: selected
                            enabled: kind !== "manual" && !setup.busy
                            onToggled: setup.toggleRow(index, checked)
                        }
                        Label {
                            text: label
                            font.pixelSize: 16
                            font.bold: true
                            color: theme.textPrimary
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            text: installed ? "✓" : (kind === "manual" ? "ℹ" : "…")
                            color: installed ? theme.success : theme.gold
                            font.pixelSize: 18
                        }
                    }

                    Label {
                        text: statusText
                        color: status === "error" ? theme.danger : theme.textSecondary
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    ProgressBar {
                        visible: status === "downloading" || status === "extracting"
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        value: progress
                    }

                    Label {
                        visible: kind === "manual" && manualHint.length > 0
                        text: manualHint
                        color: theme.textSecondary
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            TextArea {
                readOnly: true
                text: setup.logText
                color: theme.textPrimary
                font.family: "Consolas"
                font.pixelSize: 12
                wrapMode: TextArea.Wrap
                background: Rectangle {
                    color: theme.surface
                    radius: theme.radius
                }
            }
        }
    }
}
