import QtQuick
import ".."
import "."

Flipable {
    id: flip

    property bool faceUp: false
    property string label: ""
    property int cardIndex: 0
    property bool hideAnswer: false
    property bool answerRevealed: false

    readonly property real cardAspect: Theme.cardAspect
    readonly property real cardRadius: Theme.radius
    readonly property string slotImageUrl: gameViewModel.currentPuzzleId > 0
            ? (flip.cardIndex === 0
               ? gameViewModel.puzzleSlot0DisplayUrl
               : gameViewModel.puzzleImageUrl(flip.cardIndex))
            : ""

    implicitWidth: Theme.slotSize * 3.6
    implicitHeight: implicitWidth / cardAspect
    clip: true

    transform: Rotation {
        origin.x: flip.width / 2
        origin.y: flip.height / 2
        axis { x: 0; y: 1; z: 0 }
        angle: flip.faceUp ? 180 : 0
        Behavior on angle {
            NumberAnimation { duration: Theme.animNormal; easing.type: Easing.InOutCubic }
        }
    }

    front: CyberBillboard {
        anchors.fill: parent
        cornerRadius: flip.cardRadius
        glowColor: Theme.primary
        panelColor: Theme.cardBack
        panelOpacity: 1
        svgSource: "qrc:/qml/assets/card-back.svg"
        billboardGlow: false
    }

    back: Item {
        anchors.fill: parent
        clip: true

        transform: Rotation {
            origin.x: flip.width / 2
            origin.y: flip.height / 2
            axis { x: 0; y: 1; z: 0 }
            angle: 180
        }

        CyberBillboard {
            id: backPanel
            anchors.fill: parent
            cornerRadius: flip.cardRadius
            glowColor: Theme.primary
            panelColor: Theme.cardFront
            panelOpacity: 1
            billboardGlow: true

            Image {
                id: cardImage
                anchors.fill: parent
                anchors.margins: Theme.cardImageMargin
                source: flip.slotImageUrl
                fillMode: Image.PreserveAspectFit
                cache: false
                asynchronous: true
                visible: !flip.hideAnswer || flip.answerRevealed
                opacity: cardImage.status === Image.Ready ? 1
                         : (cardImage.status === Image.Loading ? 0.35 : 0)
                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.animNormal * 1.5
                        easing.type: Easing.InOutCubic
                    }
                }
            }

            Image {
                id: cardPlaceholder
                anchors.centerIn: parent
                width: parent.width * 0.42
                height: width / cardAspect
                visible: (!flip.hideAnswer || flip.answerRevealed)
                         && cardImage.status !== Image.Ready
                         && cardImage.status !== Image.Loading
                source: "qrc:/qml/assets/card-back.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
                opacity: 0.55
            }

            Image {
                id: cardSpinner
                anchors.centerIn: parent
                width: Theme.iconLg * 1.5
                height: width
                visible: cardImage.status === Image.Loading
                         && (!flip.hideAnswer || flip.answerRevealed)
                source: "qrc:/qml/assets/spinner.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
                transformOrigin: Item.Center
                RotationAnimation on rotation {
                    from: 0
                    to: 360
                    duration: Theme.animNormal * 2
                    loops: Animation.Infinite
                    running: cardSpinner.visible
                }
            }

            Image {
                id: hiddenAnswerMark
                anchors.centerIn: parent
                width: parent.width * 0.38
                height: width / cardAspect
                visible: flip.hideAnswer
                         && (!flip.answerRevealed || opacity > 0.01)
                source: "qrc:/qml/assets/card-back.svg"
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
                opacity: flip.hideAnswer ? (flip.answerRevealed ? 0 : 1) : 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.animNormal * 1.5
                        easing.type: Easing.InOutCubic
                    }
                }
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.spacing
                width: parent.width - Theme.spacing * 2
                visible: !flip.hideAnswer
                         && cardImage.status !== Image.Ready
                         && cardImage.status !== Image.Loading
                text: flip.label || gameViewModel.label("ui.card.default_format").arg(flip.cardIndex + 1)
                color: Theme.textOnAccent
                font.pixelSize: Theme.fontSizeCaption
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }
}
