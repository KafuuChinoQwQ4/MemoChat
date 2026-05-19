import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

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
    property string sourcePackagePathText: ""
    property string sourcePackageManifestText: ""
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
        if (!url) {
            return ""
        }
        if (url.indexOf("http://") === 0 || url.indexOf("https://") === 0 || url.indexOf("qrc:/") === 0) {
            return url
        }
        if (url.indexOf("/") === 0 && root.gatewayBaseUrl.length > 0) {
            return root.gatewayBaseUrl + url
        }
        return url
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

    function chooseLocalSourcePackage() {
        if (!root.r18Controller) {
            return ""
        }
        var path = root.r18Controller.pickSourcePackage()
        if (path && path.length > 0) {
            root.sourcePackagePathText = path
        }
        return path
    }

    function importChosenSourcePackage() {
        if (!root.r18Controller) {
            return
        }
        var path = root.chooseLocalSourcePackage()
        if (path && path.length > 0) {
            root.r18Controller.importSourcePackage(path, root.sourcePackageManifestText)
        }
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
        if (!tags) {
            return []
        }
        if (typeof tags === "string") {
            return tags.length > 0 ? [tags] : []
        }
        var values = []
        for (var i = 0; i < tags.length; ++i) {
            var tag = String(tags[i] || "").trim()
            if (tag.length > 0) {
                values.push(tag)
            }
        }
        return values
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
        var counts = {}
        var model = root.r18Controller.comicModel
        var total = model.count !== undefined ? model.count : 0
        for (var i = 0; i < total; ++i) {
            var item = model.get(i)
            if (!item) {
                continue
            }
            var tags = normalizeTagArray(item.tags)
            for (var j = 0; j < tags.length; ++j) {
                var key = tags[j]
                if (!counts[key]) {
                    counts[key] = 0
                }
                counts[key] += 1
            }
        }
        var buckets = []
        for (var name in counts) {
            buckets.push({ "name": name, "count": counts[name] })
        }
        buckets.sort(function(a, b) {
            if (b.count !== a.count) {
                return b.count - a.count
            }
            return a.name.localeCompare(b.name)
        })
        root.sourceTagBuckets = buckets
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
        var atBottom = gridView.contentHeight <= gridView.height
                     || gridView.contentY + gridView.height >= gridView.contentHeight - 48
        if (!atBottom) {
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
        return model && model.count !== undefined ? model.count : 0
    }

    function currentComicId() {
        if (!root.r18Controller || !root.r18Controller.currentComic) {
            return ""
        }
        return root.r18Controller.currentComic.comic_id || root.r18Controller.currentComic.id || ""
    }

    function currentComicTitle() {
        if (!root.r18Controller || !root.r18Controller.currentComic) {
            return ""
        }
        return root.r18Controller.currentComic.title || root.r18Controller.currentComic.name || ""
    }

    function searchNow() {
        var activeField = root.viewMode === 0 ? homeSearchField : searchField
        root.keyword = activeField ? activeField.text : root.keyword
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
            root.r18Controller.refreshSources()
            root.r18Controller.refreshHistory()
            if (root.r18Controller.currentSourceId.length > 0) {
                root.r18Controller.search("", 1)
            }
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

                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 12

                            TextField {
                                id: homeSearchField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 56
                                placeholderText: "搜索"
                                placeholderTextColor: root.placeholderColor
                                color: root.textPrimaryColor
                                text: root.keyword
                                selectByMouse: true
                                leftPadding: 50
                                rightPadding: 18
                                font.pixelSize: 16
                                background: Rectangle {
                                    radius: 28
                                    color: root.homeFieldFillColor
                                    border.color: root.homeFieldStrokeColor
                                }
                                onAccepted: root.searchNow()

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 18
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "⌕"
                                    color: root.textMutedColor
                                    font.pixelSize: 24
                                }
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 12
                                model: [
                                    { "title": "历史", "count": root.modelCount(root.r18Controller ? root.r18Controller.historyModel : null), "mode": 5, "action": "mode" },
                                    { "title": "本地", "count": 0, "mode": 4, "action": "import" },
                                    { "title": "追更", "count": root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null), "mode": 1, "action": "mode" },
                                    { "title": "漫画源", "count": root.modelCount(root.r18Controller ? root.r18Controller.sourceModel : null), "mode": 4, "action": "mode" },
                                    { "title": "图片收藏", "count": root.r18Controller && root.r18Controller.currentFavorite ? 1 : 0, "mode": 1, "action": "favorite" }
                                ]
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: modelData.action === "import" ? 106 : 72
                                    radius: 10
                                    color: homeCardMouse.containsMouse ? Qt.rgba(0.985, 0.988, 0.992, 1.0) : root.homeCardFillColor
                                    border.color: root.homeCardStrokeColor

                                    MouseArea {
                                        id: homeCardMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (modelData.action === "import") {
                                                root.importChosenSourcePackage()
                                            } else if (modelData.action === "favorite") {
                                                if (root.currentComicId().length > 0) {
                                                    root.viewMode = 2
                                                } else {
                                                    root.activateMode(1)
                                                }
                                            } else {
                                                root.activateMode(modelData.mode)
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 18
                                        anchors.rightMargin: 18
                                        anchors.topMargin: 16
                                        anchors.bottomMargin: modelData.action === "import" ? 16 : 14
                                        spacing: 8

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 8

                                            Text {
                                                Layout.preferredWidth: implicitWidth
                                                text: modelData.title
                                                color: root.textPrimaryColor
                                                font.pixelSize: 17
                                                elide: Text.ElideRight
                                            }

                                            Rectangle {
                                                visible: modelData.count !== undefined
                                                Layout.preferredWidth: countText.implicitWidth + 18
                                                Layout.preferredHeight: 26
                                                radius: 9
                                                color: root.homeBadgeFillColor

                                                Text {
                                                    id: countText
                                                    anchors.centerIn: parent
                                                    text: modelData.count.toString()
                                                    color: root.homeBadgeTextColor
                                                    font.pixelSize: 12
                                                }
                                            }

                                            Item { Layout.fillWidth: true }

                                            Text {
                                                text: "›"
                                                color: root.homeArrowColor
                                                font.pixelSize: 24
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            visible: modelData.action === "import"
                                        }
                                    }

                                        GlassButton {
                                            visible: modelData.action === "import"
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.rightMargin: 18
                                        anchors.bottomMargin: 14
                                        width: 92
                                        height: 40
                                        text: "导入"
                                        textPixelSize: 14
                                        textColor: "#ffffff"
                                        cornerRadius: 18
                                            normalColor: root.homeImportButtonColor
                                            hoverColor: root.homeImportButtonHoverColor
                                            pressedColor: root.homeImportButtonPressedColor
                                            onClicked: root.importChosenSourcePackage()
                                        }
                                }
                            }
                        }
                    }

                    Item {
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
                                    onAccepted: root.searchNow()
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
                                    onClicked: root.searchNow()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: root.keyword.length > 0
                                          ? "搜索: " + root.keyword
                                          : (root.r18Controller && root.r18Controller.currentSourceId.length > 0 ? "搜索漫画" : "请先导入漫画源")
                                    color: root.textPrimaryColor
                                    font.pixelSize: 24
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null) + " 项"
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
                                model: root.r18Controller ? root.r18Controller.comicModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                onContentYChanged: root.maybeLoadMoreComics(comicGridView)
                                delegate: comicTileDelegate
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null) === 0
                                text: root.r18Controller && root.r18Controller.currentSourceId.length > 0 ? "没有结果" : "请先导入漫画源"
                                color: root.textMutedColor
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    Item {
                        RowLayout {
                            anchors.fill: parent
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: Math.min(240, parent.width * 0.34)
                                Layout.fillHeight: true
                                radius: 12
                                color: root.panelStrongFillColor
                                border.color: root.fieldStrokeColor
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    source: root.absoluteUrl(root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.cover || "" : "")
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    GlassButton {
                                        Layout.preferredWidth: 72
                                        Layout.preferredHeight: 32
                                        text: "返回"
                                        textColor: root.textSecondaryColor
                                        cornerRadius: 8
                                        normalColor: root.secondaryButtonColor
                                        hoverColor: root.secondaryButtonHoverColor
                                        pressedColor: root.secondaryButtonPressedColor
                                        onClicked: root.viewMode = 1
                                    }
                                    GlassButton {
                                        Layout.preferredWidth: 86
                                        Layout.preferredHeight: 32
                                        text: root.r18Controller && root.r18Controller.currentFavorite ? "已收藏" : "收藏"
                                        textColor: root.primaryButtonTextColor
                                        cornerRadius: 8
                                        normalColor: root.primaryButtonColor
                                        hoverColor: root.primaryButtonHoverColor
                                        pressedColor: root.primaryButtonPressedColor
                                        onClicked: {
                                            if (root.r18Controller) {
                                                root.r18Controller.toggleFavorite(root.r18Controller.currentSourceId,
                                                                                  root.currentComicId(),
                                                                                  !root.r18Controller.currentFavorite)
                                            }
                                        }
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: root.currentComicTitle()
                                    color: root.textPrimaryColor
                                    font.pixelSize: 24
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.description || "" : ""
                                    color: root.textSecondaryColor
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 4
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: "章节"
                                    color: root.textSecondaryColor
                                    font.pixelSize: 15
                                    font.bold: true
                                }

                                ListView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    reuseItems: true
                                    cacheBuffer: 480
                                    spacing: 6
                                    model: root.r18Controller ? root.r18Controller.chapterModel : null
                                    ScrollBar.vertical: GlassScrollBar {}
                                    delegate: Rectangle {
                                        width: ListView.view.width
                                        height: 48
                                        radius: 8
                                        color: chapterMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                                        border.color: root.fieldStrokeColor
                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.leftMargin: 12
                                            anchors.rightMargin: 12
                                            text: model.title
                                            color: root.textPrimaryColor
                                            font.pixelSize: 14
                                            elide: Text.ElideRight
                                        }
                                        MouseArea {
                                            id: chapterMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.openChapter(model.sourceId, model.itemId)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: Qt.rgba(0.10, 0.13, 0.18, 0.28)
                            border.color: root.panelStrokeColor
                            clip: true

                            ListView {
                                id: readerList
                                anchors.fill: parent
                                anchors.margins: 8
                                clip: true
                                reuseItems: true
                                cacheBuffer: Math.max(height * 2, 1200)
                                spacing: 10
                                model: root.r18Controller ? root.r18Controller.pageModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: Image {
                                    width: ListView.view.width
                                    height: Math.max(420, width * 1.48)
                                    source: root.absoluteUrl(model.url)
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true

                                    MouseArea {
                                        anchors.fill: parent
                                        preventStealing: false
                                        onClicked: root.readerChromeVisible = !root.readerChromeVisible
                                    }
                                }
                                onMovementEnded: {
                                    var idx = indexAt(width / 2, contentY + height / 2)
                                    if (idx >= 0) {
                                        root.commitReaderProgress(idx + 1)
                                    }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                height: root.readerChromeVisible ? 56 : 0
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
                                        onClicked: root.viewMode = 2
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: root.currentComicTitle()
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
                                height: root.readerChromeVisible ? 58 : 0
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
                                        text: root.r18Controller ? ("P " + root.r18Controller.currentPageIndex + " / " + root.modelCount(root.r18Controller.pageModel)) : ""
                                        color: root.textPrimaryColor
                                        font.pixelSize: 13
                                    }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 1
                                        to: Math.max(1, root.modelCount(root.r18Controller ? root.r18Controller.pageModel : null))
                                        stepSize: 1
                                        value: root.r18Controller ? root.r18Controller.currentPageIndex : 1
                                        onMoved: {
                                            var page = Math.max(1, Math.round(value))
                                            readerList.positionViewAtIndex(page - 1, ListView.Beginning)
                                            root.commitReaderProgress(page)
                                        }
                                    }
                                    GlassButton {
                                        Layout.preferredWidth: 72
                                        Layout.preferredHeight: 32
                                        text: root.r18Controller && root.r18Controller.currentFavorite ? "已收藏" : "收藏"
                                        textColor: root.primaryButtonTextColor
                                        cornerRadius: 8
                                        normalColor: root.primaryButtonColor
                                        hoverColor: root.primaryButtonHoverColor
                                        pressedColor: root.primaryButtonPressedColor
                                        onClicked: {
                                            if (root.r18Controller) {
                                                root.r18Controller.toggleFavorite(root.r18Controller.currentSourceId,
                                                                                  root.currentComicId(),
                                                                                  !root.r18Controller.currentFavorite)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
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
                                        onClicked: root.leaveSourceView()
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
                                    text: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                          ? "当前: " + root.r18Controller.currentSourceId
                                          : "当前: 未选择"
                                    color: root.textMutedColor
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 34
                                    text: "添加"
                                    textPixelSize: 13
                                    textColor: root.sourceViewMode === 0 ? root.textPrimaryColor : root.textSecondaryColor
                                    cornerRadius: 17
                                    normalColor: root.sourceViewMode === 0 ? root.primaryButtonColor : root.secondaryButtonColor
                                    hoverColor: root.sourceViewMode === 0 ? root.primaryButtonHoverColor : root.secondaryButtonHoverColor
                                    pressedColor: root.sourceViewMode === 0 ? root.primaryButtonPressedColor : root.secondaryButtonPressedColor
                                    onClicked: root.sourceViewMode = 0
                                }

                                GlassButton {
                                    Layout.preferredWidth: 104
                                    Layout.preferredHeight: 34
                                    text: "官方目录"
                                    textPixelSize: 13
                                    textColor: root.sourceViewMode === 1 ? root.textPrimaryColor : root.textSecondaryColor
                                    cornerRadius: 17
                                    normalColor: root.sourceViewMode === 1 ? root.primaryButtonColor : root.secondaryButtonColor
                                    hoverColor: root.sourceViewMode === 1 ? root.primaryButtonHoverColor : root.secondaryButtonHoverColor
                                    pressedColor: root.sourceViewMode === 1 ? root.primaryButtonPressedColor : root.secondaryButtonPressedColor
                                    onClicked: root.sourceViewMode = 1
                                }

                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 34
                                    text: "漫画"
                                    textPixelSize: 13
                                    textColor: root.sourceViewMode === 2 ? root.textPrimaryColor : root.textSecondaryColor
                                    cornerRadius: 17
                                    normalColor: root.sourceViewMode === 2 ? root.primaryButtonColor : root.secondaryButtonColor
                                    hoverColor: root.sourceViewMode === 2 ? root.primaryButtonHoverColor : root.secondaryButtonHoverColor
                                    pressedColor: root.sourceViewMode === 2 ? root.primaryButtonPressedColor : root.secondaryButtonPressedColor
                                    onClicked: root.openSourceFeed()
                                }

                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 34
                                    text: "标签"
                                    textPixelSize: 13
                                    textColor: root.sourceViewMode === 3 ? root.textPrimaryColor : root.textSecondaryColor
                                    cornerRadius: 17
                                    normalColor: root.sourceViewMode === 3 ? root.primaryButtonColor : root.secondaryButtonColor
                                    hoverColor: root.sourceViewMode === 3 ? root.primaryButtonHoverColor : root.secondaryButtonHoverColor
                                    pressedColor: root.sourceViewMode === 3 ? root.primaryButtonPressedColor : root.secondaryButtonPressedColor
                                    onClicked: root.openSourceTags()
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
                                                    id: sourceCatalogEntryField
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 34
                                                    text: root.sourceCatalogInput
                                                    placeholderText: ""
                                                    color: root.textPrimaryColor
                                                    selectByMouse: true
                                                    leftPadding: 14
                                                    rightPadding: 14
                                                    font.pixelSize: 15
                                                    background: Rectangle {
                                                        color: "transparent"
                                                        border.color: "transparent"
                                                    }
                                                    onTextEdited: root.sourceCatalogInput = text
                                                    onAccepted: root.refreshOfficialSourceCatalog()
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
                                                onClicked: root.openOfficialSourceCatalog()
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
                                                onClicked: {
                                                    if (!root.r18Controller) {
                                                        return
                                                    }
                                                    var path = root.r18Controller.pickSourceCatalogPath()
                                                    if (path && path.length > 0) {
                                                        root.sourceCatalogInput = path
                                                    }
                                                }
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
                                                onClicked: root.toggleSourceHelp()
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
                                                onClicked: root.refreshOfficialSourceCatalog()
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
                                                     && !(root.r18Controller && root.r18Controller.loading)
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

                                            delegate: Rectangle {
                                                width: ListView.view.width
                                                height: 96
                                                radius: 8
                                                color: root.r18Controller && root.r18Controller.currentSourceId === model.sourceId
                                                       ? root.itemSelectedFillColor
                                                       : (importedSourceMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor)
                                                border.color: root.r18Controller && root.r18Controller.currentSourceId === model.sourceId
                                                              ? Qt.rgba(0.28, 0.45, 0.67, 0.42)
                                                              : root.fieldStrokeColor

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 10
                                                    spacing: 12

                                                    ColumnLayout {
                                                        Layout.fillWidth: true
                                                        spacing: 4

                                                        RowLayout {
                                                            Layout.fillWidth: true
                                                            spacing: 8

                                                            Text {
                                                                Layout.fillWidth: true
                                                                text: model.title || model.itemId || "漫画源"
                                                                color: root.textPrimaryColor
                                                                font.pixelSize: 16
                                                                font.bold: true
                                                                elide: Text.ElideRight
                                                            }

                                                            Rectangle {
                                                                visible: model.enabled !== undefined
                                                                Layout.preferredWidth: 56
                                                                Layout.preferredHeight: 22
                                                                radius: 11
                                                                color: model.enabled ? Qt.rgba(0.66, 0.82, 0.70, 0.34) : Qt.rgba(0.90, 0.60, 0.60, 0.26)

                                                                Text {
                                                                    anchors.centerIn: parent
                                                                    text: model.enabled ? "启用" : "停用"
                                                                    color: root.textPrimaryColor
                                                                    font.pixelSize: 11
                                                                }
                                                            }
                                                        }

                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: root.sourceStatusText(model.status, model.format)
                                                            color: root.textSecondaryColor
                                                            font.pixelSize: 12
                                                            elide: Text.ElideRight
                                                        }

                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: model.sourceUrl || model.url || model.message || ""
                                                            color: root.textMutedColor
                                                            font.pixelSize: 11
                                                            elide: Text.ElideRight
                                                        }
                                                    }

                                                    GlassButton {
                                                        Layout.preferredWidth: 72
                                                        Layout.preferredHeight: 34
                                                        visible: model.enabled !== undefined
                                                        text: model.enabled ? "停用" : "启用"
                                                        textPixelSize: 13
                                                        textColor: root.textSecondaryColor
                                                        cornerRadius: 17
                                                        normalColor: root.secondaryButtonColor
                                                        hoverColor: root.secondaryButtonHoverColor
                                                        pressedColor: root.secondaryButtonPressedColor
                                                        onClicked: if (root.r18Controller) root.r18Controller.enableSource(model.sourceId, !model.enabled)
                                                    }

                                                    GlassButton {
                                                        Layout.preferredWidth: 84
                                                        Layout.preferredHeight: 34
                                                        text: "进入"
                                                        textPixelSize: 13
                                                        textColor: "#ffffff"
                                                        cornerRadius: 17
                                                        normalColor: root.homeImportButtonColor
                                                        hoverColor: root.homeImportButtonHoverColor
                                                        pressedColor: root.homeImportButtonPressedColor
                                                        onClicked: if (root.r18Controller) root.openImportedSource(model.sourceId)
                                                    }
                                                }

                                                MouseArea {
                                                    id: importedSourceMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    acceptedButtons: Qt.NoButton
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
                                                    id: sourceCatalogListField
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
                                                    onTextEdited: root.sourceCatalogInput = text
                                                    onAccepted: root.refreshOfficialSourceCatalog()
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
                                                        onClicked: root.toggleSourceHelp()
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
                                                        onClicked: root.refreshOfficialSourceCatalog()
                                                    }
                                                }
                                            }
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            visible: root.modelCount(root.r18Controller ? root.r18Controller.officialSourceModel : null) === 0
                                                     && !(root.r18Controller && root.r18Controller.loading)
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

                                            delegate: Rectangle {
                                                width: ListView.view.width
                                                height: 72
                                                radius: 8
                                                color: officialSourceMouse.containsMouse ? Qt.rgba(0.94, 0.97, 1.0, 0.74) : "transparent"
                                                border.color: "transparent"

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8
                                                    anchors.rightMargin: 18
                                                    spacing: 12

                                                    ColumnLayout {
                                                        Layout.fillWidth: true
                                                        spacing: 3

                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: model.title || model.itemId || "漫画源"
                                                            color: root.textPrimaryColor
                                                            font.pixelSize: 17
                                                            elide: Text.ElideRight
                                                        }

                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: model.status || model.message || model.url || ""
                                                            color: root.textSecondaryColor
                                                            font.pixelSize: 13
                                                            elide: Text.ElideRight
                                                        }
                                                    }

                                                    GlassButton {
                                                        Layout.preferredWidth: 96
                                                        Layout.preferredHeight: 40
                                                        text: "添加"
                                                        textPixelSize: 15
                                                        textColor: "#ffffff"
                                                        cornerRadius: 20
                                                        normalColor: root.homeImportButtonColor
                                                        hoverColor: root.homeImportButtonHoverColor
                                                        pressedColor: root.homeImportButtonPressedColor
                                                        onClicked: if (root.r18Controller) root.r18Controller.importOfficialSource(index)
                                                    }
                                                }

                                                MouseArea {
                                                    id: officialSourceMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    acceptedButtons: Qt.NoButton
                                                }
                                            }
                                        }
                                    }
                                }

                                Item {
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
                                                    text: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                                          ? root.r18Controller.currentSourceId + " · " + root.modelCount(root.r18Controller.comicModel) + " 项"
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
                                                enabled: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                                onClicked: root.loadSourceFeed(root.sourceFeedKeyword)
                                            }
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 10

                                            TextField {
                                                id: sourceFeedSearchField
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 40
                                                text: root.sourceFeedKeyword
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
                                                onTextEdited: root.sourceFeedKeyword = text
                                                onAccepted: root.loadSourceFeed(root.sourceFeedKeyword)
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
                                                enabled: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                                onClicked: root.loadSourceFeed(root.sourceFeedKeyword)
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
                                            model: root.r18Controller ? root.r18Controller.comicModel : null
                                            ScrollBar.vertical: GlassScrollBar {}
                                            onContentYChanged: root.maybeLoadMoreComics(sourceComicGridView)
                                            delegate: comicTileDelegate
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            visible: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null) === 0
                                                     && !(root.r18Controller && root.r18Controller.loading)
                                            text: root.r18Controller && root.r18Controller.currentSourceId.length > 0 ? "当前源暂无漫画" : "请先选择已导入漫画源"
                                            color: root.textMutedColor
                                            font.pixelSize: 13
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }

                                Item {
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
                                                    text: root.sourceTagBuckets.length + " 个标签 · 点击后在当前源中查找"
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
                                                enabled: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                                onClicked: root.loadSourceFeed("")
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
                                            model: root.sourceTagBuckets
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
                                                        color: root.homeBadgeFillColor

                                                        Text {
                                                            id: tagCountText
                                                            anchors.centerIn: parent
                                                            text: modelData.count
                                                            color: root.homeBadgeTextColor
                                                            font.pixelSize: 11
                                                        }
                                                    }
                                                }

                                                MouseArea {
                                                    id: tagMouse
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: root.searchByTag(modelData.name)
                                                }
                                            }
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            visible: root.sourceTagBuckets.length === 0
                                                     && !(root.r18Controller && root.r18Controller.loading)
                                            text: root.r18Controller && root.r18Controller.currentSourceId.length > 0
                                                  ? "当前加载的漫画还没有 tag，可先刷新漫画列表"
                                                  : "请先选择已导入漫画源"
                                            color: root.textMutedColor
                                            font.pixelSize: 13
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: "阅读历史"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 24
                                    font.bold: true
                                }
                                GlassButton {
                                    Layout.preferredWidth: 76
                                    Layout.preferredHeight: 32
                                    text: "刷新"
                                    textColor: root.textSecondaryColor
                                    cornerRadius: 8
                                    normalColor: root.secondaryButtonColor
                                    hoverColor: root.secondaryButtonHoverColor
                                    pressedColor: root.secondaryButtonPressedColor
                                    onClicked: if (root.r18Controller) root.r18Controller.refreshHistory()
                                }
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                reuseItems: true
                                cacheBuffer: 480
                                spacing: 8
                                model: root.r18Controller ? root.r18Controller.historyModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 72
                                    radius: 10
                                    color: historyMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                                    border.color: root.fieldStrokeColor
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 10
                                        Image {
                                            Layout.preferredWidth: 48
                                            Layout.preferredHeight: 56
                                            source: root.absoluteUrl(model.cover)
                                            fillMode: Image.PreserveAspectCrop
                                            asynchronous: true
                                        }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 3
                                            Text {
                                                Layout.fillWidth: true
                                                text: model.title || model.itemId || "阅读记录"
                                                color: root.textPrimaryColor
                                                font.pixelSize: 14
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: model.subtitle || model.sourceId
                                                color: root.textMutedColor
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                    MouseArea {
                                        id: historyMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.openComic(model.sourceId, model.itemId)
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.modelCount(root.r18Controller ? root.r18Controller.historyModel : null) === 0
                                text: "暂无阅读历史"
                                color: root.textMutedColor
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
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

                Rectangle {
                    id: statusBanner
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: statusText.visible ? statusText.implicitHeight + 18 : 0
                    radius: 8
                    color: Qt.rgba(0.54, 0.70, 0.93, 0.24)
                    border.color: Qt.rgba(0.28, 0.45, 0.67, 0.28)
                    visible: root.r18Controller
                             && root.viewMode !== 4
                             && root.r18Controller.error.length === 0
                             && root.r18Controller.statusText.length > 0
                    z: 20

                    Text {
                        id: statusText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        visible: root.r18Controller
                                 && root.r18Controller.error.length === 0
                                 && root.r18Controller.statusText.length > 0
                        text: root.r18Controller ? root.r18Controller.statusText : ""
                        color: root.textSecondaryColor
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }
                }

                Rectangle {
                    id: errorBanner
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: errorText.visible ? errorText.implicitHeight + 18 : 0
                    radius: 8
                    color: Qt.rgba(0.92, 0.62, 0.68, 0.26)
                    border.color: Qt.rgba(0.54, 0.18, 0.24, 0.28)
                    visible: root.r18Controller && root.r18Controller.error.length > 0
                    z: 21

                    Text {
                        id: errorText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        visible: root.r18Controller && root.r18Controller.error.length > 0
                        text: root.r18Controller ? root.r18Controller.error : ""
                        color: "#6f1f2c"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    Component {
        id: comicTileDelegate

        Rectangle {
            width: GridView.view.cellWidth - 10
            height: 280
            property var tileTags: root.normalizeTagArray(model.tags)
            radius: 10
            clip: true
            color: tileMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
            border.color: root.fieldStrokeColor

            Image {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 176
                source: root.absoluteUrl(model.cover)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 104
                color: root.panelFillColor
                border.color: root.fieldStrokeColor
                Column {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 5
                    Text {
                        width: parent.width
                        text: model.title
                        color: root.textPrimaryColor
                        font.pixelSize: 13
                        font.bold: true
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: model.subtitle
                        color: root.textMutedColor
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }

                    Flow {
                        width: parent.width
                        height: 24
                        spacing: 5
                        clip: true

                        Repeater {
                            model: tileTags.slice(0, 3)

                            delegate: Rectangle {
                                width: Math.min(74, tagText.implicitWidth + 14)
                                height: 20
                                radius: 10
                                color: root.homeBadgeFillColor

                                Text {
                                    id: tagText
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 7
                                    anchors.rightMargin: 7
                                    text: modelData
                                    color: root.homeBadgeTextColor
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
            MouseArea {
                id: tileMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openComic(model.sourceId, model.itemId)
            }
        }
    }
}
