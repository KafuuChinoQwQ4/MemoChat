import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components"
import "../call"
import "group"

Item {
    id: root

    property Item backdrop: null
    property bool groupCreationDialogActivated: false
    property var friendModel: null
    property var callSession: null
    property var appController: null

    signal createGroupSubmitted(string name, var memberUserIds)
    signal acceptRequested()
    signal rejectRequested()
    signal endRequested()
    signal muteToggled()
    signal cameraToggled()

    function openGroupCreationDialog() {
        if (createGroupDialogLoader.item) {
            createGroupDialogLoader.item.open()
        }
    }

    function openProfile(uid, name, icon, extra) {
        contactProfilePopup.openProfile(uid, name, icon, extra || "")
    }

    Loader {
        id: createGroupDialogLoader
        active: root.groupCreationDialogActivated
        asynchronous: true
        sourceComponent: Component {
            CreateGroupDialog {
                anchors.centerIn: Overlay.overlay
                backdrop: root.backdrop
                friendModel: root.friendModel
                onSubmitted: function(name, memberUserIds) {
                    root.createGroupSubmitted(name, memberUserIds)
                }
            }
        }
        onLoaded: root.openGroupCreationDialog()
    }

    CallOverlay {
        anchors.fill: parent
        sessionModel: root.callSession
        onAcceptRequested: root.acceptRequested()
        onRejectRequested: root.rejectRequested()
        onEndRequested: root.endRequested()
        onMuteToggled: root.muteToggled()
        onCameraToggled: root.cameraToggled()
    }

    ContactProfilePopup {
        id: contactProfilePopup
        appController: root.appController
    }
}
