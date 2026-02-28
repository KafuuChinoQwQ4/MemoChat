import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "chat"

Rectangle {
    color: "#e9edf2"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ChatSideBar {
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            currentTab: controller.chatTab
            userIcon: controller.currentUserIcon
            hasPendingApply: controller.hasPendingApply
            onTabSelected: controller.switchChatTab(tab)
        }

        ChatLeftPanel {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
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

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#f5f7fa"

            StackLayout {
                anchors.fill: parent
                currentIndex: controller.chatTab

                ChatConversationPane {
                    peerName: controller.currentChatPeerName
                    hasCurrentChat: controller.hasCurrentChat
                    messageModel: controller.messageModel
                    onSendText: controller.sendTextMessage(text)
                    onSendImage: controller.sendImageMessage()
                    onSendFile: controller.sendFileMessage()
                    onOpenAttachment: controller.openExternalResource(url)
                }

                ChatContactPane {
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
