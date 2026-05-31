pragma ComponentBehavior: Bound

import QtQuick 2.15
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
                    required property var modelData

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

                    R18SourceImportPane {
                        Layout.fillWidth: true
                        sourceCatalogInput: root.sourceCatalogInput
                        sourceHelpVisible: root.sourceHelpVisible
                        homeFieldStrokeColor: root.homeFieldStrokeColor
                        textPrimaryColor: root.textPrimaryColor
                        textSecondaryColor: root.textSecondaryColor
                        textMutedColor: root.textMutedColor
                        secondaryButtonColor: root.secondaryButtonColor
                        secondaryButtonHoverColor: root.secondaryButtonHoverColor
                        secondaryButtonPressedColor: root.secondaryButtonPressedColor
                        onSourceCatalogInputEdited: function(text) { root.sourceCatalogInputEdited(text) }
                        onOfficialCatalogRequested: root.officialCatalogRequested()
                        onSourceCatalogPathRequested: root.sourceCatalogPathRequested()
                        onSourceHelpToggled: root.sourceHelpToggled()
                        onOfficialCatalogRefreshRequested: root.officialCatalogRefreshRequested()
                    }

                    R18SourceListPane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        sourceModel: root.r18Controller ? root.r18Controller.sourceModel : null
                        currentSourceId: root.currentSourceId
                        loading: root.loading
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
                        onSourceEnabledChanged: function(sourceId, enabled) {
                            root.sourceEnabledChanged(sourceId, enabled)
                        }
                        onImportedSourceOpenRequested: function(sourceId) {
                            root.importedSourceOpenRequested(sourceId)
                        }
                    }
                }
            }

            R18OfficialSourceCatalogPane {
                officialSourceModel: root.r18Controller ? root.r18Controller.officialSourceModel : null
                sourceCatalogInput: root.sourceCatalogInput
                sourceHelpVisible: root.sourceHelpVisible
                loading: root.loading
                homeCardFillColor: root.homeCardFillColor
                homeCardStrokeColor: root.homeCardStrokeColor
                homeFieldStrokeColor: root.homeFieldStrokeColor
                textPrimaryColor: root.textPrimaryColor
                textSecondaryColor: root.textSecondaryColor
                textMutedColor: root.textMutedColor
                placeholderColor: root.placeholderColor
                secondaryButtonColor: root.secondaryButtonColor
                secondaryButtonHoverColor: root.secondaryButtonHoverColor
                secondaryButtonPressedColor: root.secondaryButtonPressedColor
                importButtonColor: root.homeImportButtonColor
                importButtonHoverColor: root.homeImportButtonHoverColor
                importButtonPressedColor: root.homeImportButtonPressedColor
                onSourceCatalogInputEdited: function(text) { root.sourceCatalogInputEdited(text) }
                onOfficialCatalogRefreshRequested: root.officialCatalogRefreshRequested()
                onSourceHelpToggled: root.sourceHelpToggled()
                onOfficialSourceImportRequested: function(sourceIndex) {
                    root.officialSourceImportRequested(sourceIndex)
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
