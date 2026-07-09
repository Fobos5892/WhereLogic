import QtQuick
import ".."

FocusScope {
    id: scope

    property bool autosaveEnabled: true
    property var onSave: null

    function requestSave() {
        if (!scope.autosaveEnabled || !scope.onSave)
            return
        scope.onSave()
    }

    onActiveFocusChanged: {
        if (!activeFocus)
            scope.requestSave()
    }
}
