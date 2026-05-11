import QtQuick 2.15
import QtQuick.Shapes

Item {
    id: root

    property real cornerRadius: 24
    property color fillTopColor: Qt.rgba(0.93, 0.97, 1.0, 0.36)
    property color fillBottomColor: Qt.rgba(0.86, 0.93, 1.0, 0.34)
    property color glowTopColor: Qt.rgba(1, 1, 1, 0.20)
    property color glowMiddleColor: Qt.rgba(0.92, 0.97, 1.0, 0.07)
    property color glowBottomColor: Qt.rgba(0.72, 0.84, 0.96, 0.08)
    property color strokeColor: Qt.rgba(1, 1, 1, 0.46)
    property real strokeWidth: 0.9
    property real featherInset: 1.8
    property real featherOpacity: 0.28

    readonly property real radiusValue: Math.max(0, Math.min(root.cornerRadius, root.width / 2, root.height / 2))
    readonly property real curveControl: root.radiusValue * 0.5522847498307936
    readonly property real bodyInset: root.radiusValue > 0 ? Math.max(0, root.featherInset) : 0
    readonly property real bodyLeft: root.bodyInset
    readonly property real bodyTop: root.bodyInset
    readonly property real bodyRight: Math.max(root.bodyLeft, root.width - root.bodyInset)
    readonly property real bodyBottom: Math.max(root.bodyTop, root.height - root.bodyInset)
    readonly property real bodyWidth: Math.max(0, root.bodyRight - root.bodyLeft)
    readonly property real bodyHeight: Math.max(0, root.bodyBottom - root.bodyTop)
    readonly property real bodyRadius: Math.max(0, Math.min(root.radiusValue - root.bodyInset, root.bodyWidth / 2, root.bodyHeight / 2))
    readonly property real bodyControl: root.bodyRadius * 0.5522847498307936
    readonly property real strokeInset: Math.max(0.5, root.strokeWidth / 2)
    readonly property real strokeLeft: root.bodyLeft + root.strokeInset
    readonly property real strokeTop: root.bodyTop + root.strokeInset
    readonly property real strokeRight: Math.max(root.strokeLeft, root.bodyRight - root.strokeInset)
    readonly property real strokeBottom: Math.max(root.strokeTop, root.bodyBottom - root.strokeInset)
    readonly property real strokeRadius: Math.max(0, root.bodyRadius - root.strokeInset)
    readonly property real strokeControl: root.strokeRadius * 0.5522847498307936

    function withAlphaScale(colorValue, alphaScale) {
        return Qt.rgba(colorValue.r, colorValue.g, colorValue.b, colorValue.a * alphaScale)
    }

    Shape {
        anchors.fill: parent
        asynchronous: false
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillGradient: LinearGradient {
                x1: 0
                y1: 0
                x2: 0
                y2: root.height
                GradientStop { position: 0.0; color: root.withAlphaScale(root.fillTopColor, root.featherOpacity) }
                GradientStop { position: 1.0; color: root.withAlphaScale(root.fillBottomColor, root.featherOpacity) }
            }
            startX: root.radiusValue
            startY: 0
            PathLine { x: root.width - root.radiusValue; y: 0 }
            PathCubic {
                control1X: root.width - root.radiusValue + root.curveControl
                control1Y: 0
                control2X: root.width
                control2Y: root.radiusValue - root.curveControl
                x: root.width
                y: root.radiusValue
            }
            PathLine { x: root.width; y: root.height - root.radiusValue }
            PathCubic {
                control1X: root.width
                control1Y: root.height - root.radiusValue + root.curveControl
                control2X: root.width - root.radiusValue + root.curveControl
                control2Y: root.height
                x: root.width - root.radiusValue
                y: root.height
            }
            PathLine { x: root.radiusValue; y: root.height }
            PathCubic {
                control1X: root.radiusValue - root.curveControl
                control1Y: root.height
                control2X: 0
                control2Y: root.height - root.radiusValue + root.curveControl
                x: 0
                y: root.height - root.radiusValue
            }
            PathLine { x: 0; y: root.radiusValue }
            PathCubic {
                control1X: 0
                control1Y: root.radiusValue - root.curveControl
                control2X: root.radiusValue - root.curveControl
                control2Y: 0
                x: root.radiusValue
                y: 0
            }
        }

        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillGradient: LinearGradient {
                x1: 0
                y1: root.bodyTop
                x2: 0
                y2: root.bodyBottom
                GradientStop { position: 0.0; color: root.fillTopColor }
                GradientStop { position: 1.0; color: root.fillBottomColor }
            }
            startX: root.bodyLeft + root.bodyRadius
            startY: root.bodyTop
            PathLine { x: root.bodyRight - root.bodyRadius; y: root.bodyTop }
            PathCubic {
                control1X: root.bodyRight - root.bodyRadius + root.bodyControl
                control1Y: root.bodyTop
                control2X: root.bodyRight
                control2Y: root.bodyTop + root.bodyRadius - root.bodyControl
                x: root.bodyRight
                y: root.bodyTop + root.bodyRadius
            }
            PathLine { x: root.bodyRight; y: root.bodyBottom - root.bodyRadius }
            PathCubic {
                control1X: root.bodyRight
                control1Y: root.bodyBottom - root.bodyRadius + root.bodyControl
                control2X: root.bodyRight - root.bodyRadius + root.bodyControl
                control2Y: root.bodyBottom
                x: root.bodyRight - root.bodyRadius
                y: root.bodyBottom
            }
            PathLine { x: root.bodyLeft + root.bodyRadius; y: root.bodyBottom }
            PathCubic {
                control1X: root.bodyLeft + root.bodyRadius - root.bodyControl
                control1Y: root.bodyBottom
                control2X: root.bodyLeft
                control2Y: root.bodyBottom - root.bodyRadius + root.bodyControl
                x: root.bodyLeft
                y: root.bodyBottom - root.bodyRadius
            }
            PathLine { x: root.bodyLeft; y: root.bodyTop + root.bodyRadius }
            PathCubic {
                control1X: root.bodyLeft
                control1Y: root.bodyTop + root.bodyRadius - root.bodyControl
                control2X: root.bodyLeft + root.bodyRadius - root.bodyControl
                control2Y: root.bodyTop
                x: root.bodyLeft + root.bodyRadius
                y: root.bodyTop
            }
        }

        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillGradient: LinearGradient {
                x1: 0
                y1: root.bodyTop
                x2: 0
                y2: root.bodyBottom
                GradientStop { position: 0.0; color: root.glowTopColor }
                GradientStop { position: 0.46; color: root.glowMiddleColor }
                GradientStop { position: 1.0; color: root.glowBottomColor }
            }
            startX: root.bodyLeft + root.bodyRadius
            startY: root.bodyTop
            PathLine { x: root.bodyRight - root.bodyRadius; y: root.bodyTop }
            PathCubic {
                control1X: root.bodyRight - root.bodyRadius + root.bodyControl
                control1Y: root.bodyTop
                control2X: root.bodyRight
                control2Y: root.bodyTop + root.bodyRadius - root.bodyControl
                x: root.bodyRight
                y: root.bodyTop + root.bodyRadius
            }
            PathLine { x: root.bodyRight; y: root.bodyBottom - root.bodyRadius }
            PathCubic {
                control1X: root.bodyRight
                control1Y: root.bodyBottom - root.bodyRadius + root.bodyControl
                control2X: root.bodyRight - root.bodyRadius + root.bodyControl
                control2Y: root.bodyBottom
                x: root.bodyRight - root.bodyRadius
                y: root.bodyBottom
            }
            PathLine { x: root.bodyLeft + root.bodyRadius; y: root.bodyBottom }
            PathCubic {
                control1X: root.bodyLeft + root.bodyRadius - root.bodyControl
                control1Y: root.bodyBottom
                control2X: root.bodyLeft
                control2Y: root.bodyBottom - root.bodyRadius + root.bodyControl
                x: root.bodyLeft
                y: root.bodyBottom - root.bodyRadius
            }
            PathLine { x: root.bodyLeft; y: root.bodyTop + root.bodyRadius }
            PathCubic {
                control1X: root.bodyLeft
                control1Y: root.bodyTop + root.bodyRadius - root.bodyControl
                control2X: root.bodyLeft + root.bodyRadius - root.bodyControl
                control2Y: root.bodyTop
                x: root.bodyLeft + root.bodyRadius
                y: root.bodyTop
            }
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.strokeColor
            strokeWidth: root.strokeWidth
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap
            startX: root.strokeLeft + root.strokeRadius
            startY: root.strokeTop
            PathLine { x: root.strokeRight - root.strokeRadius; y: root.strokeTop }
            PathCubic {
                control1X: root.strokeRight - root.strokeRadius + root.strokeControl
                control1Y: root.strokeTop
                control2X: root.strokeRight
                control2Y: root.strokeTop + root.strokeRadius - root.strokeControl
                x: root.strokeRight
                y: root.strokeTop + root.strokeRadius
            }
            PathLine { x: root.strokeRight; y: root.strokeBottom - root.strokeRadius }
            PathCubic {
                control1X: root.strokeRight
                control1Y: root.strokeBottom - root.strokeRadius + root.strokeControl
                control2X: root.strokeRight - root.strokeRadius + root.strokeControl
                control2Y: root.strokeBottom
                x: root.strokeRight - root.strokeRadius
                y: root.strokeBottom
            }
            PathLine { x: root.strokeLeft + root.strokeRadius; y: root.strokeBottom }
            PathCubic {
                control1X: root.strokeLeft + root.strokeRadius - root.strokeControl
                control1Y: root.strokeBottom
                control2X: root.strokeLeft
                control2Y: root.strokeBottom - root.strokeRadius + root.strokeControl
                x: root.strokeLeft
                y: root.strokeBottom - root.strokeRadius
            }
            PathLine { x: root.strokeLeft; y: root.strokeTop + root.strokeRadius }
            PathCubic {
                control1X: root.strokeLeft
                control1Y: root.strokeTop + root.strokeRadius - root.strokeControl
                control2X: root.strokeLeft + root.strokeRadius - root.strokeControl
                control2Y: root.strokeTop
                x: root.strokeLeft + root.strokeRadius
                y: root.strokeTop
            }
        }
    }
}
