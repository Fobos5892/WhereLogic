pragma Singleton
import QtQuick

QtObject {
    // «Где Логика?» palette
    readonly property color background: "#0B0C10"
    readonly property color surface: "#1F2833"
    readonly property color surfaceAlt: "#252D38"
    readonly property color primary: "#66FCF1"
    readonly property color secondary: "#45A29E"
    readonly property color gold: "#FFD700"
    readonly property color success: "#39FF14"
    readonly property color danger: "#FF3131"
    readonly property color textPrimary: "#C5C6C7"
    readonly property color textSecondary: "#8B949E"
    readonly property color textOnAccent: "#0B0C10"
    readonly property color cardBack: "#1A2230"
    readonly property color cardFront: "#E8EDF2"
    readonly property color glowCyan: "#4066FCF1"

    // Touch / Android-friendly sizes
    readonly property int touchMin: 48
    readonly property int buttonHeight: 56
    readonly property int margin: 20
    readonly property int spacing: 12
    readonly property int radius: 10
    readonly property int borderWidth: 2

    readonly property int fontSizeHero: 42
    readonly property int fontSizeTitle: 28
    readonly property int fontSizeBody: 20
    readonly property int fontSizeCaption: 15
    readonly property int fontSizeButton: 18

    readonly property int animFast: 150
    readonly property int animNormal: 400
}
