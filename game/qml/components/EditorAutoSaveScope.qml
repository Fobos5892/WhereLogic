import QtQuick
import ".."

FocusScope {
    id: scope

    property bool enabled: true
    property var onSave: null

    function requestSave() {
        if (!scope.enabled || !scope.onSave)
            return
        scope.onSave()
    }

    onActiveFocusChanged: {
        if (!activeFocus)
            scope.requestSave()
    }
}
