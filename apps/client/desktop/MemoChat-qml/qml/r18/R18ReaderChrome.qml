import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property bool chromeVisible: true
    property string title: ""
    property int currentPageIndex: 1
    property int pageCount: 1
    property bool favorite: false
    property color panelFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color panelStrokeColor: Qt.rgba(1, 1, 1, 0.46)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal backRequested()
    signal favoriteToggled(bool favorite)
    signal pageMoved(int page)

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.chromeVisible ? 56 : 0
        clip: true
        color: root.panelFillColor
        border.color: root.panelStrokeColor
        Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 8

            GlassButton {
                Layout.preferredWidth: 86
                Layout.preferredHeight: 32
                text: "返回详情"
                textColor: root.textSecondaryColor
                cornerRadius: 8
                normalColor: root.secondaryButtonColor
                hoverColor: root.secondaryButtonHoverColor
                pressedColor: root.secondaryButtonPressedColor
                onClicked: root.backRequested()
            }

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.textPrimaryColor
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.chromeVisible ? 58 : 0
        clip: true
        color: root.panelFillColor
        border.color: root.panelStrokeColor
        Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Text {
                text: "P " + root.currentPageIndex + " / " + root.pageCount
                color: root.textPrimaryColor
                font.pixelSize: 13
            }

            Slider {
                Layout.fillWidth: true
                from: 1
                to: Math.max(1, root.pageCount)
                stepSize: 1
                value: root.currentPageIndex
                onMoved: root.pageMoved(Math.max(1, Math.round(value)))
            }

            GlassButton {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 32
                text: root.favorite ? "已收藏" : "收藏"
                textColor: root.primaryButtonTextColor
                cornerRadius: 8
                normalColor: root.primaryButtonColor
                hoverColor: root.primaryButtonHoverColor
                pressedColor: root.primaryButtonPressedColor
                onClicked: root.favoriteToggled(!root.favorite)
            }
        }
    }
}
