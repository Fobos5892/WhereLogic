import QtQuick
import "."

SettingsTextField {
    id: control

    property int reloadKey: 0
    property var readValue: null

    function reloadFromModel() {
        if (typeof readValue !== "function")
            return
        const nextValue = readValue() || ""
        if (field.text === nextValue && control.text === nextValue)
            return
        control.text = nextValue
    }

    Component.onCompleted: reloadFromModel()

    onReloadKeyChanged: reloadFromModel()
}
