import QtQuick 2.15
import QtQuick.Controls 2.15
import "qrc:/qml/components"
import "qrc:/features/call/view"
import "qrc:/features/contact/view"
import "qrc:/features/group/view"

Item {
    id: root

    property Item backdrop: null
    property bool groupCreationDialogActivated: false
    property var friendModel: null
    property var callSession: null
    property var livekitBridge: null
    property var contactController: null

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
        livekitBridge: root.livekitBridge
        onAcceptRequested: root.acceptRequested()
        onRejectRequested: root.rejectRequested()
        onEndRequested: root.endRequested()
        onMuteToggled: root.muteToggled()
        onCameraToggled: root.cameraToggled()
    }

    ContactProfilePopup {
        id: contactProfilePopup
        contactController: root.contactController
    }
}
