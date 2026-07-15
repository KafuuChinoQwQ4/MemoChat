import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var comicModel: null
    property Component comicDelegate: null
    property string keyword: ""
    property int comicCount: 0
    property string currentSourceId: ""
    property var sortOptions: []
    property var tagOptions: []
    property string selectedSort: ""
    property string selectedTag: ""
    property string tagPlaceholder: "tag / 分类"
    property bool filtersExpanded: false
    property color fieldFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color textPrimaryColor: "#263241"
    property color textMutedColor: "#7b8794"
    property color placeholderColor: "#8493a3"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color chipFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color chipActiveFillColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
    property color chipBorderColor: Qt.rgba(1, 1, 1, 0.38)

    readonly property bool sourceSelected: currentSourceId.length > 0

    signal keywordEdited(string keyword)
    signal searchRequested(string keyword)
    signal sortSelected(string sortId)
    signal tagSelected(string tagId)
    signal loadMoreProbe(var gridView)

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: searchField
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                placeholderText: "搜索"
                placeholderTextColor: root.placeholderColor
                color: root.textPrimaryColor
                text: root.keyword
                selectByMouse: true
                leftPadding: 18
                rightPadding: 18
                background: Rectangle {
                    radius: 22
                    color: root.fieldFillColor
                    border.color: "transparent"
                }
                onTextEdited: root.keywordEdited(text)
                onAccepted: root.searchRequested(text)
            }

            GlassButton {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 42
                text: root.filtersExpanded ? "收起" : "筛选"
                textColor: root.primaryButtonTextColor
                cornerRadius: 16
                normalColor: root.primaryButtonColor
                hoverColor: root.primaryButtonHoverColor
                pressedColor: root.primaryButtonPressedColor
                enabled: root.sourceSelected
                onClicked: root.filtersExpanded = !root.filtersExpanded
            }

            GlassButton {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 42
                text: "搜索"
                textColor: root.primaryButtonTextColor
                cornerRadius: 16
                normalColor: root.primaryButtonColor
                hoverColor: root.primaryButtonHoverColor
                pressedColor: root.primaryButtonPressedColor
                onClicked: root.searchRequested(searchField.text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.filtersExpanded && root.sourceSelected

            Text {
                text: "排序"
                color: root.textMutedColor
                font.pixelSize: 12
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: root.sortOptions
                    delegate: Rectangle {
                        required property var modelData
                        width: sortLabel.implicitWidth + 20
                        height: 30
                        radius: 15
                        color: (modelData.id || "") === root.selectedSort ? root.chipActiveFillColor : root.chipFillColor
                        border.color: root.chipBorderColor
                        Text {
                            id: sortLabel
                            anchors.centerIn: parent
                            text: modelData.label || modelData.id || "默认"
                            color: root.textPrimaryColor
                            font.pixelSize: 12
                            font.bold: (modelData.id || "") === root.selectedSort
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.sortSelected(modelData.id || "")
                        }
                    }
                }
            }

            Text {
                text: "分类 / Tag"
                color: root.textMutedColor
                font.pixelSize: 12
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: root.tagOptions
                    delegate: Rectangle {
                        required property var modelData
                        width: tagLabel.implicitWidth + 20
                        height: 30
                        radius: 15
                        color: (modelData.id || "") === root.selectedTag ? root.chipActiveFillColor : root.chipFillColor
                        border.color: root.chipBorderColor
                        Text {
                            id: tagLabel
                            anchors.centerIn: parent
                            text: modelData.label || modelData.id || "全部"
                            color: root.textPrimaryColor
                            font.pixelSize: 12
                            font.bold: (modelData.id || "") === root.selectedTag
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.tagSelected(modelData.id || "")
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: {
                    var parts = []
                    if (root.keyword.length > 0)
                        parts.push("搜索: " + root.keyword)
                    if (root.selectedTag && root.selectedTag.length > 0)
                        parts.push("tag: " + root.selectedTag)
                    if (parts.length === 0)
                        return root.sourceSelected ? "搜索漫画" : "请先导入漫画源"
                    return parts.join(" · ")
                }
                color: root.textPrimaryColor
                font.pixelSize: 22
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                text: root.comicCount + " 项"
                color: root.textMutedColor
                font.pixelSize: 13
            }
        }

        GridView {
            id: comicGridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            cacheBuffer: Math.max(height, 720)
            cellWidth: Math.max(170, Math.floor(width / Math.max(1, Math.floor(width / 190))))
            cellHeight: 294
            model: root.comicModel
            ScrollBar.vertical: GlassScrollBar {}
            onContentYChanged: root.loadMoreProbe(comicGridView)
            delegate: root.comicDelegate
        }

        Text {
            Layout.fillWidth: true
            visible: root.comicCount === 0
            text: root.sourceSelected ? "没有结果" : "请先导入漫画源"
            color: root.textMutedColor
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
