import QtQuick
import "."

SettingsTextField {
    id: control

    property int reloadKey: 0
    property var readValue: null

    function reloadFromModel() {
        if (typeof readValue !== "function")
            return
        control.text = readValue() || ""
    }

    Component.onCompleted: reloadFromModel()

    onReloadKeyChanged: {
        if (readOnly || !control.inputActiveFocus)
            reloadFromModel()
    }
}
