import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "components"
import "chat"

Rectangle {
    id: root
    color: "transparent"
    property real revealProgress: 0.0

    function stageValue(start, span) {
        return Math.max(0, Math.min(1, (revealProgress - start) / span))
    }

    Component.onCompleted: revealProgress = 1.0

    Behavior on revealProgress {
        NumberAnimation {
            duration: 640
            easing.type: Easing.OutCubic
        }
    }

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        GlassSurface {
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 14
            blurRadius: 30
            fillColor: Qt.rgba(0.20, 0.28, 0.38, 0.34)
            strokeColor: Qt.rgba(1, 1, 1, 0.36)
            glowTopColor: Qt.rgba(1, 1, 1, 0.16)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.02)
            opacity: root.stageValue(0.02, 0.20)
            y: (1.0 - opacity) * 20

            ChatSideBar {
                anchors.fill: parent
                currentTab: controller.chatTab
                userIcon: controller.currentUserIcon
                hasPendingApply: controller.hasPendingApply
                onTabSelected: controller.switchChatTab(tab)
            }
        }

        GlassSurface {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 14
            blurRadius: 30
            fillColor: Qt.rgba(1, 1, 1, 0.16)
            strokeColor: Qt.rgba(1, 1, 1, 0.48)
            glowTopColor: Qt.rgba(1, 1, 1, 0.24)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.11, 0.20)
            y: (1.0 - opacity) * 18

            ChatLeftPanel {
                anchors.fill: parent
                backdrop: backdropLayer
                currentTab: controller.chatTab
                chatModel: controller.chatListModel
                contactModel: controller.contactListModel
                searchModel: controller.searchResultModel
                searchPending: controller.searchPending
                searchStatusText: controller.searchStatusText
                searchStatusError: controller.searchStatusError
                canLoadMoreChats: controller.canLoadMoreChats
                chatLoadingMore: controller.chatLoadingMore
                hasPendingApply: controller.hasPendingApply
                canLoadMoreContacts: controller.canLoadMoreContacts
                contactLoadingMore: controller.contactLoadingMore
                onChatIndexSelected: controller.selectChatIndex(index)
                onRequestChatLoadMore: controller.loadMoreChats()
                onContactIndexSelected: controller.selectContactIndex(index)
                onOpenApplyRequested: controller.showApplyRequests()
                onRequestContactLoadMore: controller.loadMoreContacts()
                onSearchRequested: controller.searchUser(uidText)
                onSearchCleared: controller.clearSearchState()
                onAddFriendRequested: controller.requestAddFriend(uid, bakName, tags)
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: backdropLayer
            cornerRadius: 16
            blurRadius: 34
            fillColor: Qt.rgba(1, 1, 1, 0.14)
            strokeColor: Qt.rgba(1, 1, 1, 0.54)
            glowTopColor: Qt.rgba(1, 1, 1, 0.25)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
            opacity: root.stageValue(0.20, 0.24)
            y: (1.0 - opacity) * 24

            StackLayout {
                anchors.fill: parent
                anchors.margins: 8
                currentIndex: controller.chatTab

                ChatConversationPane {
                    backdrop: backdropLayer
                    peerName: controller.currentChatPeerName
                    hasCurrentChat: controller.hasCurrentChat
                    messageModel: controller.messageModel
                    onSendText: controller.sendTextMessage(text)
                    onSendImage: controller.sendImageMessage()
                    onSendFile: controller.sendFileMessage()
                    onOpenAttachment: controller.openExternalResource(url)
                }

                ChatContactPane {
                    backdrop: backdropLayer
                    paneIndex: controller.contactPane
                    contactName: controller.currentContactName
                    contactNick: controller.currentContactNick
                    contactIcon: controller.currentContactIcon
                    contactBack: controller.currentContactBack
                    contactSex: controller.currentContactSex
                    hasCurrentContact: controller.hasCurrentContact
                    applyModel: controller.applyRequestModel
                    authStatusText: controller.authStatusText
                    authStatusError: controller.authStatusError
                    onApproveFriendRequested: controller.approveFriend(uid, backName, tags)
                    onAuthStatusCleared: controller.clearAuthStatus()
                    onMessageChatRequested: controller.jumpChatWithCurrentContact()
                    onVoiceChatRequested: controller.startVoiceChat()
                    onVideoChatRequested: controller.startVideoChat()
                }

                ChatSettingsPane {
                    backdrop: backdropLayer
                    userIcon: controller.currentUserIcon
                    userNick: controller.currentUserNick
                    userName: controller.currentUserName
                    userDesc: controller.currentUserDesc
                    statusText: controller.settingsStatusText
                    statusError: controller.settingsStatusError
                    onChooseAvatarRequested: controller.chooseAvatar()
                    onSaveProfileRequested: controller.saveProfile(nick, desc)
                    onStatusCleared: controller.clearSettingsStatus()
                }
            }
        }
    }

}
