import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property var r18Controller: null
    property Component comicDelegate: null
    property int sourceViewMode: 0
    property bool sourceHelpVisible: false
    property string sourceCatalogInput: ""
    property string sourceFeedKeyword: ""
    property var sourceTagBuckets: []
    property color panelFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color fieldFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color itemSelectedFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.30)
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    property color homeCardFillColor: "#ffffff"
    property color homeCardStrokeColor: "#d8dde6"
    property color homeFieldStrokeColor: "#d7dce5"
    property color homeBadgeFillColor: "#dce6f8"
    property color homeBadgeTextColor: "#526173"
    property color homeImportButtonColor: "#0c4f92"
    property color homeImportButtonHoverColor: "#0f61b0"
    property color homeImportButtonPressedColor: "#093d72"
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color primaryButtonTextColor: "#203246"
    property color placeholderColor: "#8493a3"

    readonly property string currentSourceId: root.r18Controller ? root.r18Controller.currentSourceId : ""
    readonly property bool loading: root.r18Controller && root.r18Controller.loading

    signal backRequested()
    signal sourceViewModeChangedByUser(int mode)
    signal sourceHelpToggled()
    signal sourceCatalogInputEdited(string text)
    signal officialCatalogRequested()
    signal officialCatalogRefreshRequested()
    signal sourceCatalogPathRequested()
    signal officialSourceImportRequested(int sourceIndex)
    signal importedSourceOpenRequested(string sourceId)
    signal sourceEnabledChanged(string sourceId, bool enabled)
    signal sourceFeedKeywordEdited(string keyword)
    signal sourceFeedRequested(string keyword)
    signal sourceTagsRequested()
    signal tagSearchRequested(string tag)
    signal loadMoreProbe(var gridView)

    function modelCount(model) {
        return model && model.count !== undefined ? model.count : 0
    }

    function sourceStatusText(status, format) {
        var statusText = status || "ok"
        var formatText = format || "native"
        if (statusText === "staged-js") {
            return formatText + " · 已导入，等待 MemoChat JS 源运行适配"
        }
        if (statusText === "staged") {
            return formatText + " · 已暂存"
        }
        return formatText + " · " + statusText
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Item {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44

                Rectangle {
                    anchors.centerIn: parent
                    width: 38
                    height: 38
                    radius: 19
                    color: sourceBackMouse.containsMouse ? Qt.rgba(0.86, 0.91, 1.0, 0.52) : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "←"
                        color: root.textPrimaryColor
                        font.pixelSize: 28
                    }
                }

                MouseArea {
                    id: sourceBackMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.backRequested()
                }
            }

            Text {
                Layout.fillWidth: true
                text: "漫画源"
                color: root.textPrimaryColor
                font.pixelSize: 25
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                text: root.currentSourceId.length > 0 ? "当前: " + root.currentSourceId : "当前: 未选择"
                color: root.textMutedColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Repeater {
                model: [
                    { "title": "添加", "mode": 0, "width": 92 },
                    { "title": "官方目录", "mode": 1, "width": 104 },
                    { "title": "漫画", "mode": 2, "width": 92 },
                    { "title": "标签", "mode": 3, "width": 92 }
                ]

                delegate: GlassButton {
                    Layout.preferredWidth: modelData.width
                    Layout.preferredHeight: 34
                    text: modelData.title
                    textPixelSize: 13
                    textColor: root.sourceViewMode === modelData.mode ? root.textPrimaryColor : root.textSecondaryColor
                    cornerRadius: 17
                    normalColor: root.sourceViewMode === modelData.mode ? root.primaryButtonColor : root.secondaryButtonColor
                    hoverColor: root.sourceViewMode === modelData.mode ? root.primaryButtonHoverColor : root.secondaryButtonHoverColor
                    pressedColor: root.sourceViewMode === modelData.mode ? root.primaryButtonPressedColor : root.secondaryButtonPressedColor
                    onClicked: {
                        if (modelData.mode === 2) {
                            root.sourceFeedRequested("")
                        } else if (modelData.mode === 3) {
                            root.sourceTagsRequested()
                        } else {
                            root.sourceViewModeChangedByUser(modelData.mode)
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.sourceViewMode

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 2
                    anchors.rightMargin: 2
                    spacing: 18

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 22

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Image {
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                source: "qrc:/icons/r18_datasource.png"
                                fillMode: Image.PreserveAspectFit
                                opacity: 0.88
                                mipmap: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "添加漫画源"
                                color: root.textPrimaryColor
                                font.pixelSize: 18
                                elide: Text.ElideRight
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                Layout.fillWidth: true
                                text: "URL"
                                color: root.textPrimaryColor
                                font.pixelSize: 15
                                elide: Text.ElideRight
                            }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 34
                                text: root.sourceCatalogInput
                                color: root.textPrimaryColor
                                selectByMouse: true
                                leftPadding: 14
                                rightPadding: 14
                                font.pixelSize: 15
                                background: Rectangle {
                                    color: "transparent"
                                    border.color: "transparent"
                                }
                                onTextEdited: root.sourceCatalogInputEdited(text)
                                onAccepted: root.officialCatalogRefreshRequested()
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.homeFieldStrokeColor
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        GlassButton {
                            Layout.preferredWidth: 168
                            Layout.preferredHeight: 40
                            text: "漫画源列表"
                            textPixelSize: 14
                            textColor: root.textSecondaryColor
                            cornerRadius: 20
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: root.officialCatalogRequested()
                        }

                        GlassButton {
                            Layout.preferredWidth: 174
                            Layout.preferredHeight: 40
                            text: "使用配置文件"
                            textPixelSize: 14
                            textColor: root.textSecondaryColor
                            cornerRadius: 20
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: root.sourceCatalogPathRequested()
                        }

                        GlassButton {
                            Layout.preferredWidth: 118
                            Layout.preferredHeight: 40
                            text: "帮助"
                            textPixelSize: 14
                            textColor: root.textSecondaryColor
                            cornerRadius: 20
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: root.sourceHelpToggled()
                        }

                        GlassButton {
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 40
                            text: "检查更新"
                            textPixelSize: 14
                            textColor: root.textSecondaryColor
                            cornerRadius: 20
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: root.officialCatalogRefreshRequested()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.sourceHelpVisible
                        text: "该地址应指向 index.json；也可以输入本地目录，例如 D:\\Venera，刷新时会自动读取目录下的 index.json。"
                        color: root.textMutedColor
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "已导入漫画源"
                        color: root.textPrimaryColor
                        font.pixelSize: 18
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.modelCount(root.r18Controller ? root.r18Controller.sourceModel : null) === 0
                                 && !root.loading
                        text: "暂无已导入漫画源，导入后会显示在这里"
                        color: root.textMutedColor
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: root.r18Controller ? root.r18Controller.sourceModel : null
                        ScrollBar.vertical: GlassScrollBar {}

                        delegate: R18ImportedSourceRow {
                            sourceId: model.sourceId || ""
                            title: model.title || ""
                            itemId: model.itemId || ""
                            statusText: root.sourceStatusText(model.status, model.format)
                            sourceUrl: model.sourceUrl || model.url || model.message || ""
                            enabledState: model.enabled === undefined ? true : model.enabled
                            hasEnabledState: model.enabled !== undefined
                            selected: root.currentSourceId === model.sourceId
                            itemFillColor: root.itemFillColor
                            itemHoverFillColor: root.itemHoverFillColor
                            itemSelectedFillColor: root.itemSelectedFillColor
                            fieldStrokeColor: root.fieldStrokeColor
                            textPrimaryColor: root.textPrimaryColor
                            textSecondaryColor: root.textSecondaryColor
                            textMutedColor: root.textMutedColor
                            secondaryButtonColor: root.secondaryButtonColor
                            secondaryButtonHoverColor: root.secondaryButtonHoverColor
                            secondaryButtonPressedColor: root.secondaryButtonPressedColor
                            importButtonColor: root.homeImportButtonColor
                            importButtonHoverColor: root.homeImportButtonHoverColor
                            importButtonPressedColor: root.homeImportButtonPressedColor
                            onEnableToggled: function(sourceId, enabled) {
                                root.sourceEnabledChanged(sourceId, enabled)
                            }
                            onOpenRequested: function(sourceId) {
                                root.importedSourceOpenRequested(sourceId)
                            }
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.sourceHelpVisible ? 232 : 184
                        radius: 8
                        color: root.homeCardFillColor
                        border.color: root.homeCardStrokeColor
                        antialiasing: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 18
                            anchors.topMargin: 16
                            anchors.bottomMargin: 14
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Image {
                                    Layout.preferredWidth: 26
                                    Layout.preferredHeight: 26
                                    source: "qrc:/icons/file.png"
                                    fillMode: Image.PreserveAspectFit
                                    opacity: 0.82
                                    mipmap: true
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: "仓库地址"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 18
                                    elide: Text.ElideRight
                                }
                            }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                text: root.sourceCatalogInput
                                placeholderText: "https://.../index.json 或 D:\\Venera"
                                placeholderTextColor: root.placeholderColor
                                color: root.textPrimaryColor
                                selectByMouse: true
                                leftPadding: 14
                                rightPadding: 14
                                font.pixelSize: 15
                                background: Rectangle {
                                    color: "transparent"
                                    border.color: "transparent"
                                }
                                onTextEdited: root.sourceCatalogInputEdited(text)
                                onAccepted: root.officialCatalogRefreshRequested()
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.homeFieldStrokeColor
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.sourceHelpVisible
                                text: "该地址应指向 index.json；也可以选择本地目录，例如 D:\\Venera，刷新后会读取目录中的 index.json。"
                                color: root.textSecondaryColor
                                font.pixelSize: 13
                                lineHeight: 1.2
                                wrapMode: Text.WordWrap
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.sourceHelpVisible
                                text: "不会默认填入或加载任何仓库地址；需要由用户手动输入或选择目录。"
                                color: root.textMutedColor
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }

                            Item { Layout.fillHeight: true }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                Item { Layout.fillWidth: true }

                                GlassButton {
                                    Layout.preferredWidth: 84
                                    Layout.preferredHeight: 38
                                    text: "帮助"
                                    textPixelSize: 14
                                    textColor: "#0c4f92"
                                    cornerRadius: 19
                                    normalColor: "transparent"
                                    hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                                    pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.26)
                                    onClicked: root.sourceHelpToggled()
                                }

                                GlassButton {
                                    Layout.preferredWidth: 96
                                    Layout.preferredHeight: 38
                                    text: "刷新"
                                    textPixelSize: 14
                                    textColor: root.textSecondaryColor
                                    cornerRadius: 19
                                    normalColor: root.secondaryButtonColor
                                    hoverColor: root.secondaryButtonHoverColor
                                    pressedColor: root.secondaryButtonPressedColor
                                    onClicked: root.officialCatalogRefreshRequested()
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.modelCount(root.r18Controller ? root.r18Controller.officialSourceModel : null) === 0
                                 && !root.loading
                        text: "暂无漫画源，刷新仓库地址后会出现在这里"
                        color: root.textMutedColor
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: root.r18Controller ? root.r18Controller.officialSourceModel : null
                        ScrollBar.vertical: GlassScrollBar {}

                        delegate: R18OfficialSourceRow {
                            sourceIndex: index
                            title: model.title || ""
                            itemId: model.itemId || ""
                            statusText: model.status || model.message || model.url || ""
                            textPrimaryColor: root.textPrimaryColor
                            textSecondaryColor: root.textSecondaryColor
                            importButtonColor: root.homeImportButtonColor
                            importButtonHoverColor: root.homeImportButtonHoverColor
                            importButtonPressedColor: root.homeImportButtonPressedColor
                            onImportRequested: function(sourceIndex) {
                                root.officialSourceImportRequested(sourceIndex)
                            }
                        }
                    }
                }
            }

            R18SourceFeedPane {
                comicModel: root.r18Controller ? root.r18Controller.comicModel : null
                comicDelegate: root.comicDelegate
                currentSourceId: root.currentSourceId
                keyword: root.sourceFeedKeyword
                comicCount: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null)
                loading: root.loading
                fieldFillColor: root.fieldFillColor
                fieldStrokeColor: root.fieldStrokeColor
                textPrimaryColor: root.textPrimaryColor
                textSecondaryColor: root.textSecondaryColor
                textMutedColor: root.textMutedColor
                placeholderColor: root.placeholderColor
                primaryButtonTextColor: root.primaryButtonTextColor
                primaryButtonColor: root.primaryButtonColor
                primaryButtonHoverColor: root.primaryButtonHoverColor
                primaryButtonPressedColor: root.primaryButtonPressedColor
                secondaryButtonColor: root.secondaryButtonColor
                secondaryButtonHoverColor: root.secondaryButtonHoverColor
                secondaryButtonPressedColor: root.secondaryButtonPressedColor
                onKeywordEdited: function(keyword) { root.sourceFeedKeywordEdited(keyword) }
                onRefreshRequested: root.sourceFeedRequested(root.sourceFeedKeyword)
                onSearchRequested: root.sourceFeedRequested(root.sourceFeedKeyword)
                onLoadMoreProbe: function(gridView) { root.loadMoreProbe(gridView) }
            }

            R18TagPane {
                tagBuckets: root.sourceTagBuckets
                loading: root.loading
                sourceSelected: root.currentSourceId.length > 0
                itemFillColor: root.itemFillColor
                itemHoverFillColor: root.itemHoverFillColor
                fieldStrokeColor: root.fieldStrokeColor
                textPrimaryColor: root.textPrimaryColor
                textSecondaryColor: root.textSecondaryColor
                textMutedColor: root.textMutedColor
                badgeFillColor: root.homeBadgeFillColor
                badgeTextColor: root.homeBadgeTextColor
                secondaryButtonColor: root.secondaryButtonColor
                secondaryButtonHoverColor: root.secondaryButtonHoverColor
                secondaryButtonPressedColor: root.secondaryButtonPressedColor
                onReloadRequested: root.sourceFeedRequested("")
                onTagSelected: function(tag) { root.tagSearchRequested(tag) }
            }
        }
    }
}
