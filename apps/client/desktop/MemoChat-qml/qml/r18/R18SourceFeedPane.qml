import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property var comicModel: null
    property Component comicDelegate: null
    property string currentSourceId: ""
    property string keyword: ""
    property int comicCount: 0
    property bool loading: false
    property color fieldFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color placeholderColor: "#8493a3"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    readonly property bool sourceSelected: currentSourceId.length > 0

    signal keywordEdited(string keyword)
    signal refreshRequested()
    signal searchRequested()
    signal loadMoreProbe(var gridView)

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
                    text: "源内漫画"
                    color: root.textPrimaryColor
                    font.pixelSize: 20
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: root.sourceSelected
                          ? root.currentSourceId + " · " + root.comicCount + " 项"
                          : "请先在已导入列表选择一个漫画源"
                    color: root.textMutedColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            GlassButton {
                Layout.preferredWidth: 86
                Layout.preferredHeight: 34
                text: "刷新"
                textPixelSize: 13
                textColor: root.textSecondaryColor
                cornerRadius: 17
                normalColor: root.secondaryButtonColor
                hoverColor: root.secondaryButtonHoverColor
                pressedColor: root.secondaryButtonPressedColor
                enabled: root.sourceSelected
                onClicked: root.refreshRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: sourceFeedSearchField
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: root.keyword
                placeholderText: "在当前源中搜索"
                placeholderTextColor: root.placeholderColor
                color: root.textPrimaryColor
                selectByMouse: true
                leftPadding: 14
                rightPadding: 14
                font.pixelSize: 14
                background: Rectangle {
                    radius: 20
                    color: root.fieldFillColor
                    border.color: root.fieldStrokeColor
                }
                onTextEdited: root.keywordEdited(text)
                onAccepted: root.searchRequested()
            }

            GlassButton {
                Layout.preferredWidth: 82
                Layout.preferredHeight: 38
                text: "搜索"
                textPixelSize: 13
                textColor: root.primaryButtonTextColor
                cornerRadius: 19
                normalColor: root.primaryButtonColor
                hoverColor: root.primaryButtonHoverColor
                pressedColor: root.primaryButtonPressedColor
                enabled: root.sourceSelected
                onClicked: root.searchRequested()
            }
        }

        GridView {
            id: sourceComicGridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            cacheBuffer: Math.max(height, 720)
            cellWidth: Math.max(170, Math.floor(width / Math.max(1, Math.floor(width / 190))))
            cellHeight: 294
            model: root.comicModel
            ScrollBar.vertical: GlassScrollBar {}
            onContentYChanged: root.loadMoreProbe(sourceComicGridView)
            delegate: root.comicDelegate
        }

        Text {
            Layout.fillWidth: true
            visible: root.comicCount === 0 && !root.loading
            text: root.sourceSelected ? "当前源暂无漫画" : "请先选择已导入漫画源"
            color: root.textMutedColor
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
