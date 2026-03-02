import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "contact"

Rectangle {
    id: root
    color: "transparent"
    width: 250
    property Item backdrop: null

    property int currentTab: 0
    property var chatModel
    property var groupModel
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
    property string groupStatusText: ""
    property bool groupStatusError: false
    property bool groupMode: false

    signal chatIndexSelected(int index)
    signal groupIndexSelected(int index)
    signal requestChatLoadMore()
    signal searchRequested(string uidText)
    signal searchCleared()
    signal addFriendRequested(int uid, string bakName, var tags)
    signal openApplyRequested()
    signal contactIndexSelected(int index)
    signal requestContactLoadMore()
    signal createGroupRequested()
    signal refreshGroupRequested()

    onCurrentTabChanged: {
        if (currentTab !== 0) {
            groupMode = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Qt.rgba(1, 1, 1, 0.10)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 6

                GlassTextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    blurRadius: 30
                    cornerRadius: 9
                    leftInset: 12
                    rightInset: 12
                    textPixelSize: 14
                    textHorizontalAlignment: TextInput.AlignLeft
                    placeholderText: "搜索 UID"
                    onTextChanged: root.searchCleared()
                    onAccepted: root.searchRequested(searchField.text)
                }

                GlassButton {
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 34
                    text: "查找"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.55, 0.70, 0.92, 0.24)
                    hoverColor: Qt.rgba(0.55, 0.70, 0.92, 0.34)
                    pressedColor: Qt.rgba(0.55, 0.70, 0.92, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.56, 0.62, 0.18)
                    onClicked: root.searchRequested(searchField.text)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(1, 1, 1, 0.08)
            border.color: Qt.rgba(1, 1, 1, 0.28)

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length > 0
                    color: "transparent"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label {
                                text: searchPending ? "搜索中..." : "搜索用户"
                                color: "#415066"
                                Layout.fillWidth: true
                            }
                            GlassButton {
                                text: "查找"
                                enabled: !searchPending
                                implicitWidth: 62
                                implicitHeight: 30
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
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

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ListView {
                                id: searchResultList
                                anchors.fill: parent
                                clip: true
                                model: root.searchModel
                                ScrollBar.vertical: GlassScrollBar { }

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 86
                                    radius: 8
                                    color: Qt.rgba(1, 1, 1, 0.30)
                                    border.color: Qt.rgba(1, 1, 1, 0.48)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Rectangle {
                                            Layout.preferredWidth: 50
                                            Layout.preferredHeight: 50
                                            radius: 6
                                            clip: true
                                            color: Qt.rgba(0.74, 0.82, 0.92, 0.50)
                                            Image {
                                                anchors.fill: parent
                                                fillMode: Image.PreserveAspectCrop
                                                source: icon
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Label { text: name; font.bold: true; color: "#273449" }
                                            Label { text: "UID: " + uid; color: "#5f6f85" }
                                            Label {
                                                text: desc
                                                color: "#5f6f85"
                                                elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }
                                        }

                                        GlassButton {
                                            text: "加好友"
                                            implicitWidth: 70
                                            implicitHeight: 30
                                            cornerRadius: 8
                                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                            onClicked: addFriendDialog.openFor(uid, name, desc, icon)
                                        }
                                    }
                                }
                            }

                            GlassSurface {
                                anchors.centerIn: parent
                                width: 190
                                height: 64
                                visible: searchField.text.length > 0
                                         && !searchPending
                                         && searchResultList.count === 0
                                backdrop: root.backdrop !== null ? root.backdrop : root
                                cornerRadius: 10
                                blurRadius: 28
                                fillColor: Qt.rgba(1, 1, 1, 0.20)
                                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                                Label {
                                    anchors.centerIn: parent
                                    text: "未找到匹配用户"
                                    color: "#6a7b92"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 76
                            color: Qt.rgba(1, 1, 1, 0.08)
                            border.color: Qt.rgba(1, 1, 1, 0.22)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    GlassButton {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        text: "会话"
                                        cornerRadius: 8
                                        normalColor: root.groupMode ? Qt.rgba(0.48, 0.56, 0.66, 0.20) : Qt.rgba(0.36, 0.62, 0.92, 0.28)
                                        hoverColor: root.groupMode ? Qt.rgba(0.48, 0.56, 0.66, 0.28) : Qt.rgba(0.36, 0.62, 0.92, 0.38)
                                        pressedColor: root.groupMode ? Qt.rgba(0.48, 0.56, 0.66, 0.34) : Qt.rgba(0.36, 0.62, 0.92, 0.45)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.groupMode = false
                                    }

                                    GlassButton {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        text: "群聊"
                                        cornerRadius: 8
                                        normalColor: root.groupMode ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                        hoverColor: root.groupMode ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                        pressedColor: root.groupMode ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.groupMode = true
                                    }

                                    GlassButton {
                                        implicitWidth: 42
                                        implicitHeight: 30
                                        text: "+"
                                        visible: root.groupMode
                                        cornerRadius: 8
                                        normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                                        hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                                        pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.createGroupRequested()
                                    }

                                    GlassButton {
                                        implicitWidth: 42
                                        implicitHeight: 30
                                        text: "刷"
                                        visible: root.groupMode
                                        cornerRadius: 8
                                        normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.24)
                                        hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.34)
                                        pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.42)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.refreshGroupRequested()
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.groupStatusText
                                    visible: root.groupMode && text.length > 0
                                    color: root.groupStatusError ? "#cc4a4a" : "#2a7f62"
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ListView {
                            id: sessionList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.groupMode ? root.groupModel : root.chatModel
                            ScrollBar.vertical: GlassScrollBar { }
                            onContentYChanged: {
                                if (root.groupMode) {
                                    return
                                }
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
                                if (count <= 0 || currentIndex >= 0) {
                                    return
                                }
                                currentIndex = 0
                                if (root.groupMode) {
                                    root.groupIndexSelected(0)
                                } else {
                                    root.chatIndexSelected(0)
                                }
                            }

                            footer: Item {
                                width: sessionList.width
                                height: (!root.groupMode && (root.chatLoadingMore || root.canLoadMoreChats)) ? 40 : 0

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
                                color: ListView.isCurrentItem ? Qt.rgba(0.77, 0.86, 0.97, 0.32) : "transparent"

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
                                        color: Qt.rgba(0.73, 0.82, 0.92, 0.46)
                                        Image {
                                            anchors.fill: parent
                                            fillMode: Image.PreserveAspectCrop
                                            source: icon
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Label { text: name; font.bold: true; color: "#273449" }
                                        Label {
                                            text: (lastMsg && lastMsg.length > 0) ? lastMsg : desc
                                            color: "#647489"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        sessionList.currentIndex = index
                                        if (root.groupMode) {
                                            root.groupIndexSelected(index)
                                        } else {
                                            root.chatIndexSelected(index)
                                        }
                                    }
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    height: 1
                                    color: Qt.rgba(1, 1, 1, 0.28)
                                    visible: index < (sessionList.count - 1)
                                }
                            }
                        }
                    }

                    GlassSurface {
                        anchors.centerIn: parent
                        width: 180
                        height: 64
                        visible: sessionList.count === 0 && !root.chatLoadingMore
                        backdrop: root.backdrop !== null ? root.backdrop : root
                        cornerRadius: 10
                        blurRadius: 28
                        fillColor: Qt.rgba(1, 1, 1, 0.20)
                        strokeColor: Qt.rgba(1, 1, 1, 0.42)

                        Label {
                            anchors.centerIn: parent
                            text: root.groupMode ? "暂无群聊" : "暂无聊天记录"
                            color: "#6a7b92"
                            font.pixelSize: 13
                        }
                    }
                }

                ContactListPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 1
                    backdrop: root.backdrop
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
        backdrop: root.backdrop !== null ? root.backdrop : root
        onSubmitted: root.addFriendRequested(uid, backName, tags)
    }
}
