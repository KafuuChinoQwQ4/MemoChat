import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "../components"

RowLayout {
    id: root
    spacing: 10
    visible: root.flipAngle >= 90
    enabled: visible
    layer.enabled: visible && root.flipInProgress

    property Item backdrop: null
    property real flipAngle: 0
    property bool flipInProgress: false
    property bool glassEffectsEnabled: false
    property bool r18PaneWarm: false
    property int r18ViewMode: 0
    property var r18NavigationItems: []
    property var r18Controller: null
    readonly property bool r18Ready: r18PaneLoader.status === Loader.Ready

    signal viewModeChangedByUser(int mode)
    signal returnToChatRequested()
    signal pendingFlipReady()

    transform: Rotation {
        origin.x: root.width / 2
        origin.y: root.height / 2
        axis.x: 0
        axis.y: 1
        axis.z: 0
        angle: root.flipAngle - 180
    }

    GlassSurface {
        Layout.preferredWidth: 72
        Layout.fillHeight: true
        backdrop: root.backdrop
        cornerRadius: 14
        blurEnabled: root.glassEffectsEnabled
        blurRadius: 18
        fillColor: Qt.rgba(1, 1, 1, 0.18)
        strokeColor: Qt.rgba(1, 1, 1, 0.56)
        glowTopColor: Qt.rgba(1, 1, 1, 0.28)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.02)

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            cursorShape: Qt.SizeAllCursor
            onPressed: function(mouse) {
                mouse.accepted = false
                if (Window.window) {
                    Window.window.startSystemMove()
                }
            }
        }

        ColumnLayout {
            z: 1
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 14

            Repeater {
                model: root.r18NavigationItems
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    radius: 13
                    color: root.r18ViewMode === modelData.mode ? Qt.rgba(0.56, 0.70, 0.86, 0.26)
                                                               : navMouse.containsMouse ? Qt.rgba(0.56, 0.70, 0.86, 0.16)
                                                                                        : "transparent"
                    border.color: root.r18ViewMode === modelData.mode ? Qt.rgba(1, 1, 1, 0.54) : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 27
                        height: 27
                        source: modelData.icon
                        fillMode: Image.PreserveAspectFit
                        opacity: root.r18ViewMode === modelData.mode ? 0.98 : 0.62
                        mipmap: true
                    }

                    MouseArea {
                        id: navMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.viewModeChangedByUser(modelData.mode)
                            if (r18PaneLoader.item) {
                                r18PaneLoader.item.viewMode = modelData.mode
                            }
                        }
                    }

                    ToolTip.visible: navMouse.containsMouse
                    ToolTip.delay: 350
                    ToolTip.text: modelData.label
                }
            }

            Item { Layout.fillHeight: true }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                property bool hovering: chatReturnMouse.containsMouse
                property bool pressed: chatReturnMouse.pressed
                scale: pressed ? 0.96 : (hovering ? 1.02 : 1.0)

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 13
                    color: chatReturnMouse.pressed ? Qt.rgba(0.56, 0.70, 0.86, 0.22)
                                                   : chatReturnMouse.containsMouse ? Qt.rgba(0.56, 0.70, 0.86, 0.16)
                                                                                   : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                Image {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    source: "qrc:/icons/r18.png"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.72
                    mipmap: true
                }

                MouseArea {
                    id: chatReturnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.returnToChatRequested()
                }

                ToolTip.visible: chatReturnMouse.containsMouse
                ToolTip.delay: 120
                ToolTip.text: "返回聊天"
            }
        }
    }

    GlassSurface {
        Layout.fillWidth: true
        Layout.fillHeight: true
        backdrop: root.backdrop
        cornerRadius: 16
        blurEnabled: root.glassEffectsEnabled
        blurRadius: 20
        fillColor: Qt.rgba(1, 1, 1, 0.14)
        strokeColor: Qt.rgba(1, 1, 1, 0.54)
        glowTopColor: Qt.rgba(1, 1, 1, 0.25)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.04)

        Loader {
            id: r18PaneLoader
            anchors.fill: parent
            anchors.margins: 8
            active: root.r18PaneWarm
            asynchronous: true
            sourceComponent: Component {
                R18ShellPane {
                    backdrop: root.backdrop
                    r18Controller: root.r18Controller
                    viewMode: root.r18ViewMode
                    onViewModeChanged: root.viewModeChangedByUser(viewMode)
                }
            }
            onLoaded: {
                if (item) {
                    item.viewMode = root.r18ViewMode
                }
                root.pendingFlipReady()
            }
            onStatusChanged: root.pendingFlipReady()
        }
    }
}
