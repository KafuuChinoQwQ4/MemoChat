import QtQuick 2.15
import QtQuick.Controls 2.15
import "qrc:/qml/components"

Item {
    id: root

    property var pageModel: null
    property bool chromeVisible: true
    property string title: ""
    property int currentPageIndex: 1
    property int pageCount: 1
    property bool favorite: false
    property string gatewayBaseUrl: ""
    property color panelFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color panelStrokeColor: Qt.rgba(1, 1, 1, 0.46)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal backRequested()
    signal favoriteToggled(bool favorite)
    signal progressCommitted(int pageIndex)
    signal chromeVisibilityChangedByUser(bool visible)

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
            model: root.pageModel
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
                    onClicked: root.chromeVisibilityChangedByUser(!root.chromeVisible)
                }
            }

            onMovementEnded: {
                var idx = indexAt(width / 2, contentY + height / 2)
                if (idx >= 0) {
                    root.progressCommitted(idx + 1)
                }
            }
        }

        R18ReaderChrome {
            anchors.fill: parent
            chromeVisible: root.chromeVisible
            title: root.title
            currentPageIndex: root.currentPageIndex
            pageCount: root.pageCount
            favorite: root.favorite
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
            onBackRequested: root.backRequested()
            onPageMoved: function(page) {
                readerList.positionViewAtIndex(page - 1, ListView.Beginning)
                root.progressCommitted(page)
            }
            onFavoriteToggled: function(favorite) {
                root.favoriteToggled(favorite)
            }
        }
    }
}
