import QtQuick

import QtQuick.Layouts

import ".."

import "../components"



Item {

    ColumnLayout {

        anchors.centerIn: parent

        width: Math.min(parent.width * 0.85, 640)

        spacing: Theme.spacing * 2



        Text {

            Layout.fillWidth: true

            text: gameViewModel.label("ui.welcome.title")

            color: Theme.primary

            font.pixelSize: Theme.fontSizeHero

            font.bold: true

            font.letterSpacing: 4

            horizontalAlignment: Text.AlignHCenter

        }



        Text {

            Layout.fillWidth: true

            text: gameViewModel.label("ui.welcome.subtitle")

            color: Theme.textSecondary

            font.pixelSize: Theme.fontSizeBody

            horizontalAlignment: Text.AlignHCenter

            wrapMode: Text.WordWrap

        }



        Item { Layout.preferredHeight: Theme.spacing }



        GameButton {

            Layout.alignment: Qt.AlignHCenter

            Layout.fillWidth: true

            text: gameViewModel.hasActiveSession

                  ? gameViewModel.label("ui.welcome.continue")

                  : gameViewModel.label("ui.welcome.start")

            gold: true

            onClicked: {

                if (gameViewModel.hasActiveSession)

                    gameViewModel.resumeSession()

                else if (adminViewModel.selectedPresetId > 0)

                    gameViewModel.startGame(adminViewModel.selectedPresetId)

                else

                    gameViewModel.startGame(1)

            }

        }



        GameButton {

            Layout.alignment: Qt.AlignHCenter

            Layout.fillWidth: true

            visible: gameViewModel.hasActiveSession

            text: gameViewModel.label("ui.welcome.new_game")

            primary: false

            outline: true

            onClicked: gameViewModel.clearSession()

        }

    }

}


