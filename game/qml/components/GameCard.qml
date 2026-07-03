import QtQuick
import ".."

Flipable {
    id: flip

    property bool faceUp: false
    property string label: ""
    property int cardIndex: 0

    width: 140
    height: 180

    transform: Rotation {
        id: rot
        origin.x: flip.width / 2
        origin.y: flip.height / 2
        axis { x: 0; y: 1; z: 0 }
        angle: flip.faceUp ? 180 : 0
        Behavior on angle {
            NumberAnimation { duration: Theme.animNormal; easing.type: Easing.InOutCubic }
        }
    }

    // Рубашка
    front: Rectangle {
        radius: Theme.radius
        color: Theme.cardBack
        border.color: Theme.secondary
        border.width: Theme.borderWidth

        Column {
            anchors.centerIn: parent
            spacing: 8
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "?"
                font.pixelSize: 48
                font.bold: true
                color: Theme.primary
            }
            Repeater {
                model: 3
                Rectangle {
                    width: 60
                    height: 3
                    radius: 1
                    color: Theme.secondary
                    opacity: 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // Лицо
    back: Item {
        Rectangle {
            anchors.fill: parent
            radius: Theme.radius
            color: Theme.cardFront
            border.color: Theme.primary
            border.width: Theme.borderWidth

            Image {
                id: cardImage
                anchors.fill: parent
                anchors.margins: 8
                visible: status === Image.Ready
                source: (gameViewModel.currentPuzzleId > 0 && flip.cardIndex === 0)
                        ? gameViewModel.puzzleDisplayImageUrl(0)
                        : ""
                fillMode: Image.PreserveAspectFit
                cache: false
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                visible: cardImage.status !== Image.Ready
                text: flip.label || gameViewModel.label("ui.card.default_format").arg(flip.cardIndex + 1)
                color: Theme.textOnAccent
                font.pixelSize: Theme.fontSizeCaption
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}
