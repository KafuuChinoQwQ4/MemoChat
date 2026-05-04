import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var smartResult: null
    property bool busy: false
    property string statusText: ""
    property string resultTitle: "智能结果"
    property bool compactMode: false

    signal summaryRequested()
    signal suggestRequested()
    signal translateRequested()
    signal clearResultRequested()

    property string _pendingSummary: ""
    property string _pendingSuggest: ""
    property string _pendingTranslate: ""

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.compactMode ? 0 : 12
        anchors.rightMargin: root.compactMode ? 0 : 12
        spacing: root.compactMode ? 6 : 8

        Item {
            Layout.fillWidth: true
            visible: !root.compactMode
        }

        GlassButton {
            Layout.preferredWidth: root.compactMode ? 76 : 80
            Layout.preferredHeight: 30
            text: "📝 摘要"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.busy
            normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
            hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
            pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
            onClicked: {
                root.summaryRequested()
            }
        }

        GlassButton {
            Layout.preferredWidth: root.compactMode ? 92 : 96
            Layout.preferredHeight: 30
            text: "💡 建议回复"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.busy
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
            onClicked: {
                root.suggestRequested()
            }
        }

        GlassButton {
            Layout.preferredWidth: root.compactMode ? 76 : 80
            Layout.preferredHeight: 30
            text: "🌐 翻译"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.busy
            normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
            hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
            pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
            onClicked: {
                root.translateRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            visible: !root.compactMode
        }

        Label {
            Layout.maximumWidth: 160
            text: root.busy ? "AI 处理中" : root.statusText
            color: "#6a7b92"
            font.pixelSize: 11
            elide: Text.ElideRight
            visible: !root.compactMode && text.length > 0
        }
    }

    // 智能结果浮层
    Rectangle {
        id: smartResultBanner
        anchors.horizontalCenter: root.compactMode ? undefined : parent.horizontalCenter
        anchors.right: root.compactMode ? parent.right : undefined
        anchors.bottom: parent.top
        anchors.bottomMargin: 8
        width: root.compactMode ? 380 : Math.min(Math.max(parent.width - 40, 260), 500)
        height: Math.min(260, contentColumn.implicitHeight + 24)
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.95)
        border.color: Qt.rgba(1, 1, 1, 0.6)
        visible: root.smartResult && root.smartResult.length > 0
        property string currentResult: root.smartResult || ""

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: root.resultTitle
                font.pixelSize: 13
                font.bold: true
                color: "#2a3649"
            }

            TextEdit {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(170, Math.max(70, paintedHeight + 12))
                text: root.smartResult || ""
                wrapMode: Text.Wrap
                font.pixelSize: 13
                color: "#253247"
                textFormat: Text.PlainText
                readOnly: true
                selectByMouse: true
                cursorVisible: false
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item { Layout.fillWidth: true }

                GlassButton {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 26
                    text: "关闭"
                    textPixelSize: 11
                    cornerRadius: 6
                    normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                    onClicked: {
                        root.clearResultRequested()
                    }
                }
            }
        }
    }
}
