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
    property int currentDialogUid: 0
    property bool dialogsReady: false
    property bool contactsReady: false
    property bool groupsReady: false
    property var dialogModel
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
    property int sessionFilter: 0
    property real _sessionLastContentY: 0
    property bool _chatLoadBottomArmed: true
    property real _chatLoadThresholdPx: 48
    property real _chatLoadRearmOffsetPx: 96

    signal dialogUidSelected(int uid)
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
    signal dialogPinToggled(int uid)
    signal dialogMuteToggled(int uid)
    signal dialogMarkRead(int uid)
    signal dialogDraftCleared(int uid)

    function currentSessionListView() { return sessionPaneLoader.item ? sessionPaneLoader.item.sessionListView : null }
    function currentSessionModel() { return sessionFilter === 0 ? dialogModel : (sessionFilter === 1 ? chatModel : groupModel) }
    function ensureCurrentSessionSource() {
        if (currentTab !== 0) {
            return
        }
        if (sessionFilter === 1) {
            controller.ensureChatListInitialized()
        } else if (sessionFilter === 2) {
            controller.ensureGroupsInitialized()
        }
    }
    function activateSession(uidValue, indexValue) {
        const sessionView = currentSessionListView()
        if (sessionView) {
            sessionView.currentIndex = indexValue
        }
        if (sessionFilter === 2) {
            groupIndexSelected(indexValue)
        } else if (sessionFilter === 1) {
            chatIndexSelected(indexValue)
        } else {
            dialogUidSelected(uidValue)
        }
    }
    function syncCurrentSelection() {
        const modelRef = currentSessionModel()
        const sessionView = currentSessionListView()
        const modelCount = modelRef && modelRef.count !== undefined ? modelRef.count : 0
        if (!modelRef || modelCount <= 0) {
            if (sessionView) { sessionView.currentIndex = -1 }
            return
        }
        const targetUid = sessionFilter === 0 ? currentDialogUid : (sessionFilter === 1 ? (currentDialogUid > 0 ? currentDialogUid : 0) : (currentDialogUid < 0 ? currentDialogUid : 0))
        const targetIndex = targetUid === 0 ? -1 : (modelRef.indexOfUid ? modelRef.indexOfUid(targetUid) : -1)
        if (sessionView) { sessionView.currentIndex = targetIndex }
    }

    onCurrentTabChanged: {
        if (currentTab !== 0) {
            sessionFilter = 0
        } else {
            ensureCurrentSessionSource()
        }
    }
    onSessionFilterChanged: {
        const sessionView = currentSessionListView()
        _sessionLastContentY = sessionView ? sessionView.contentY : 0
        _chatLoadBottomArmed = true
        ensureCurrentSessionSource()
        syncCurrentSelection()
    }
    onCurrentDialogUidChanged: syncCurrentSelection()

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
                    blurRadius: 18
                    cornerRadius: 9
                    leftInset: 12
                    rightInset: 12
                    textPixelSize: 14
                    textHorizontalAlignment: TextInput.AlignLeft
                    placeholderText: "搜索用户ID（u#########）"
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

            Loader {
                anchors.fill: parent
                active: searchField.text.length > 0
                asynchronous: true
                sourceComponent: searchPaneComponent
            }

            Loader {
                id: sessionPaneLoader
                anchors.fill: parent
                active: searchField.text.length === 0 && currentTab === 0
                asynchronous: true
                sourceComponent: sessionPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: searchField.text.length === 0 && currentTab === 1
                asynchronous: true
                sourceComponent: contactPaneComponent
            }
        }
    }

    Component {
        id: searchPaneComponent
        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: searchPending ? "搜索中..." : "搜索用户ID"; color: "#415066"; Layout.fillWidth: true }
                    GlassButton {
                        text: "查找"; enabled: !searchPending; implicitWidth: 62; implicitHeight: 30; cornerRadius: 8
                        normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22); hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                        pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                        onClicked: root.searchRequested(searchField.text)
                    }
                }
                Label { Layout.fillWidth: true; text: root.searchStatusText; visible: text.length > 0; color: root.searchStatusError ? "#cc4a4a" : "#2a7f62"; wrapMode: Text.Wrap }
                ListView {
                    id: searchResultList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.searchModel
                    ScrollBar.vertical: GlassScrollBar { }
                    delegate: Rectangle {
                        width: ListView.view.width; height: 86; radius: 8
                        color: Qt.rgba(1, 1, 1, 0.30); border.color: Qt.rgba(1, 1, 1, 0.48)
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 10
                            Rectangle {
                                Layout.preferredWidth: 50; Layout.preferredHeight: 50; radius: 6; clip: true; color: Qt.rgba(0.74, 0.82, 0.92, 0.50)
                                Image {
                                    anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                                    property string fallbackSource: "qrc:/res/head_1.jpg"
                                    property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                    property bool loadFailed: false
                                    source: loadFailed ? fallbackSource : baseSource
                                    cache: true
                                    asynchronous: true
                                    opacity: (status === Image.Ready) ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                    onBaseSourceChanged: loadFailed = false
                                    onStatusChanged: if (status === Image.Error) { loadFailed = true }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                Label { text: name; font.bold: true; color: "#273449" }
                                Label { text: "ID: " + (userId && userId.length > 0 ? userId : uid); color: "#5f6f85" }
                                Label { text: desc; color: "#5f6f85"; elide: Text.ElideRight; Layout.fillWidth: true }
                            }
                            GlassButton {
                                text: "加好友"; implicitWidth: 70; implicitHeight: 30; cornerRadius: 8
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24); hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: addFriendDialog.openFor(uid, name, desc, icon)
                            }
                        }
                    }
                }
            }
            GlassSurface {
                anchors.centerIn: parent; width: 190; height: 64
                visible: !searchPending && searchResultList.count === 0
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10; blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20); strokeColor: Qt.rgba(1, 1, 1, 0.42)
                Label { anchors.centerIn: parent; text: "未找到匹配用户"; color: "#6a7b92"; font.pixelSize: 13 }
            }
        }
    }

    Component {
        id: sessionPaneComponent
        Item {
            property alias sessionListView: sessionList
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 76
                    color: Qt.rgba(1, 1, 1, 0.08); border.color: Qt.rgba(1, 1, 1, 0.22)
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 6
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            GlassButton { Layout.fillWidth: true; implicitHeight: 30; text: "全部"; cornerRadius: 8; normalColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20); hoverColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28); pressedColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16); onClicked: root.sessionFilter = 0 }
                            GlassButton { Layout.fillWidth: true; implicitHeight: 30; text: ""; cornerRadius: 8; normalColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20); hoverColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28); pressedColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16); onClicked: root.sessionFilter = 1; Image { anchors.centerIn: parent; width: 16; height: 16; source: "qrc:/icons/user.png"; fillMode: Image.PreserveAspectFit } }
                            GlassButton { Layout.fillWidth: true; implicitHeight: 30; text: ""; cornerRadius: 8; normalColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20); hoverColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28); pressedColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16); onClicked: root.sessionFilter = 2; Image { anchors.centerIn: parent; width: 16; height: 16; source: "qrc:/icons/team.png"; fillMode: Image.PreserveAspectFit } }
                            GlassButton { implicitWidth: 42; implicitHeight: 30; text: "+"; visible: root.sessionFilter === 2; cornerRadius: 8; normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24); hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34); pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42); disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16); onClicked: root.createGroupRequested() }
                        }
                        Label { Layout.fillWidth: true; text: root.groupStatusText; visible: root.sessionFilter === 2 && text.length > 0; color: root.groupStatusError ? "#cc4a4a" : "#2a7f62"; elide: Text.ElideRight }
                    }
                }

                ListView {
                    id: sessionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    interactive: false
                    visible: root.sessionFilter === 0 ? root.dialogsReady : (root.sessionFilter === 1 || root.groupsReady)
                    model: root.currentSessionModel()
                    ScrollBar.vertical: GlassScrollBar { }
                    cacheBuffer: 200
                    maximumFlickVelocity: 4000
                    WheelHandler {
                        target: null
                        onWheel: function(event) {
                            const maxY = Math.max(0, sessionList.contentHeight - sessionList.height)
                            let deltaY = event.pixelDelta.y !== 0 ? event.pixelDelta.y : ((event.angleDelta.y / 120) * 48)
                            if (deltaY === 0) { return }
                            const nextY = Math.max(0, Math.min(maxY, sessionList.contentY - deltaY))
                            if (Math.abs(nextY - sessionList.contentY) > 0.1) {
                                sessionList.contentY = nextY
                                event.accepted = true
                            }
                        }
                    }
                    onContentYChanged: {
                        const movingDown = contentY > root._sessionLastContentY + 0.5
                        const nearBottom = (contentY + height) >= (contentHeight - root._chatLoadThresholdPx)
                        root._sessionLastContentY = contentY
                        if ((contentY + height) < (contentHeight - root._chatLoadRearmOffsetPx)) { root._chatLoadBottomArmed = true }
                        if (root.sessionFilter !== 1 || !root.canLoadMoreChats || root.chatLoadingMore || contentHeight <= height + 1) { return }
                        if (movingDown && nearBottom && root._chatLoadBottomArmed) { root._chatLoadBottomArmed = false; root.requestChatLoadMore() }
                    }
                    onCountChanged: root.syncCurrentSelection()
                    footer: Item {
                        width: sessionList.width
                        height: (root.sessionFilter === 1 && (root.chatLoadingMore || root.canLoadMoreChats)) ? 40 : 0
                        Label { anchors.centerIn: parent; text: root.chatLoadingMore ? "加载中..." : (root.canLoadMoreChats ? "上滑加载更多" : ""); color: "#8e9aac"; font.pixelSize: 12 }
                    }
                    delegate: Rectangle {
                        width: ListView.view.width; height: 64
                        color: ListView.isCurrentItem ? Qt.rgba(0.77, 0.86, 0.97, 0.32) : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 8
                            Rectangle {
                                Layout.preferredWidth: 42; Layout.preferredHeight: 42; radius: 4; clip: true; color: Qt.rgba(0.73, 0.82, 0.92, 0.46)
                                Image {
                                    anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                                    property bool isGroupDialog: root.sessionFilter === 2 || (root.sessionFilter === 0 && uid < 0)
                                    property string fallbackSource: isGroupDialog ? "qrc:/res/chat_icon.png" : "qrc:/res/head_1.jpg"
                                    property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                    property bool loadFailed: false
                                    source: loadFailed ? fallbackSource : baseSource
                                    cache: true
                                    asynchronous: true
                                    opacity: (status === Image.Ready) ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                    onBaseSourceChanged: loadFailed = false
                                    onStatusChanged: if (status === Image.Error) { loadFailed = true }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 3
                                Label { text: name; font.bold: true; color: "#273449" }
                                Label { text: (draftText && draftText.length > 0) ? ("草稿: " + draftText) : ((lastMsg && lastMsg.length > 0) ? lastMsg : desc); color: (draftText && draftText.length > 0) ? "#c05f45" : "#647489"; elide: Text.ElideRight; Layout.fillWidth: true }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.RightButton) {
                                    root.activateSession(uid, index)
                                    sessionMenu.popup()
                                    return
                                }
                                root.activateSession(uid, index)
                            }
                        }
                        Menu {
                            id: sessionMenu
                            y: parent.height - 4
                            MenuItem { text: (pinnedRank && pinnedRank > 0) ? "取消置顶" : "置顶会话"; onTriggered: function() { root.dialogPinToggled(uid) } }
                            MenuItem { text: (muteState && muteState > 0) ? "取消静音" : "静音会话"; onTriggered: function() { root.dialogMuteToggled(uid) } }
                            MenuItem { text: "标记已读"; enabled: unreadCount && unreadCount > 0; onTriggered: function() { root.dialogMarkRead(uid) } }
                            MenuItem { text: "清空草稿"; enabled: draftText && draftText.length > 0; onTriggered: function() { root.dialogDraftCleared(uid) } }
                        }
                    }
                }
            }
            GlassSurface {
                anchors.centerIn: parent; width: 220; height: 68
                visible: !sessionList.visible || (sessionList.count === 0 && !root.chatLoadingMore)
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10; blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20); strokeColor: Qt.rgba(1, 1, 1, 0.42)
                Label {
                    anchors.centerIn: parent
                    text: !root.dialogsReady && root.sessionFilter === 0 ? "正在加载最近会话"
                          : (!root.groupsReady && root.sessionFilter === 2 ? "正在加载群组列表"
                                                                             : (root.sessionFilter === 2 ? "暂无群聊" : (root.sessionFilter === 1 ? "暂无私聊" : "暂无会话")))
                    color: "#6a7b92"; font.pixelSize: 13
                }
            }
        }
    }

    Component {
        id: contactPaneComponent
        ContactListPane {
            anchors.fill: parent
            backdrop: root.backdrop
            contactModel: root.contactModel
            hasPendingApply: root.hasPendingApply
            canLoadMore: root.canLoadMoreContacts
            loadingMore: root.contactLoadingMore
            onOpenApplyRequested: root.openApplyRequested()
            onContactIndexSelected: function(index) { root.contactIndexSelected(index) }
            onLoadMoreRequested: root.requestContactLoadMore()
            Component.onCompleted: controller.ensureContactsInitialized()
        }
    }

    AddFriendDialog {
        id: addFriendDialog
        anchors.centerIn: Overlay.overlay
        backdrop: root.backdrop !== null ? root.backdrop : root
        onSubmitted: function(uid, backName, tags) { root.addFriendRequested(uid, backName, tags) }
    }
}
