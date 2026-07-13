import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"
import "../runtime/R18ShellRuntime.js" as R18ShellRuntime

Item {
    id: root

    property var r18Controller: null
    property var backdrop: null
    property string keyword: ""
    property string activeChapterId: ""
    property int viewMode: 0 // 0 shelf, 1 search, 2 detail, 3 reader, 4 sources, 5 history
    property int sourceViewMode: 0 // 0 add, 1 catalog, 2 comics, 3 tags
    property bool readerChromeVisible: true
    property bool sourceHelpVisible: false
    property string sourceCatalogInput: ""
    property string sourceFeedKeyword: ""
    property bool comicBottomLoadArmed: false
    property var sourceTagBuckets: []
    readonly property string gatewayBaseUrl: typeof gateMediaUrlPrefix === "string" && gateMediaUrlPrefix.length > 0
                                             ? gateMediaUrlPrefix
                                             : (typeof gateUrlPrefix === "string" ? gateUrlPrefix : "")

    readonly property color pageBackgroundColor: "transparent"
    readonly property color panelFillColor: Qt.rgba(1, 1, 1, 0.16)
    readonly property color panelStrongFillColor: Qt.rgba(1, 1, 1, 0.20)
    readonly property color panelStrokeColor: Qt.rgba(1, 1, 1, 0.46)
    readonly property color fieldFillColor: Qt.rgba(1, 1, 1, 0.16)
    readonly property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    readonly property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    readonly property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    readonly property color itemSelectedFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.30)
    readonly property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    readonly property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    readonly property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    readonly property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    readonly property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    readonly property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    readonly property color homeFieldFillColor: "#eceef3"
    readonly property color homeFieldStrokeColor: "#d7dce5"
    readonly property color homeCardFillColor: "#ffffff"
    readonly property color homeCardStrokeColor: "#d8dde6"
    readonly property color homeBadgeFillColor: "#dce6f8"
    readonly property color homeBadgeTextColor: "#526173"
    readonly property color homeArrowColor: "#2f343c"
    readonly property color homeImportButtonColor: "#0c4f92"
    readonly property color homeImportButtonHoverColor: "#0f61b0"
    readonly property color homeImportButtonPressedColor: "#093d72"
    readonly property color textPrimaryColor: "#263241"
    readonly property color textSecondaryColor: "#4e6072"
    readonly property color textMutedColor: "#7b8794"
    readonly property color primaryButtonTextColor: "#203246"
    readonly property color placeholderColor: "#8493a3"

    function absoluteUrl(url) {
        return R18ShellRuntime.absoluteUrl(url, root.gatewayBaseUrl)
    }

    function openOfficialSourceCatalog() {
        root.sourceViewMode = 1
        if (root.r18Controller) {
            root.r18Controller.refreshOfficialSources(root.sourceCatalogInput)
        }
    }

    function refreshOfficialSourceCatalog() {
        if (!root.r18Controller) {
            return
        }
        root.r18Controller.refreshOfficialSources(root.sourceCatalogInput)
    }

    function leaveSourceView() {
        if (root.sourceViewMode === 1) {
            root.sourceViewMode = 0
        } else {
            root.viewMode = 0
        }
    }

    function toggleSourceHelp() {
        root.sourceHelpVisible = !root.sourceHelpVisible
    }

    function normalizeTagArray(tags) {
        return R18ShellRuntime.normalizeTagArray(tags)
    }

    function loadSourceFeed(searchText) {
        if (!root.r18Controller || root.r18Controller.currentSourceId.length === 0) {
            return
        }
        var query = searchText === undefined ? "" : searchText
        root.sourceFeedKeyword = query
        root.comicBottomLoadArmed = false
        root.r18Controller.search(query, 1)
    }

    function openImportedSource(sourceId) {
        if (!root.r18Controller || !sourceId) {
            return
        }
        root.comicBottomLoadArmed = false
        root.r18Controller.selectSource(sourceId)
        root.sourceViewMode = 2
        root.loadSourceFeed()
    }

    function selectSourceAndSearch(sourceId) {
        if (!root.r18Controller || !sourceId) {
            return
        }
        root.comicBottomLoadArmed = false
        root.r18Controller.selectSource(sourceId)
        root.r18Controller.search(root.keyword, 1)
    }

    function rebuildSourceTagBuckets() {
        if (!root.r18Controller || !root.r18Controller.comicModel) {
            root.sourceTagBuckets = []
            return
        }
        root.sourceTagBuckets = R18ShellRuntime.buildSourceTagBuckets(root.r18Controller.comicModel)
    }

    function openSourceFeed() {
        root.sourceViewMode = 2
        root.loadSourceFeed("")
    }

    function openSourceTags() {
        root.sourceViewMode = 3
        if (root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null) === 0) {
            root.loadSourceFeed("")
        }
        root.rebuildSourceTagBuckets()
    }

    function searchByTag(tag) {
        if (!root.r18Controller || !tag) {
            return
        }
        root.viewMode = 4
        root.sourceViewMode = 2
        root.sourceFeedKeyword = tag
        root.comicBottomLoadArmed = false
        root.r18Controller.search(tag, 1)
    }

    function maybeLoadMoreComics(gridView) {
        if (!root.r18Controller || !gridView) {
            return
        }
        if (!R18ShellRuntime.isGridAtBottom(gridView, 48)) {
            root.comicBottomLoadArmed = false
            return
        }
        if (root.comicBottomLoadArmed
                || root.r18Controller.loading
                || !root.r18Controller.currentSearchHasMore
                || root.r18Controller.currentSourceId.length === 0) {
            return
        }
        root.comicBottomLoadArmed = true
        var nextKeyword = root.viewMode === 4 ? root.sourceFeedKeyword : root.keyword
        root.r18Controller.search(nextKeyword, root.r18Controller.currentSearchPage + 1)
    }

    function modelCount(model) {
        return R18ShellRuntime.modelCount(model)
    }

    function currentComicId() {
        return R18ShellRuntime.currentComicId(root.r18Controller ? root.r18Controller.currentComic : null)
    }

    function currentComicTitle() {
        return R18ShellRuntime.currentComicTitle(root.r18Controller ? root.r18Controller.currentComic : null)
    }

    function searchNow() {
        root.comicBottomLoadArmed = false
        root.viewMode = 1
        if (root.r18Controller) {
            root.r18Controller.search(root.keyword, 1)
        }
    }

    function activateMode(mode) {
        root.comicBottomLoadArmed = false
        root.viewMode = mode
        root.refreshModeData(mode)
    }

    function refreshModeData(mode) {
        if (!root.r18Controller) {
            return
        }
        if (mode === 5) {
            root.r18Controller.refreshHistory()
        } else if (mode === 4) {
            root.sourceViewMode = 0
            root.sourceHelpVisible = false
            root.r18Controller.refreshSources()
        }
    }

    onViewModeChanged: refreshModeData(viewMode)
    onSourceViewModeChanged: {
        if (root.sourceViewMode === 2 || root.sourceViewMode === 3) {
            root.rebuildSourceTagBuckets()
        }
    }

    Connections {
        target: root.r18Controller ? root.r18Controller.comicModel : null
        function onCountChanged() {
            root.comicBottomLoadArmed = false
            root.rebuildSourceTagBuckets()
        }
    }

    function openComic(sourceId, comicId) {
        root.readerChromeVisible = true
        root.viewMode = 2
        if (root.r18Controller) {
            root.r18Controller.openComic(sourceId, comicId)
        }
    }

    function openChapter(sourceId, chapterId) {
        root.activeChapterId = chapterId
        root.readerChromeVisible = true
        root.viewMode = 3
        if (root.r18Controller) {
            root.r18Controller.openChapter(sourceId, chapterId)
        }
    }

    function commitReaderProgress(pageIndex) {
        if (!root.r18Controller || pageIndex < 1) {
            return
        }
        root.r18Controller.updateHistory(root.r18Controller.currentSourceId,
                                         root.currentComicId(),
                                         root.activeChapterId,
                                         pageIndex)
    }

    Component.onCompleted: {
        if (root.r18Controller) {
            root.r18Controller.refreshAccess()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.pageBackgroundColor
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: root.backdrop
            cornerRadius: 16
            blurRadius: 20
            blurEnabled: true
            fillColor: root.panelFillColor
            strokeColor: root.panelStrokeColor
            glowTopColor: Qt.rgba(1, 1, 1, 0.22)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)

            Item {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 18
                anchors.bottomMargin: 16

                StackLayout {
                    anchors.fill: parent
                    currentIndex: root.viewMode

                    R18HomePane {
                        keyword: root.keyword
                        historyCount: root.modelCount(root.r18Controller ? root.r18Controller.historyModel : null)
                        followCount: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null)
                        sourceCount: root.modelCount(root.r18Controller ? root.r18Controller.sourceModel : null)
                        favoriteCount: root.r18Controller && root.r18Controller.currentFavorite ? 1 : 0
                        homeFieldFillColor: root.homeFieldFillColor
                        homeFieldStrokeColor: root.homeFieldStrokeColor
                        homeCardFillColor: root.homeCardFillColor
                        homeCardStrokeColor: root.homeCardStrokeColor
                        homeBadgeFillColor: root.homeBadgeFillColor
                        homeBadgeTextColor: root.homeBadgeTextColor
                        homeArrowColor: root.homeArrowColor
                        textPrimaryColor: root.textPrimaryColor
                        textMutedColor: root.textMutedColor
                        placeholderColor: root.placeholderColor
                        onKeywordEdited: function(keyword) { root.keyword = keyword }
                        onSearchRequested: function(keyword) {
                            root.keyword = keyword
                            root.searchNow()
                        }
                        onEntryActivated: function(action, modeValue) {
                            if (action === "favorite") {
                                if (root.currentComicId().length > 0) {
                                    root.viewMode = 2
                                } else {
                                    root.activateMode(1)
                                }
                            } else {
                                root.activateMode(modeValue)
                            }
                        }
                    }

                    R18SearchPane {
                        comicModel: root.r18Controller ? root.r18Controller.comicModel : null
                        comicDelegate: comicTileDelegate
                        keyword: root.keyword
                        comicCount: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null)
                        currentSourceId: root.r18Controller ? root.r18Controller.currentSourceId : ""
                        fieldFillColor: root.fieldFillColor
                        textPrimaryColor: root.textPrimaryColor
                        textMutedColor: root.textMutedColor
                        placeholderColor: root.placeholderColor
                        primaryButtonTextColor: root.primaryButtonTextColor
                        primaryButtonColor: root.primaryButtonColor
                        primaryButtonHoverColor: root.primaryButtonHoverColor
                        primaryButtonPressedColor: root.primaryButtonPressedColor
                        onKeywordEdited: function(keyword) { root.keyword = keyword }
                        onSearchRequested: function(keyword) {
                            root.keyword = keyword
                            root.searchNow()
                        }
                        onLoadMoreProbe: function(gridView) { root.maybeLoadMoreComics(gridView) }
                    }

                    R18DetailPane {
                        coverSource: root.absoluteUrl(root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.cover || "" : "")
                        title: root.currentComicTitle()
                        description: root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.description || "" : ""
                        chapterModel: root.r18Controller ? root.r18Controller.chapterModel : null
                        favorite: root.r18Controller && root.r18Controller.currentFavorite
                        panelStrongFillColor: root.panelStrongFillColor
                        fieldStrokeColor: root.fieldStrokeColor
                        itemFillColor: root.itemFillColor
                        itemHoverFillColor: root.itemHoverFillColor
                        textPrimaryColor: root.textPrimaryColor
                        textSecondaryColor: root.textSecondaryColor
                        primaryButtonTextColor: root.primaryButtonTextColor
                        primaryButtonColor: root.primaryButtonColor
                        primaryButtonHoverColor: root.primaryButtonHoverColor
                        primaryButtonPressedColor: root.primaryButtonPressedColor
                        secondaryButtonColor: root.secondaryButtonColor
                        secondaryButtonHoverColor: root.secondaryButtonHoverColor
                        secondaryButtonPressedColor: root.secondaryButtonPressedColor
                        onBackRequested: root.viewMode = 1
                        onFavoriteToggled: function(favorite) {
                            if (root.r18Controller) {
                                root.r18Controller.toggleFavorite(root.r18Controller.currentSourceId,
                                                                  root.currentComicId(),
                                                                  favorite)
                            }
                        }
                        onChapterOpened: function(sourceId, chapterId) { root.openChapter(sourceId, chapterId) }
                    }

                    R18ReaderPane {
                        pageModel: root.r18Controller ? root.r18Controller.pageModel : null
                        chromeVisible: root.readerChromeVisible
                        title: root.currentComicTitle()
                        currentPageIndex: root.r18Controller ? root.r18Controller.currentPageIndex : 1
                        pageCount: root.modelCount(root.r18Controller ? root.r18Controller.pageModel : null)
                        favorite: root.r18Controller && root.r18Controller.currentFavorite
                        gatewayBaseUrl: root.gatewayBaseUrl
                        panelFillColor: root.panelFillColor
                        panelStrokeColor: root.panelStrokeColor
                        textPrimaryColor: root.textPrimaryColor
                        textSecondaryColor: root.textSecondaryColor
                        primaryButtonTextColor: root.primaryButtonTextColor
                        primaryButtonColor: root.primaryButtonColor
                        primaryButtonHoverColor: root.primaryButtonHoverColor
                        primaryButtonPressedColor: root.primaryButtonPressedColor
                        secondaryButtonColor: root.secondaryButtonColor
                        secondaryButtonHoverColor: root.secondaryButtonHoverColor
                        secondaryButtonPressedColor: root.secondaryButtonPressedColor
                        onBackRequested: root.viewMode = 2
                        onProgressCommitted: function(pageIndex) { root.commitReaderProgress(pageIndex) }
                        onChromeVisibilityChangedByUser: function(visible) { root.readerChromeVisible = visible }
                        onFavoriteToggled: function(favorite) {
                            if (root.r18Controller) {
                                root.r18Controller.toggleFavorite(root.r18Controller.currentSourceId,
                                                                  root.currentComicId(),
                                                                  favorite)
                            }
                        }
                    }

                    R18SourceManagerPane {
                        r18Controller: root.r18Controller
                        comicDelegate: comicTileDelegate
                        sourceViewMode: root.sourceViewMode
                        sourceHelpVisible: root.sourceHelpVisible
                        sourceCatalogInput: root.sourceCatalogInput
                        sourceFeedKeyword: root.sourceFeedKeyword
                        sourceTagBuckets: root.sourceTagBuckets
                        panelFillColor: root.panelFillColor
                        fieldFillColor: root.fieldFillColor
                        fieldStrokeColor: root.fieldStrokeColor
                        itemFillColor: root.itemFillColor
                        itemHoverFillColor: root.itemHoverFillColor
                        itemSelectedFillColor: root.itemSelectedFillColor
                        primaryButtonColor: root.primaryButtonColor
                        primaryButtonHoverColor: root.primaryButtonHoverColor
                        primaryButtonPressedColor: root.primaryButtonPressedColor
                        secondaryButtonColor: root.secondaryButtonColor
                        secondaryButtonHoverColor: root.secondaryButtonHoverColor
                        secondaryButtonPressedColor: root.secondaryButtonPressedColor
                        homeCardFillColor: root.homeCardFillColor
                        homeCardStrokeColor: root.homeCardStrokeColor
                        homeFieldStrokeColor: root.homeFieldStrokeColor
                        homeBadgeFillColor: root.homeBadgeFillColor
                        homeBadgeTextColor: root.homeBadgeTextColor
                        homeImportButtonColor: root.homeImportButtonColor
                        homeImportButtonHoverColor: root.homeImportButtonHoverColor
                        homeImportButtonPressedColor: root.homeImportButtonPressedColor
                        textPrimaryColor: root.textPrimaryColor
                        textSecondaryColor: root.textSecondaryColor
                        textMutedColor: root.textMutedColor
                        primaryButtonTextColor: root.primaryButtonTextColor
                        placeholderColor: root.placeholderColor
                        onBackRequested: root.leaveSourceView()
                        onSourceViewModeChangedByUser: function(mode) { root.sourceViewMode = mode }
                        onSourceHelpToggled: root.toggleSourceHelp()
                        onSourceCatalogInputEdited: function(text) { root.sourceCatalogInput = text }
                        onOfficialCatalogRequested: root.openOfficialSourceCatalog()
                        onOfficialCatalogRefreshRequested: root.refreshOfficialSourceCatalog()
                        onPresetSourceSelected: function(sourceId) {
                            if (root.r18Controller) {
                                root.r18Controller.selectSource(sourceId)
                                root.sourceViewMode = 2
                                root.loadSourceFeed("")
                            }
                        }
                        onSourceCatalogPathRequested: {
                            if (root.r18Controller) {
                                var path = root.r18Controller.pickSourceCatalogPath()
                                if (path && path.length > 0) {
                                    root.sourceCatalogInput = path
                                }
                            }
                        }
                        onImportedSourceOpenRequested: function(sourceId) { root.openImportedSource(sourceId) }
                        onSourceFeedKeywordEdited: function(keyword) { root.sourceFeedKeyword = keyword }
                        onSourceFeedRequested: function(keyword) { root.loadSourceFeed(keyword) }
                        onSourceTagsRequested: root.openSourceTags()
                        onTagSearchRequested: function(tag) { root.searchByTag(tag) }
                        onLoadMoreProbe: function(gridView) { root.maybeLoadMoreComics(gridView) }
                    }

                    R18HistoryPane {
                        historyModel: root.r18Controller ? root.r18Controller.historyModel : null
                        loading: root.r18Controller && root.r18Controller.loading
                        itemFillColor: root.itemFillColor
                        itemHoverFillColor: root.itemHoverFillColor
                        fieldStrokeColor: root.fieldStrokeColor
                        textPrimaryColor: root.textPrimaryColor
                        textSecondaryColor: root.textSecondaryColor
                        textMutedColor: root.textMutedColor
                        secondaryButtonColor: root.secondaryButtonColor
                        secondaryButtonHoverColor: root.secondaryButtonHoverColor
                        secondaryButtonPressedColor: root.secondaryButtonPressedColor
                        onRefreshRequested: if (root.r18Controller) root.r18Controller.refreshHistory()
                        onComicRequested: function(sourceId, itemId) { root.openComic(sourceId, itemId) }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.r18Controller && root.r18Controller.loading
                    text: "加载中"
                    color: root.textPrimaryColor
                    font.pixelSize: 18
                    z: 20
                }

                R18StatusBanner {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    statusText: root.r18Controller ? root.r18Controller.statusText : ""
                    errorText: root.r18Controller ? root.r18Controller.error : ""
                    visible: root.r18Controller
                             && root.viewMode !== 4
                             && (root.r18Controller.error.length > 0
                                 || root.r18Controller.statusText.length > 0)
                    statusVisible: root.r18Controller
                                   && root.viewMode !== 4
                                   && root.r18Controller.error.length === 0
                                   && root.r18Controller.statusText.length > 0
                    textColor: root.textSecondaryColor
                    z: 20
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !root.r18Controller || !root.r18Controller.accessAllowed
        color: root.pageBackgroundColor
        z: 100

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 420)
            spacing: 14

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: {
                    if (!root.r18Controller) {
                        return "登录状态无效"
                    }
                    if (!root.r18Controller.accessResolved) {
                        return root.r18Controller.error.length > 0
                                ? "访问策略暂时不可用" : "正在验证访问权限"
                    }
                    return root.r18Controller.accessRevoked
                            ? "访问权限已被撤销" : "成人内容访问确认"
                }
                color: root.textPrimaryColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                visible: root.r18Controller
                         && root.r18Controller.accessResolved
                         && !root.r18Controller.accessRevoked
                enabled: root.r18Controller && !root.r18Controller.loading
                text: "我已年满 18 岁"
                onClicked: root.r18Controller.attestAdult()
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                visible: root.r18Controller
                         && !root.r18Controller.accessResolved
                         && root.r18Controller.error.length > 0
                text: "重试"
                onClicked: root.r18Controller.refreshAccess()
            }
        }
    }

    Component {
        id: comicTileDelegate

        R18ComicTile {
            width: GridView.view.cellWidth - 10
            coverSource: root.absoluteUrl(model.cover)
            title: model.title
            subtitle: model.subtitle
            tags: root.normalizeTagArray(model.tags)
            itemFillColor: root.itemFillColor
            itemHoverFillColor: root.itemHoverFillColor
            fieldStrokeColor: root.fieldStrokeColor
            panelFillColor: root.panelFillColor
            textPrimaryColor: root.textPrimaryColor
            textMutedColor: root.textMutedColor
            badgeFillColor: root.homeBadgeFillColor
            badgeTextColor: root.homeBadgeTextColor
            onActivated: root.openComic(model.sourceId, model.itemId)
        }
    }
}
