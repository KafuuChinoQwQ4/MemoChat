pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/qml/components"
import "qrc:/features/contact/view"

Rectangle {
    id: root
    color: "transparent"

    property var backdrop: null
    property var contactController: null
    property var momentsModel: null
    property var momentsController: null
    property int currentUserUid: 0
    property int selectedFriendUid: 0
    property string selectedFriendName: ""
    property bool showPublishPage: false
    property int pendingDeleteMomentId: 0

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 16
        blurRadius: 20
        fillColor: Qt.rgba(0.95, 0.95, 0.97, 0.90)
        strokeColor: Qt.rgba(0.8, 0.8, 0.85, 0.6)
    }

    StackLayout {
        id: pageStack
        anchors.fill: parent
        currentIndex: root.showPublishPage ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Header
                Item {
                    Layout.preferredHeight: 52
                    Layout.fillWidth: true

                    GlassSurface {
                        anchors.fill: parent
                        backdrop: root.backdrop
                        cornerRadius: 0
                        blurRadius: 12
                        fillColor: Qt.rgba(1, 1, 1, 0.80)
                        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16

                        Label {
                            Layout.fillWidth: true
                            text: root.selectedFriendUid > 0
                                  ? ((root.selectedFriendName.length > 0 ? root.selectedFriendName : "好友") + " 的朋友圈")
                                  : "朋友圈"
                            font.pixelSize: 17
                            font.weight: Font.Medium
                            color: "#1a1a1a"
                            elide: Text.ElideRight
                        }

                        ToolButton {
                            id: publishButton
                            implicitWidth: 38
                            implicitHeight: 38
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.delay: 120
                            ToolTip.text: "发布朋友圈"
                            background: Rectangle {
                                radius: 10
                                color: publishButton.down ? "#dbe8fb"
                                      : publishButton.hovered ? "#edf5ff" : "#ffffff"
                                border.width: 1
                                border.color: publishButton.hovered ? "#9fc4f4" : "#d5dfec"
                            }
                            contentItem: Item {
                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 6
                                    clip: true
                                    color: "transparent"
                                    anchors.centerIn: parent

                                    Image {
                                        anchors.fill: parent
                                        source: "qrc:/icons/add.png"
                                        fillMode: Image.PreserveAspectCrop
                                        smooth: true
                                    }
                                }
                            }

                            onClicked: {
                                root.showPublishPage = true
                            }
                        }
                    }
                }

                // Feed masonry
                Item {
                    id: feedViewport
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Flickable {
                        id: feedView
                        anchors.fill: parent
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        contentWidth: width
                        contentHeight: Math.max(height, masonryHeight)
                        interactive: contentHeight > height
                        ScrollBar.vertical: GlassScrollBar { }

                        property int sidePadding: 18
                        property int topPadding: 14
                        property int columnGap: 16
                        property int rowGap: 14
                        property int bottomPadding: 24
                        property real masonryHeight: height
                        readonly property int columnCount: {
                            const usableWidth = Math.max(0, width - sidePadding * 2)
                            if (usableWidth >= 1080) return 3
                            if (usableWidth >= 660) return 2
                            return 1
                        }
                        readonly property real cardWidth: Math.max(
                            260,
                            Math.floor((width - sidePadding * 2 - columnGap * (columnCount - 1)) / columnCount))

                        function relayoutMasonry() {
                            const columns = Math.max(1, columnCount)
                            const heights = []
                            for (let c = 0; c < columns; ++c) {
                                heights.push(topPadding)
                            }

                            for (let i = 0; i < masonryRepeater.count; ++i) {
                                const card = masonryRepeater.itemAt(i)
                                if (!card) continue

                                let targetColumn = 0
                                for (let c = 1; c < columns; ++c) {
                                    if (heights[c] < heights[targetColumn]) {
                                        targetColumn = c
                                    }
                                }

                                card.width = cardWidth
                                card.x = sidePadding + targetColumn * (cardWidth + columnGap)
                                card.y = heights[targetColumn]

                                const measuredHeight = Math.max(card.height || 0, card.implicitHeight || 0)
                                heights[targetColumn] += measuredHeight + rowGap
                            }

                            let maxHeight = 0
                            for (let c = 0; c < columns; ++c) {
                                maxHeight = Math.max(maxHeight, heights[c])
                            }
                            masonryHeight = Math.max(height, maxHeight > 0 ? maxHeight - rowGap + bottomPadding : height)
                            maybeLoadMore()
                        }

                        function maybeLoadMore() {
                            if (!root.momentsController || !root.momentsController.hasMore) return
                            if (root.momentsController.loading) return
                            if (contentY + height >= contentHeight - 96) {
                                root.momentsController.loadMore()
                            }
                        }

                        onWidthChanged: Qt.callLater(relayoutMasonry)
                        onColumnCountChanged: Qt.callLater(relayoutMasonry)
                        onCardWidthChanged: Qt.callLater(relayoutMasonry)
                        onHeightChanged: Qt.callLater(relayoutMasonry)
                        onContentYChanged: maybeLoadMore()
                        onContentHeightChanged: maybeLoadMore()

                        Item {
                            id: masonryContent
                            width: feedView.width
                            height: feedView.masonryHeight

                            Repeater {
                                id: masonryRepeater
                                model: root.momentsModel ? root.momentsModel : null

                                onItemAdded: function(index, item) {
                                    Qt.callLater(feedView.relayoutMasonry)
                                }
                                onItemRemoved: function(index, item) {
                                    Qt.callLater(feedView.relayoutMasonry)
                                }

                                delegate: Loader {
                                    id: cardLoader
                                    width: feedView.cardWidth
                                    height: item ? item.implicitHeight : 0
                                    asynchronous: true

                                    required property int index
                                    required property var momentId
                                    required property int uid
                                    required property string userId
                                    required property string userName
                                    required property string userNick
                                    required property string userIcon
                                    required property int visibility
                                    required property string location
                                    required property var createdAt
                                    required property int likeCount
                                    required property int commentCount
                                    required property bool hasLiked
                                    required property var items
                                    required property string textContent
                                    required property var likes
                                    required property var comments

                                    property var momentModel: ({
                                                                    "momentId": momentId,
                                                                    "uid": uid,
                                                                    "userId": userId,
                                                                    "userName": userName,
                                                                    "userNick": userNick,
                                                                    "userIcon": userIcon,
                                                                    "visibility": visibility,
                                                                    "location": location,
                                                                    "createdAt": createdAt,
                                                                    "likeCount": likeCount,
                                                                    "commentCount": commentCount,
                                                                    "hasLiked": hasLiked,
                                                                    "items": items || [],
                                                                    "textContent": textContent || "",
                                                                    "likes": likes || [],
                                                                    "comments": comments || []
                                                                })

                                    sourceComponent: Component {
                                        MomentsDelegate {
                                            width: cardLoader.width
                                            backdrop: root.backdrop
                                            controller: root.momentsController
                                            momentData: cardLoader.momentModel
                                            listTextContent: cardLoader.momentModel.textContent || ""
                                            canDelete: root.currentUserUid > 0
                                                       && cardLoader.momentModel.uid === root.currentUserUid
                                            onLikeClicked: {
                                                if (root.momentsController) {
                                                    root.momentsController.toggleLike(cardLoader.momentModel.momentId)
                                                }
                                            }
                                            onCommentClicked: {
                                                detailLoader.active = true
                                                if (detailLoader.item) {
                                                    detailLoader.item.openMoment(cardLoader.momentModel.momentId)
                                                }
                                            }
                                            onAvatarClicked: function(uid, name, icon, userId) {
                                                contactProfilePopup.openProfile(uid,
                                                                                name || "用户",
                                                                                icon || "qrc:/res/head_1.jpg",
                                                                                userId || "")
                                            }
                                            onDeleteClicked: {
                                                root.pendingDeleteMomentId = cardLoader.momentModel.momentId
                                                deleteConfirmDialog.open()
                                            }
                                        }
                                    }

                                    onLoaded: Qt.callLater(feedView.relayoutMasonry)
                                    onHeightChanged: Qt.callLater(feedView.relayoutMasonry)

                                    Connections {
                                        target: cardLoader.item
                                        function onImplicitHeightChanged() {
                                            Qt.callLater(feedView.relayoutMasonry)
                                        }
                                        function onHeightChanged() {
                                            Qt.callLater(feedView.relayoutMasonry)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        visible: masonryRepeater.count === 0 && !(root.momentsController && root.momentsController.loading)
                        text: root.selectedFriendUid > 0 ? "暂无该好友朋友圈" : "暂无朋友圈内容"
                        font.pixelSize: 14
                        color: "#999999"
                    }

                    // Loading indicator
                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 20
                        visible: root.momentsController ? root.momentsController.loading : false
                        running: visible
                        width: 28
                        height: 28
                    }
                }
            }
        }

        MomentsPublishPage {
            backdrop: root.backdrop
            controller: root.momentsController
            onBackRequested: {
                root.showPublishPage = false
            }
            onPublishRequested: {
                root.showPublishPage = false
                resetForm()
            }
        }
    }

    // Detail popup
    Loader {
        id: detailLoader
        active: false
        sourceComponent: Component {
            MomentsDetailPopup {
                anchors.centerIn: Overlay.overlay
                backdrop: root.backdrop
                controller: root.momentsController
                currentUserUid: root.currentUserUid
                onAvatarProfileRequested: function(uid, name, icon, userId) {
                    contactProfilePopup.openProfile(uid, name, icon, userId)
                }
                onClosed: detailLoader.active = false
            }
        }
    }

    ContactProfilePopup {
        id: contactProfilePopup
        contactController: root.contactController
    }

    Dialog {
        id: deleteConfirmDialog
        modal: true
        focus: true
        anchors.centerIn: parent
        width: Math.min(320, root.width - 48)
        title: "删除朋友圈"
        standardButtons: Dialog.Cancel | Dialog.Ok

        onAccepted: {
            if (root.momentsController && root.pendingDeleteMomentId > 0) {
                root.momentsController.deleteMoment(root.pendingDeleteMomentId)
            }
            root.pendingDeleteMomentId = 0
        }
        onRejected: root.pendingDeleteMomentId = 0

        contentItem: Label {
            text: "确认删除这条朋友圈？"
            font.pixelSize: 14
            color: "#2a2f39"
            wrapMode: Text.Wrap
        }
    }
}
