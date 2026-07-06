pragma Singleton
import QtQuick

QtObject {
    id: theme

    property var window: null

    readonly property real refW: 960
    readonly property real refH: 540
    readonly property real scale: window
        ? Math.min(window.width / refW, window.height / refH)
        : 1.0

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

    readonly property int touchMin: Math.max(32, Math.round(40 * scale))
    readonly property int buttonHeight: Math.max(34, Math.round(42 * scale))
    readonly property int margin: Math.max(8, Math.round(14 * scale))
    readonly property int spacing: Math.max(6, Math.round(9 * scale))
    readonly property int radius: Math.max(6, Math.round(8 * scale))
    readonly property int borderWidth: Math.max(1, Math.round(2 * scale))

    readonly property int fontSizeHero: Math.max(26, Math.round(34 * scale))
    readonly property int fontSizeTitle: Math.max(18, Math.round(22 * scale))
    readonly property int fontSizeBody: Math.max(13, Math.round(16 * scale))
    readonly property int fontSizeCaption: Math.max(11, Math.round(12 * scale))
    readonly property int fontSizeButton: Math.max(12, Math.round(15 * scale))

    readonly property int iconSm: Math.max(14, Math.round(16 * scale))
    readonly property int iconMd: Math.max(18, Math.round(20 * scale))
    readonly property int iconLg: Math.max(20, Math.round(24 * scale))
    readonly property int slotSize: Math.max(56, Math.round(68 * scale))
    readonly property int chromeSize: Math.max(28, Math.round(34 * scale))

    readonly property int animFast: 150
    readonly property int animNormal: 400
}
