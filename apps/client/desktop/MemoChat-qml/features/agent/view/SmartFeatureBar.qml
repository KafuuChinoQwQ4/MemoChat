import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Rectangle {
    id: root
    color: "transparent"

    property Item backdrop: null
    property var smartResult: null
    property bool busy: false
    property string statusText: ""
    property string resultTitle: "智能结果"
    property bool compactMode: false

    signal summaryRequested()
    signal suggestRequested()
    signal translateRequested()
    signal clearResultRequested()

    // 单一「智能」入口：把摘要/建议回复/翻译收纳进弹出菜单，整行更清爽
    GlassButton {
        id: smartEntry
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 84
        height: 30
        text: root.busy ? "✨ 处理中" : "✨ 智能 ▾"
        textPixelSize: 12
        cornerRadius: 8
        enabled: !root.busy
        normalColor: Qt.rgba(0.40, 0.62, 0.92, 0.20)
        hoverColor: Qt.rgba(0.40, 0.62, 0.92, 0.30)
        pressedColor: Qt.rgba(0.40, 0.62, 0.92, 0.40)
        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
        onClicked: smartMenuPopup.open()
    }

    Popup {
        id: smartMenuPopup
        width: 152
        height: smartMenuColumn.implicitHeight + 16
        // 输入栏位于窗口底部，菜单向上弹出，右对齐入口按钮
        x: Math.round(smartEntry.x + smartEntry.width - width)
        y: Math.round(smartEntry.y - height - 6)
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "transparent"
        }

        contentItem: GlassSurface {
            backdrop: root.backdrop
            cornerRadius: 12
            blurRadius: 20
            fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.88)
            strokeColor: Qt.rgba(1, 1, 1, 0.58)
            glowTopColor: Qt.rgba(1, 1, 1, 0.28)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

            ColumnLayout {
                id: smartMenuColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "📝 摘要"
                    textPixelSize: 13
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                    hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
                    pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
                    onClicked: {
                        smartMenuPopup.close()
                        root.summaryRequested()
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "💡 建议回复"
                    textPixelSize: 13
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                    onClicked: {
                        smartMenuPopup.close()
                        root.suggestRequested()
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: "🌐 翻译"
                    textPixelSize: 13
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                    hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.28)
                    pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
                    onClicked: {
                        smartMenuPopup.close()
                        root.translateRequested()
                    }
                }
            }
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
