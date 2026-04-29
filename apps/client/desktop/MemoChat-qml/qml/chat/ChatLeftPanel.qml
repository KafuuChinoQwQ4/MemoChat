import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
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
    property var agentSessions: []
    property string agentCurrentSessionId: ""
    property string agentCurrentModel: ""
    property bool agentBusy: false
    property int momentsSelectedUid: 0
    property string momentsSelectedName: ""
    property int sessionFilter: 0
    property real _sessionLastContentY: 0
    property bool _chatLoadBottomArmed: true
    property real _chatLoadThresholdPx: 48
    property real _chatLoadRearmOffsetPx: 96
    property string pendingAgentDeleteSessionId: ""
    property string pendingAgentDeleteTitle: ""

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
    signal agentRefreshRequested()
    signal agentNewSessionRequested()
    signal agentSessionSelected(string sessionId)
    signal agentSessionDeleted(string sessionId)
    signal momentFriendSelected(int uid, string displayName)
    signal dialogPinToggled(int uid)
    signal dialogMuteToggled(int uid)
    signal dialogMarkRead(int uid)
    signal dialogDraftCleared(int uid)

    function currentSessionListView() { return sessionPaneLoader.item ? sessionPaneLoader.item.sessionListView : null }
    function currentSessionModel() { return sessionFilter === 0 ? dialogModel : (sessionFilter === 1 ? chatModel : groupModel) }
    function usesSearchHeader() {
        return currentTab === AppController.ChatTabPage || currentTab === AppController.ContactTabPage
    }
    function contextualTitle() {
        if (currentTab === AppController.MomentsTabPage) {
            return "朋友圈"
        }
        if (currentTab === AppController.AgentTabPage) {
            return "AI 助手"
        }
        return "更多"
    }
    function contextualSubtitle() {
        if (currentTab === AppController.MomentsTabPage) {
            return momentsSelectedUid > 0
                   ? ((momentsSelectedName.length > 0 ? momentsSelectedName : "好友") + " 的朋友圈")
                   : "按好友查看朋友圈动态。"
        }
        if (currentTab === AppController.AgentTabPage) {
            return agentCurrentModel.length > 0 ? agentCurrentModel : "选择会话后开始和 AI 对话。"
        }
        return "在这里管理账户信息和应用设置。"
    }
    function ensureCurrentSessionSource() {
        if (currentTab !== AppController.ChatTabPage) {
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
            if (sessionView) {
                sessionView.currentIndex = -1
            }
            return
        }
        const targetUid = sessionFilter === 0
                        ? currentDialogUid
                        : (sessionFilter === 1
                           ? (currentDialogUid > 0 ? currentDialogUid : 0)
                           : (currentDialogUid < 0 ? currentDialogUid : 0))
        const targetIndex = targetUid === 0 ? -1 : (modelRef.indexOfUid ? modelRef.indexOfUid(targetUid) : -1)
        if (sessionView) {
            sessionView.currentIndex = targetIndex
        }
    }

    onCurrentTabChanged: {
        if (currentTab !== AppController.ChatTabPage) {
            sessionFilter = 0
        } else {
            ensureCurrentSessionSource()
        }

        if (!usesSearchHeader() && searchField.text.length > 0) {
            searchField.text = ""
            searchCleared()
        }

        if (currentTab === AppController.AgentTabPage) {
            agentRefreshRequested()
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
            Layout.preferredHeight: root.usesSearchHeader() ? 60 : 84
            color: Qt.rgba(1, 1, 1, 0.10)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 6
                visible: root.usesSearchHeader()

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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4
                visible: !root.usesSearchHeader()

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: root.contextualTitle()
                        color: "#2a3649"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Rectangle {
                        Layout.preferredHeight: 22
                        Layout.preferredWidth: busyLabel.implicitWidth + 14
                        radius: 11
                        color: Qt.rgba(0.36, 0.62, 0.92, 0.16)
                        visible: root.currentTab === AppController.AgentTabPage && root.agentBusy

                        Label {
                            id: busyLabel
                            anchors.centerIn: parent
                            text: "生成中"
                            color: "#2d6fb4"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.contextualSubtitle()
                    color: "#6a7b92"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
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
                active: root.usesSearchHeader() && searchField.text.length > 0
                asynchronous: true
                sourceComponent: searchPaneComponent
            }

            Loader {
                id: sessionPaneLoader
                anchors.fill: parent
                active: root.usesSearchHeader() && searchField.text.length === 0 && currentTab === AppController.ChatTabPage
                asynchronous: true
                sourceComponent: sessionPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: root.usesSearchHeader() && searchField.text.length === 0 && currentTab === AppController.ContactTabPage
                asynchronous: true
                sourceComponent: contactPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === AppController.MomentsTabPage
                asynchronous: true
                sourceComponent: momentsPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === AppController.AgentTabPage
                asynchronous: true
                sourceComponent: agentPaneComponent
            }

            Loader {
                anchors.fill: parent
                active: currentTab === AppController.SettingsTabPage
                asynchronous: true
                sourceComponent: placeholderPaneComponent
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

                    Label {
                        text: searchPending ? "搜索中..." : "搜索用户ID"
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

                ListView {
                    id: searchResultList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                                    property string fallbackSource: "qrc:/res/head_1.jpg"
                                    property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                    property bool loadFailed: false
                                    source: loadFailed ? fallbackSource : baseSource
                                    cache: true
                                    asynchronous: true
                                    opacity: status === Image.Ready ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                    onBaseSourceChanged: loadFailed = false
                                    onStatusChanged: if (status === Image.Error) { loadFailed = true }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: name
                                    font.bold: true
                                    color: "#273449"
                                }

                                Label {
                                    text: "ID: " + (userId && userId.length > 0 ? userId : uid)
                                    color: "#5f6f85"
                                }

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
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 190
                height: 64
                visible: !searchPending && searchResultList.count === 0
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 16
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

    Component {
        id: sessionPaneComponent
        Item {
            property alias sessionListView: sessionList

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
                                text: "全部"
                                cornerRadius: 8
                                normalColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                hoverColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                pressedColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: root.sessionFilter = 0
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                implicitHeight: 30
                                text: ""
                                cornerRadius: 8
                                normalColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                hoverColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                pressedColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: root.sessionFilter = 1

                                Image {
                                    anchors.centerIn: parent
                                    width: 16
                                    height: 16
                                    source: "qrc:/icons/user.png"
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                implicitHeight: 30
                                text: ""
                                cornerRadius: 8
                                normalColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                hoverColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                pressedColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: root.sessionFilter = 2

                                Image {
                                    anchors.centerIn: parent
                                    width: 16
                                    height: 16
                                    source: "qrc:/icons/team.png"
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            GlassButton {
                                implicitWidth: 42
                                implicitHeight: 30
                                text: "+"
                                visible: root.sessionFilter === 2
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: root.createGroupRequested()
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.groupStatusText
                            visible: root.sessionFilter === 2 && text.length > 0
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
                            if (deltaY === 0) {
                                return
                            }
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
                        if ((contentY + height) < (contentHeight - root._chatLoadRearmOffsetPx)) {
                            root._chatLoadBottomArmed = true
                        }
                        if (root.sessionFilter !== 1 || !root.canLoadMoreChats || root.chatLoadingMore || contentHeight <= height + 1) {
                            return
                        }
                        if (movingDown && nearBottom && root._chatLoadBottomArmed) {
                            root._chatLoadBottomArmed = false
                            root.requestChatLoadMore()
                        }
                    }

                    onCountChanged: root.syncCurrentSelection()

                    footer: Item {
                        width: sessionList.width
                        height: (root.sessionFilter === 1 && (root.chatLoadingMore || root.canLoadMoreChats)) ? 40 : 0

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
                                    property bool isGroupDialog: root.sessionFilter === 2 || (root.sessionFilter === 0 && uid < 0)
                                    property string fallbackSource: isGroupDialog ? "qrc:/res/chat_icon.png" : "qrc:/res/head_1.jpg"
                                    property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                    property bool loadFailed: false
                                    source: loadFailed ? fallbackSource : baseSource
                                    cache: true
                                    asynchronous: true
                                    opacity: status === Image.Ready ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                    onBaseSourceChanged: loadFailed = false
                                    onStatusChanged: if (status === Image.Error) { loadFailed = true }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3

                                Label {
                                    text: name
                                    font.bold: true
                                    color: "#273449"
                                }

                                Label {
                                    text: (draftText && draftText.length > 0) ? ("草稿: " + draftText) : ((lastMsg && lastMsg.length > 0) ? lastMsg : desc)
                                    color: (draftText && draftText.length > 0) ? "#c05f45" : "#647489"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
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

                            MenuItem {
                                text: (pinnedRank && pinnedRank > 0) ? "取消置顶" : "置顶会话"
                                onTriggered: function() { root.dialogPinToggled(uid) }
                            }

                            MenuItem {
                                text: (muteState && muteState > 0) ? "取消静音" : "静音会话"
                                onTriggered: function() { root.dialogMuteToggled(uid) }
                            }

                            MenuItem {
                                text: "标记已读"
                                enabled: unreadCount && unreadCount > 0
                                onTriggered: function() { root.dialogMarkRead(uid) }
                            }

                            MenuItem {
                                text: "清空草稿"
                                enabled: draftText && draftText.length > 0
                                onTriggered: function() { root.dialogDraftCleared(uid) }
                            }
                        }
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 220
                height: 68
                visible: !sessionList.visible || (sessionList.count === 0 && !root.chatLoadingMore)
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                Label {
                    anchors.centerIn: parent
                    text: !root.dialogsReady && root.sessionFilter === 0 ? "正在加载最近会话"
                          : (!root.groupsReady && root.sessionFilter === 2 ? "正在加载群组列表"
                                                                             : (root.sessionFilter === 2 ? "暂无群聊" : (root.sessionFilter === 1 ? "暂无私聊" : "暂无会话")))
                    color: "#6a7b92"
                    font.pixelSize: 13
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

    Component {
        id: momentsPaneComponent
        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    radius: 10
                    property bool selected: root.momentsSelectedUid <= 0
                    color: selected ? Qt.rgba(0.54, 0.70, 0.93, 0.24)
                                    : (allMomentsArea.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.06))
                    border.color: selected ? Qt.rgba(0.54, 0.70, 0.93, 0.50)
                                           : Qt.rgba(1, 1, 1, 0.28)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 8
                            color: Qt.rgba(0.54, 0.70, 0.93, 0.24)

                            Image {
                                anchors.centerIn: parent
                                width: 20
                                height: 20
                                source: "qrc:/icons/moments.png"
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: "全部动态"
                                color: "#273449"
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "好友和自己的朋友圈"
                                color: "#6a7b92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }

                    MouseArea {
                        id: allMomentsArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.momentFriendSelected(0, "")
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: "好友"
                    color: "#687991"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: momentsFriendList
                        anchors.fill: parent
                        clip: true
                        spacing: 6
                        model: root.contactModel
                        ScrollBar.vertical: GlassScrollBar { }

                        delegate: Rectangle {
                            id: momentFriendDelegate
                            width: ListView.view.width
                            height: 58
                            radius: 10
                            property bool selected: uid === root.momentsSelectedUid
                            color: selected ? Qt.rgba(0.54, 0.70, 0.93, 0.22)
                                            : (friendArea.containsMouse ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.04))
                            border.color: selected ? Qt.rgba(0.54, 0.70, 0.93, 0.50)
                                                   : Qt.rgba(1, 1, 1, 0.22)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    radius: 8
                                    clip: true
                                    color: Qt.rgba(0.74, 0.83, 0.93, 0.50)

                                    Image {
                                        anchors.fill: parent
                                        fillMode: Image.PreserveAspectCrop
                                        property string fallbackSource: "qrc:/res/head_1.jpg"
                                        property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                        property bool loadFailed: false
                                        source: loadFailed ? fallbackSource : baseSource
                                        cache: true
                                        asynchronous: true
                                        opacity: status === Image.Ready ? 1.0 : 0.0
                                        Behavior on opacity { NumberAnimation { duration: 200 } }
                                        onBaseSourceChanged: loadFailed = false
                                        onStatusChanged: if (status === Image.Error) { loadFailed = true }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: (nick && nick.length > 0) ? nick : name
                                        color: "#273449"
                                        font.pixelSize: 13
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: "ID: " + (userId && userId.length > 0 ? userId : uid)
                                        color: "#6a7b92"
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            MouseArea {
                                id: friendArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    const displayName = (nick && nick.length > 0) ? nick : name
                                    root.momentFriendSelected(uid, displayName)
                                }
                            }
                        }
                    }

                    GlassSurface {
                        anchors.centerIn: parent
                        width: 180
                        height: 64
                        visible: momentsFriendList.count === 0
                        backdrop: root.backdrop !== null ? root.backdrop : root
                        cornerRadius: 10
                        blurRadius: 16
                        fillColor: Qt.rgba(1, 1, 1, 0.18)
                        strokeColor: Qt.rgba(1, 1, 1, 0.38)

                        Label {
                            anchors.centerIn: parent
                            text: "暂无好友"
                            color: "#6a7b92"
                            font.pixelSize: 13
                        }
                    }
                }
            }

            Component.onCompleted: controller.ensureContactsInitialized()
        }
    }

    Component {
        id: agentPaneComponent
        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "会话"
                        color: "#2a3649"
                        font.pixelSize: 15
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    GlassButton {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 30
                        text: "新建"
                        textPixelSize: 12
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
                        onClicked: root.agentNewSessionRequested()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.agentCurrentModel.length > 0 ? ("当前模型: " + root.agentCurrentModel) : "当前模型: 正在加载"
                    color: "#6a7b92"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                ListView {
                    id: agentSessionList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.agentSessions
                    spacing: 6
                    ScrollBar.vertical: GlassScrollBar { }

                    delegate: Rectangle {
                        width: ListView.view.width
                        implicitHeight: 62
                        radius: 10
                        color: {
                            if (modelData.session_id === root.agentCurrentSessionId) {
                                return Qt.rgba(0.54, 0.70, 0.93, 0.22)
                            }
                            return sessionHover.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.04)
                        }
                        border.color: modelData.session_id === root.agentCurrentSessionId
                                      ? Qt.rgba(0.54, 0.70, 0.93, 0.54)
                                      : Qt.rgba(1, 1, 1, 0.24)

                        MouseArea {
                            id: sessionHover
                            z: 0
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.agentSessionSelected(modelData.session_id)
                        }

                        RowLayout {
                            z: 1
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 10
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.title || "新会话"
                                    color: "#273449"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.model_name || root.agentCurrentModel
                                    color: "#6a7b92"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                radius: 12
                                color: sessionDeleteArea.pressed ? Qt.rgba(0.89, 0.27, 0.27, 0.30)
                                      : sessionDeleteArea.containsMouse ? Qt.rgba(0.89, 0.27, 0.27, 0.24)
                                                                        : Qt.rgba(0.89, 0.27, 0.27, 0.16)
                                visible: sessionHover.containsMouse || modelData.session_id === root.agentCurrentSessionId
                                opacity: root.agentBusy ? 0.45 : 1.0

                                Label {
                                    anchors.centerIn: parent
                                    text: "×"
                                    color: "#b84b4b"
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                MouseArea {
                                    id: sessionDeleteArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    enabled: !root.agentBusy
                                    cursorShape: Qt.PointingHandCursor
                                    ToolTip.visible: containsMouse
                                    ToolTip.delay: 120
                                    ToolTip.text: "删除会话"
                                    onClicked: function(mouse) {
                                        mouse.accepted = true
                                        root.pendingAgentDeleteSessionId = modelData.session_id || ""
                                        root.pendingAgentDeleteTitle = modelData.title || "新会话"
                                        if (root.pendingAgentDeleteSessionId.length > 0) {
                                            agentDeleteDialog.open()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 210
                height: 82
                visible: agentSessionList.count === 0
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.18)
                strokeColor: Qt.rgba(1, 1, 1, 0.38)

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4

                    Label {
                        text: "暂无会话"
                        color: "#2a3649"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        text: "点击右上角「新建」开始对话"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Popup {
                id: agentDeleteDialog
                modal: true
                focus: true
                width: Math.min(320, parent.width - 32)
                height: 184
                x: Math.round((parent.width - width) / 2)
                y: Math.round((parent.height - height) / 2)
                padding: 0
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                Overlay.modal: Rectangle {
                    color: Qt.rgba(20, 28, 40, 0.26)
                }

                background: Rectangle {
                    radius: 16
                    color: "transparent"
                }

                onClosed: {
                    root.pendingAgentDeleteSessionId = ""
                    root.pendingAgentDeleteTitle = ""
                }

                contentItem: GlassSurface {
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    cornerRadius: 16
                    blurRadius: 20
                    fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.88)
                    strokeColor: Qt.rgba(1, 1, 1, 0.58)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.30)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        Label {
                            Layout.fillWidth: true
                            text: "删除 AI 会话"
                            color: "#263448"
                            font.pixelSize: 16
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: "确认删除「" + root.pendingAgentDeleteTitle + "」？"
                            color: "#5f6f85"
                            font.pixelSize: 14
                            wrapMode: Text.Wrap
                            verticalAlignment: Text.AlignVCenter
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            GlassButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                text: "取消"
                                textPixelSize: 13
                                cornerRadius: 10
                                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                                hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.26)
                                pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.34)
                                onClicked: agentDeleteDialog.close()
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                text: "删除"
                                textPixelSize: 13
                                textColor: "#b83f4a"
                                cornerRadius: 10
                                normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.16)
                                hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.24)
                                pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.32)
                                onClicked: {
                                    const sessionId = root.pendingAgentDeleteSessionId
                                    if (sessionId.length > 0) {
                                        root.agentSessionDeleted(sessionId)
                                    }
                                    agentDeleteDialog.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: placeholderPaneComponent
        Item {
            GlassSurface {
                anchors.fill: parent
                anchors.margins: 12
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 12
                blurRadius: 18
                fillColor: Qt.rgba(1, 1, 1, 0.16)
                strokeColor: Qt.rgba(1, 1, 1, 0.38)

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6

                    Label {
                        text: "更多设置"
                        color: "#2a3649"
                        font.pixelSize: 15
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        text: "设置项已放在右侧主区，左栏保持简洁。"
                        color: "#6a7b92"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    AddFriendDialog {
        id: addFriendDialog
        anchors.centerIn: Overlay.overlay
        backdrop: root.backdrop !== null ? root.backdrop : root
        onSubmitted: function(uid, backName, tags) { root.addFriendRequested(uid, backName, tags) }
    }
}
