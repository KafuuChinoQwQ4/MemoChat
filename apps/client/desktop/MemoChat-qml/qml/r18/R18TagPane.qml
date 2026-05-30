import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property var tagBuckets: []
    property bool loading: false
    property bool sourceSelected: false
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color badgeFillColor: "#dce6f8"
    property color badgeTextColor: "#526173"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal reloadRequested()
    signal tagSelected(string tag)

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    Layout.fillWidth: true
                    text: "Tag 分类"
                    color: root.textPrimaryColor
                    font.pixelSize: 20
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: root.tagBuckets.length + " 个标签 · 点击后在当前源中查找"
                    color: root.textMutedColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            GlassButton {
                Layout.preferredWidth: 86
                Layout.preferredHeight: 34
                text: "重载"
                textPixelSize: 13
                textColor: root.textSecondaryColor
                cornerRadius: 17
                normalColor: root.secondaryButtonColor
                hoverColor: root.secondaryButtonHoverColor
                pressedColor: root.secondaryButtonPressedColor
                enabled: root.sourceSelected
                onClicked: root.reloadRequested()
            }
        }

        GridView {
            id: sourceTagGridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            cacheBuffer: 360
            cellWidth: Math.max(138, Math.floor(width / Math.max(1, Math.floor(width / 154))))
            cellHeight: 58
            model: root.tagBuckets
            ScrollBar.vertical: GlassScrollBar {}

            delegate: Rectangle {
                width: GridView.view.cellWidth - 10
                height: 46
                radius: 8
                color: tagMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                border.color: root.fieldStrokeColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 10
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        text: modelData.name
                        color: root.textPrimaryColor
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.preferredWidth: tagCountText.implicitWidth + 14
                        Layout.preferredHeight: 22
                        radius: 11
                        color: root.badgeFillColor

                        Text {
                            id: tagCountText
                            anchors.centerIn: parent
                            text: modelData.count
                            color: root.badgeTextColor
                            font.pixelSize: 11
                        }
                    }
                }

                MouseArea {
                    id: tagMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tagSelected(modelData.name)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.tagBuckets.length === 0 && !root.loading
            text: root.sourceSelected ? "当前加载的漫画还没有 tag，可先刷新漫画列表" : "请先选择已导入漫画源"
            color: root.textMutedColor
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
