pragma Singleton
import QtQuick

QtObject {
    id: theme

    property var window: null

    readonly property real refW: 960
    readonly property real refH: 540

    readonly property real w: window ? window.width : refW
    readonly property real h: window ? window.height : refH
    readonly property real scale: Math.min(w / refW, h / refH)

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

    readonly property int touchMin: Math.round(h * 0.04)
    readonly property int buttonHeight: Math.round(h * 0.052)
    readonly property int margin: Math.round(w * 0.013)
    readonly property int spacing: Math.round(w * 0.009)
    readonly property int radius: Math.max(6, Math.round(w * 0.01))
    readonly property int borderWidth: Math.max(1, Math.round(w * 0.0015))

    readonly property int fontSizeHero: Math.round(h * 0.045)
    readonly property int fontSizeTitle: Math.round(h * 0.03)
    readonly property int fontSizeBody: Math.round(h * 0.022)
    readonly property int fontSizeCaption: Math.round(h * 0.017)
    readonly property int fontSizeButton: Math.round(h * 0.021)

    readonly property int iconSm: Math.round(h * 0.022)
    readonly property int iconMd: Math.round(h * 0.028)
    readonly property int iconLg: Math.round(h * 0.033)
    readonly property int slotSize: Math.round(h * 0.09)
    readonly property int chromeSize: Math.round(h * 0.056)
    readonly property int topBarHeight: chromeSize + spacing * 2
    readonly property real cardAspect: 1.56
    readonly property real cardImageMargin: spacing * 0.35

    readonly property int animFast: 150
    readonly property int animNormal: 400
}
