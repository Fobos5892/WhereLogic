import QtQuick
import QtQuick.Layouts
import ".."
import "../components"

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Text {
            Layout.fillWidth: true
            text: gameViewModel.label("ui.admin.preset")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeCaption
        }

        Repeater {
            model: adminViewModel.presets
            delegate: GameButton {
                Layout.fillWidth: true
                text: modelData.name
                primary: adminViewModel.selectedPresetId === modelData.id
                outline: adminViewModel.selectedPresetId !== modelData.id
                onClicked: adminViewModel.selectedPresetId = modelData.id
            }
        }
    }
}
