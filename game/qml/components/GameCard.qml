import QtQuick
import ".."

Flipable {
    id: flip

    property bool faceUp: false
    property string label: ""
    property int cardIndex: 0
    property bool hideAnswer: false
    property bool answerRevealed: false

    width: 140
    height: 180

    transform: Rotation {
        origin.x: flip.width / 2
        origin.y: flip.height / 2
        axis { x: 0; y: 1; z: 0 }
        angle: flip.faceUp ? 180 : 0
        Behavior on angle {
            NumberAnimation { duration: Theme.animNormal; easing.type: Easing.InOutCubic }
        }
    }

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

    back: Rectangle {
        radius: Theme.radius
        color: Theme.cardFront
        border.color: Theme.primary
        border.width: Theme.borderWidth

        Image {
            id: cardImage
            anchors.fill: parent
            anchors.margins: 8
            source: gameViewModel.currentPuzzleId > 0
                    ? (flip.cardIndex === 0
                       ? gameViewModel.puzzleSlot0DisplayUrl
                       : gameViewModel.puzzleImageUrl(flip.cardIndex))
                    : ""
            fillMode: Image.PreserveAspectFit
            cache: false
            opacity: flip.hideAnswer
                     ? (flip.answerRevealed ? (status === Image.Loading ? 0.35 : 1) : 0)
                     : ((status === Image.Ready && source.length > 0)
                        ? (status === Image.Loading ? 0.35 : 1)
                        : 0)
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.animNormal * 1.5
                    easing.type: Easing.InOutCubic
                }
            }
        }

        Image {
            id: cardSpinner
            anchors.centerIn: parent
            width: 40
            height: 40
            visible: cardImage.status === Image.Loading
                     && (flip.hideAnswer ? flip.answerRevealed : !flip.hideAnswer)
            source: "qrc:/qml/assets/spinner.svg"
            fillMode: Image.PreserveAspectFit
            transformOrigin: Item.Center
            RotationAnimation on rotation {
                from: 0
                to: 360
                duration: 850
                loops: Animation.Infinite
                running: cardSpinner.visible
            }
        }

        Text {
            id: hiddenAnswerMark
            anchors.centerIn: parent
            width: parent.width - 16
            visible: flip.hideAnswer
                     && (!flip.answerRevealed
                         || opacity > 0.01)
            text: "?"
            color: Theme.primary
            font.pixelSize: Theme.fontSizeHero
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            opacity: flip.hideAnswer ? (flip.answerRevealed ? 0 : 1) : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.animNormal * 1.5
                    easing.type: Easing.InOutCubic
                }
            }
        }

        Text {
            anchors.centerIn: parent
            width: parent.width - 16
            visible: !flip.hideAnswer
                     && cardImage.status !== Image.Ready
                     && cardImage.status !== Image.Loading
            text: flip.label || gameViewModel.label("ui.card.default_format").arg(flip.cardIndex + 1)
            color: Theme.textOnAccent
            font.pixelSize: Theme.fontSizeCaption
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}
