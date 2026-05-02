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
    property bool readerChromeVisible: true

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
        return gate_url_prefix + url
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
        root.keyword = searchField.text
        root.viewMode = 1
        if (root.r18Controller) {
            root.r18Controller.search(root.keyword, 1)
        }
    }

    function activateMode(mode) {
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
            root.r18Controller.refreshSources()
            if (root.modelCount(root.r18Controller.officialSourceModel) === 0) {
                root.r18Controller.refreshOfficialSources("")
            }
        }
    }

    onViewModeChanged: refreshModeData(viewMode)

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
            root.r18Controller.search("", 1)
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

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: "书架"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 26
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                GlassButton {
                                    Layout.preferredWidth: 92
                                    Layout.preferredHeight: 34
                                    text: "刷新"
                                    textColor: root.textSecondaryColor
                                    cornerRadius: 9
                                    normalColor: root.secondaryButtonColor
                                    hoverColor: root.secondaryButtonHoverColor
                                    pressedColor: root.secondaryButtonPressedColor
                                    onClicked: {
                                        if (root.r18Controller) {
                                            root.r18Controller.refreshSources()
                                            root.r18Controller.refreshHistory()
                                            root.r18Controller.search(root.keyword, 1)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                spacing: 14

                                Repeater {
                                    model: [
                                        { "title": "搜索结果", "count": root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null), "mode": 1 },
                                        { "title": "最近阅读", "count": root.modelCount(root.r18Controller ? root.r18Controller.historyModel : null), "mode": 5 },
                                        { "title": "漫画源", "count": root.modelCount(root.r18Controller ? root.r18Controller.sourceModel : null), "mode": 4 }
                                    ]
                                    delegate: Rectangle {
                                        Layout.preferredWidth: Math.max(104, statText.implicitWidth + statCount.implicitWidth + 36)
                                        Layout.preferredHeight: 44
                                        radius: 10
                                        color: statMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                                        border.color: root.fieldStrokeColor

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 8
                                            Text {
                                                id: statText
                                                text: modelData.title
                                                color: root.textPrimaryColor
                                                font.pixelSize: 13
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                id: statCount
                                                text: modelData.count.toString()
                                                color: root.textMutedColor
                                                font.pixelSize: 12
                                                font.bold: true
                                            }
                                        }

                                        MouseArea {
                                            id: statMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.viewMode = modelData.mode
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.panelStrokeColor
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "继续阅读"
                                color: root.textSecondaryColor
                                font.pixelSize: 15
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 148
                                clip: true
                                orientation: ListView.Horizontal
                                spacing: 10
                                model: root.r18Controller ? root.r18Controller.historyModel : null
                                visible: count > 0
                                delegate: Rectangle {
                                    width: 140
                                    height: 138
                                    radius: 10
                                    color: root.itemFillColor
                                    border.color: root.fieldStrokeColor
                                    clip: true

                                    Image {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        height: 92
                                        source: root.absoluteUrl(model.cover)
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }
                                    Text {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 8
                                        text: model.title || model.itemId || "阅读记录"
                                        color: root.textPrimaryColor
                                        font.pixelSize: 12
                                        maximumLineCount: 2
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.openComic(model.sourceId, model.itemId)
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 80
                                visible: root.modelCount(root.r18Controller ? root.r18Controller.historyModel : null) === 0
                                radius: 10
                                color: root.itemFillColor
                                border.color: root.fieldStrokeColor
                                Text {
                                    anchors.centerIn: parent
                                    text: "暂无阅读记录"
                                    color: root.textMutedColor
                                    font.pixelSize: 13
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "发现"
                                color: root.textSecondaryColor
                                font.pixelSize: 15
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            GridView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                cellWidth: Math.max(170, Math.floor(width / Math.max(1, Math.floor(width / 190))))
                                cellHeight: 238
                                model: root.r18Controller ? root.r18Controller.comicModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: comicTileDelegate
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
                                    text: root.keyword.length > 0 ? "搜索: " + root.keyword : "搜索漫画"
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
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                cellWidth: Math.max(170, Math.floor(width / Math.max(1, Math.floor(width / 190))))
                                cellHeight: 255
                                model: root.r18Controller ? root.r18Controller.comicModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: comicTileDelegate
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.modelCount(root.r18Controller ? root.r18Controller.comicModel : null) === 0
                                text: "没有结果"
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
                            spacing: 20

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    Layout.fillWidth: true
                                    text: "漫画源"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 24
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                GlassButton {
                                    Layout.preferredWidth: 86
                                    Layout.preferredHeight: 36
                                    text: "刷新"
                                    textColor: root.textSecondaryColor
                                    cornerRadius: 18
                                    normalColor: root.secondaryButtonColor
                                    hoverColor: root.secondaryButtonHoverColor
                                    pressedColor: root.secondaryButtonPressedColor
                                    onClicked: if (root.r18Controller) root.r18Controller.refreshSources()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                radius: 9
                                color: root.itemFillColor
                                border.color: root.fieldStrokeColor

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 20
                                    anchors.rightMargin: 20
                                    spacing: 12

                                    Text {
                                        Layout.preferredWidth: 92
                                        text: "直接拉源"
                                        color: root.textPrimaryColor
                                        font.pixelSize: 19
                                        elide: Text.ElideRight
                                    }
                                    TextField {
                                        id: directSourceUrl
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 40
                                        placeholderText: "漫画源 JS URL"
                                        placeholderTextColor: root.placeholderColor
                                        color: root.textPrimaryColor
                                        text: "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/jm.js"
                                        selectByMouse: true
                                        background: Rectangle {
                                            radius: 20
                                            color: root.fieldFillColor
                                            border.color: "transparent"
                                        }
                                    }
                                    GlassButton {
                                        Layout.preferredWidth: 108
                                        Layout.preferredHeight: 40
                                        text: "拉取JM源"
                                        textColor: root.primaryButtonTextColor
                                        cornerRadius: 20
                                        normalColor: root.primaryButtonColor
                                        hoverColor: root.primaryButtonHoverColor
                                        pressedColor: root.primaryButtonPressedColor
                                        onClicked: {
                                            if (root.r18Controller) root.r18Controller.importSourceUrl(directSourceUrl.text)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                radius: 9
                                color: root.itemFillColor
                                border.color: root.fieldStrokeColor

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 20
                                    anchors.rightMargin: 20
                                    anchors.topMargin: 12
                                    anchors.bottomMargin: 12
                                    spacing: 10

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            Layout.preferredWidth: 92
                                            text: "从目录选"
                                            color: root.textPrimaryColor
                                            font.pixelSize: 19
                                            elide: Text.ElideRight
                                        }
                                        TextField {
                                            id: officialCatalogUrl
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 38
                                            placeholderText: "可选: 源目录 index.json"
                                            placeholderTextColor: root.placeholderColor
                                            color: root.textPrimaryColor
                                            text: root.r18Controller ? root.r18Controller.officialSourceCatalogUrl : ""
                                            selectByMouse: true
                                            background: Rectangle {
                                                radius: 19
                                                color: root.fieldFillColor
                                                border.color: "transparent"
                                            }
                                        }
                                        GlassButton {
                                            Layout.preferredWidth: 82
                                            Layout.preferredHeight: 38
                                            text: "查看"
                                            textColor: root.textSecondaryColor
                                            textPixelSize: 13
                                            cornerRadius: 19
                                            normalColor: root.secondaryButtonColor
                                            hoverColor: root.secondaryButtonHoverColor
                                            pressedColor: root.secondaryButtonPressedColor
                                            onClicked: {
                                                if (root.r18Controller) root.r18Controller.refreshOfficialSources(officialCatalogUrl.text)
                                            }
                                        }
                                    }

                                    ListView {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        clip: true
                                        orientation: ListView.Horizontal
                                        spacing: 8
                                        model: root.r18Controller ? root.r18Controller.officialSourceModel : null
                                        delegate: GlassButton {
                                            width: Math.max(84, Math.min(160, titleMetrics.width + 34))
                                            height: 30
                                            text: model.title
                                            textColor: root.textSecondaryColor
                                            textPixelSize: 13
                                            cornerRadius: 10
                                            normalColor: root.secondaryButtonColor
                                            hoverColor: root.secondaryButtonHoverColor
                                            pressedColor: root.secondaryButtonPressedColor
                                            onClicked: if (root.r18Controller) root.r18Controller.importOfficialSource(index)

                                            TextMetrics {
                                                id: titleMetrics
                                                text: model.title
                                                font.pixelSize: 13
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 132
                                radius: 9
                                color: root.itemFillColor
                                border.color: root.fieldStrokeColor

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 20
                                    anchors.rightMargin: 20
                                    anchors.topMargin: 12
                                    anchors.bottomMargin: 12
                                    spacing: 8

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            Layout.preferredWidth: 92
                                            text: "本地导入"
                                            color: root.textPrimaryColor
                                            font.pixelSize: 19
                                        }
                                        TextField {
                                            id: sourcePackagePath
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 38
                                            placeholderText: "本地插件 zip/js 路径"
                                            placeholderTextColor: root.placeholderColor
                                            color: root.textPrimaryColor
                                            selectByMouse: true
                                            background: Rectangle {
                                                radius: 19
                                                color: root.fieldFillColor
                                                border.color: "transparent"
                                            }
                                        }
                                        GlassButton {
                                            Layout.preferredWidth: 76
                                            Layout.preferredHeight: 38
                                            text: "导入"
                                            textColor: root.primaryButtonTextColor
                                            cornerRadius: 19
                                            normalColor: root.primaryButtonColor
                                            hoverColor: root.primaryButtonHoverColor
                                            pressedColor: root.primaryButtonPressedColor
                                            onClicked: {
                                                if (root.r18Controller) root.r18Controller.importSourcePackage(sourcePackagePath.text, manifestText.text)
                                            }
                                        }
                                    }

                                    TextArea {
                                        id: manifestText
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        placeholderText: "可选 manifest.json"
                                        placeholderTextColor: root.placeholderColor
                                        color: root.textPrimaryColor
                                        wrapMode: TextEdit.Wrap
                                        selectByMouse: true
                                        background: Rectangle {
                                            radius: 10
                                            color: root.fieldFillColor
                                            border.color: "transparent"
                                        }
                                    }
                                }
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 0
                                model: root.r18Controller ? root.r18Controller.sourceModel : null
                                ScrollBar.vertical: GlassScrollBar {}
                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 56
                                    color: sourceRowMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                                    border.color: root.panelStrokeColor
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 20
                                        anchors.rightMargin: 14
                                        spacing: 12
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Text {
                                                Layout.fillWidth: true
                                                text: model.title
                                                color: root.textPrimaryColor
                                                font.pixelSize: 14
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: model.status
                                                color: root.textMutedColor
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }
                                        }
                                        Switch {
                                            checked: model.enabled
                                            onToggled: if (root.r18Controller) root.r18Controller.enableSource(model.sourceId, checked)
                                        }
                                    }
                                    MouseArea {
                                        id: sourceRowMouse
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
            height: 242
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
                height: 66
                color: root.panelFillColor
                border.color: root.fieldStrokeColor
                Column {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 3
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
