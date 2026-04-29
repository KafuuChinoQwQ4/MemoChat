import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var backdrop: null
    property var appController: null
    property var momentsModel: null
    property var momentsController: null
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

                // Feed list
                ListView {
                    id: feedView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 12
                    clip: true
                    cacheBuffer: 200

                    model: root.momentsModel ? root.momentsModel : null

                    delegate: Loader {
                        width: feedView.width
                        asynchronous: true
                        sourceComponent: Component {
                            MomentsDelegate {
                                width: feedView.width
                                backdrop: root.backdrop
                                momentData: model
                                listTextContent: model.textContent || ""
                                canDelete: root.appController
                                           && root.appController.currentUserUid > 0
                                           && model.uid === root.appController.currentUserUid
                                onLikeClicked: {
                                    if (root.momentsController) {
                                        root.momentsController.toggleLike(model.momentId)
                                    }
                                }
                                onCommentClicked: {
                                    detailLoader.active = true
                                    if (detailLoader.item) {
                                        detailLoader.item.openMoment(model.momentId)
                                    }
                                }
                                onAvatarClicked: {
                                    contactProfilePopup.openProfile(model.uid,
                                                                    model.userNick || model.userName || "用户",
                                                                    model.userIcon || "qrc:/res/head_1.jpg",
                                                                    model.userId || "")
                                }
                                onDeleteClicked: {
                                    root.pendingDeleteMomentId = model.momentId
                                    deleteConfirmDialog.open()
                                }
                            }
                        }
                    }

                    // Pull-to-refresh / Load more
                    onAtYEndChanged: {
                        if (atYEnd && root.momentsController && root.momentsController.hasMore) {
                            root.momentsController.loadMore()
                        }
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        visible: feedView.count === 0 && !(root.momentsController && root.momentsController.loading)
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
                currentUserUid: root.appController ? root.appController.currentUserUid : 0
                onAvatarProfileRequested: function(uid, name, icon, userId) {
                    contactProfilePopup.openProfile(uid, name, icon, userId)
                }
                onClosed: detailLoader.active = false
            }
        }
    }

    ContactProfilePopup {
        id: contactProfilePopup
        appController: root.appController
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
