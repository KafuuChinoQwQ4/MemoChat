import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "contact"

Rectangle {
    id: root
    color: "#f7f8fa"
    width: 250

    property int currentTab: 0
    property var chatModel
    property var contactModel
    property var searchModel
    property bool searchPending: false
    property string searchStatusText: ""
    property bool searchStatusError: false
    property bool canLoadMoreChats: false
    property bool chatLoadingMore: false
    property bool hasPendingApply: false
    property bool canLoadMoreContacts: false
    property bool contactLoadingMore: false

    signal chatIndexSelected(int index)
    signal requestChatLoadMore()
    signal searchRequested(string uidText)
    signal searchCleared()
    signal addFriendRequested(int uid, string bakName, var tags)
    signal openApplyRequested()
    signal contactIndexSelected(int index)
    signal requestContactLoadMore()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#f7f8fa"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 6

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    placeholderText: "搜索 UID"
                    onTextChanged: {
                        root.searchCleared()
                    }
                    onAccepted: {
                        root.searchRequested(searchField.text)
                    }
                }

                Button {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    icon.source: "qrc:/res/search.png"
                    onClicked: root.searchRequested(searchField.text)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length > 0
                    color: "#ffffff"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label {
                                text: searchPending ? "搜索中..." : "搜索用户"
                                color: "#4e5969"
                                Layout.fillWidth: true
                            }
                            Button {
                                text: "查找"
                                enabled: !searchPending
                                onClicked: root.searchRequested(searchField.text)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.searchStatusText
                            visible: text.length > 0
                            color: root.searchStatusError ? "#cc4a4a" : "#2a7f62"
                            wrapMode: Text.Wrap
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.searchModel
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 86
                                radius: 8
                                color: "#f5f7fa"
                                border.color: "#dce3ed"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Rectangle {
                                        Layout.preferredWidth: 50
                                        Layout.preferredHeight: 50
                                        radius: 6
                                        clip: true
                                        color: "#dfe4eb"
                                        Image {
                                            anchors.fill: parent
                                            fillMode: Image.PreserveAspectCrop
                                            source: icon
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Label { text: name; font.bold: true; color: "#2a3240" }
                                        Label { text: "UID: " + uid; color: "#6f7d91" }
                                        Label {
                                            text: desc
                                            color: "#6f7d91"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Button {
                                        text: "加好友"
                                        onClicked: addFriendDialog.openFor(uid, name, desc, icon)
                                    }
                                }
                            }
                        }
                    }
                }

                ListView {
                    id: chatList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 0
                    clip: true
                    model: root.chatModel
                    onContentYChanged: {
                        if (!root.canLoadMoreChats || root.chatLoadingMore) {
                            return
                        }
                        if (contentHeight <= height + 1) {
                            return
                        }
                        if (contentY + height >= contentHeight - 48) {
                            root.requestChatLoadMore()
                        }
                    }
                    onCountChanged: {
                        if (count > 0 && currentIndex < 0) {
                            currentIndex = 0
                            root.chatIndexSelected(0)
                        }
                    }

                    footer: Item {
                        width: chatList.width
                        height: (root.chatLoadingMore || root.canLoadMoreChats) ? 40 : 0

                        Label {
                            anchors.centerIn: parent
                            text: root.chatLoadingMore ? "加载中..." : (root.canLoadMoreChats ? "上滑加载更多" : "")
                            color: "#8e9aac"
                            font.pixelSize: 12
                        }
                    }

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 64
                        color: ListView.isCurrentItem ? "#e9eef7" : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 42
                                radius: 4
                                clip: true
                                color: "#dde3ec"
                                Image {
                                    anchors.fill: parent
                                    fillMode: Image.PreserveAspectCrop
                                    source: icon
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Label { text: name; font.bold: true; color: "#2a3240" }
                                Label {
                                    text: (lastMsg && lastMsg.length > 0) ? lastMsg : desc
                                    color: "#7a8698"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                chatList.currentIndex = index
                                root.chatIndexSelected(index)
                            }
                        }
                    }
                }

                ContactListPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 1
                    contactModel: root.contactModel
                    hasPendingApply: root.hasPendingApply
                    canLoadMore: root.canLoadMoreContacts
                    loadingMore: root.contactLoadingMore
                    onOpenApplyRequested: root.openApplyRequested()
                    onContactIndexSelected: root.contactIndexSelected(index)
                    onLoadMoreRequested: root.requestContactLoadMore()
                }
            }
        }
    }

    AddFriendDialog {
        id: addFriendDialog
        anchors.centerIn: Overlay.overlay
        onSubmitted: root.addFriendRequested(uid, backName, tags)
    }
}
