import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var backdrop: null
    property var momentsModel: null
    property var momentsController: null
    property bool showPublishPage: false

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
                            text: "朋友圈"
                            font.pixelSize: 17
                            font.weight: Font.Medium
                            color: "#1a1a1a"
                        }

                        Item { Layout.fillWidth: true }

                        ToolButton {
                            id: publishButton
                            icon.source: "qrc:/icons/add.png"
                            icon.width: 22
                            icon.height: 22
                            icon.color: "#2a7ae2"
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.delay: 120
                            ToolTip.text: "发布朋友圈"

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
                                    Qt.openUrlExternally("memochat://user/" + model.uid)
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
                        visible: feedView.count === 0
                        text: "暂无朋友圈内容"
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
                onClosed: detailLoader.active = false
            }
        }
    }
}
