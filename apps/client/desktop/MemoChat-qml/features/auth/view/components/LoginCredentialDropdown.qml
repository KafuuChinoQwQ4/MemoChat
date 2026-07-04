pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import "qrc:/qml/components"
import "qrc:/qml/linux/components" as LinuxComponents

Popup {
    id: root
    parent: Overlay.overlay
    z: 1100
    width: 320
    height: panelHeight()
    padding: 0
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    opacity: 1.0
    scale: popupScale
    transformOrigin: Item.Top

    property Item hostItem: null
    property Item anchorItem: null
    property Item backdrop: null
    property var credentialModel: null
    property bool linuxStyle: false
    property real popupScale: 1.0
    property color surfaceFillColor: Qt.rgba(0.97, 0.985, 1.0, 0.93)
    property color surfaceStrokeColor: Qt.rgba(0.78, 0.86, 0.94, 0.66)
    property color itemBaseColor: Qt.rgba(1, 1, 1, 0.62)

    signal credentialSelected(int index)

    function credentialCount() {
        return credentialModel ? credentialModel.count : 0
    }

    function panelHeight() {
        if (credentialCount() <= 0) {
            // Empty state: header (58) + centered title/subtitle block + bottom padding.
            return 58 + 96 + 14
        }
        const listHeight = Math.min(credentialCount() * 56, 224)
        return 58 + listHeight + 14
    }

    function reposition() {
        if (!root.parent || !root.anchorItem || !root.hostItem) {
            return
        }
        const overlayPoint = root.anchorItem.mapToItem(root.parent, 0, root.anchorItem.height)
        if (!isFinite(overlayPoint.x) || !isFinite(overlayPoint.y)) {
            return
        }

        const horizontalMargin = 12
        const popupWidth = Math.min(Math.max(root.anchorItem.width, 280),
                                    root.hostItem.width - horizontalMargin * 2)
        const maxX = Math.max(horizontalMargin, root.hostItem.width - popupWidth - horizontalMargin)
        root.width = popupWidth
        root.x = Math.max(horizontalMargin, Math.min(overlayPoint.x, maxX))
        root.y = overlayPoint.y + 10
    }

    onAboutToShow: {
        root.popupScale = 0.94
        root.reposition()
    }
    onOpened: Qt.callLater(function() { root.reposition() })

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 150
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                target: root
                property: "popupScale"
                from: 0.94
                to: 1.0
                duration: 180
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 110
                easing.type: Easing.InQuad
            }
            NumberAnimation {
                target: root
                property: "popupScale"
                from: 1.0
                to: 0.97
                duration: 110
                easing.type: Easing.InQuad
            }
        }
    }

    background: Item {
        GlassSurface {
            anchors.fill: parent
            visible: !root.linuxStyle
            backdrop: root.backdrop
            blurRadius: 28
            cornerRadius: 18
            fillColor: root.surfaceFillColor
            strokeColor: root.surfaceStrokeColor
            strokeWidth: 1
            glowTopColor: Qt.rgba(1, 1, 1, 0.42)
            glowBottomColor: Qt.rgba(0.70, 0.80, 0.92, 0.10)
        }

        LinuxComponents.GlassSurface {
            anchors.fill: parent
            visible: root.linuxStyle
            backdrop: root.backdrop
            blurRadius: 28
            cornerRadius: 18
            fillColor: root.surfaceFillColor
            strokeColor: root.surfaceStrokeColor
            strokeWidth: 1
            glowTopColor: Qt.rgba(1, 1, 1, 0.42)
            glowBottomColor: Qt.rgba(0.70, 0.80, 0.92, 0.10)
        }
    }

    contentItem: Item {
        implicitWidth: root.width
        implicitHeight: root.height

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10
            height: 48
            radius: 14
            color: Qt.rgba(1, 1, 1, 0.62)
            border.width: 1
            border.color: Qt.rgba(0.80, 0.87, 0.95, 0.58)

            Row {
                anchors.left: parent.left
                anchors.right: countBadge.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 14
                anchors.rightMargin: 10
                spacing: 10

                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    anchors.verticalCenter: parent.verticalCenter
                    color: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                    border.width: 1
                    border.color: Qt.rgba(0.71, 0.86, 1.0, 0.42)

                    Text {
                        anchors.centerIn: parent
                        text: "\u2198"
                        color: "#33698a"
                        font.pixelSize: 16
                        font.bold: true
                    }
                }

                Column {
                    width: parent.width - 28 - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        width: parent.width
                        text: "最近登录"
                        color: "#233244"
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: root.credentialCount() > 0
                              ? "选择一个历史邮箱快速填充"
                              : "登录成功后会自动保存在这里"
                        color: "#6a7d90"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle {
                id: countBadge
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 12
                width: 46
                height: 26
                radius: 13
                color: Qt.rgba(0.24, 0.46, 0.66, 0.10)
                border.width: 1
                border.color: Qt.rgba(0.60, 0.78, 0.95, 0.46)

                Text {
                    anchors.centerIn: parent
                    text: root.credentialCount() > 0 ? root.credentialCount() : 0
                    color: "#2b5876"
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 64
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10

            Column {
                anchors.centerIn: parent
                width: parent.width - 28
                spacing: 8
                visible: root.credentialCount() <= 0

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "暂无历史账号"
                    color: "#274056"
                    font.pixelSize: 15
                    font.bold: true
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "完成一次登录后，这里会展示最近使用过的邮箱。"
                    color: "#72859a"
                    font.pixelSize: 12
                }
            }

            ListView {
                id: credentialList
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                clip: true
                visible: root.credentialCount() > 0
                spacing: 8
                model: root.credentialModel
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: GlassScrollBar { }

                delegate: Item {
                    id: credentialDelegate
                    required property int index
                    required property string email

                    width: credentialList.width
                    height: 56

                    Rectangle {
                        anchors.fill: parent
                        radius: 14
                        color: credentialMouse.containsMouse
                               ? Qt.rgba(0.82, 0.90, 0.99, 0.92)
                               : root.itemBaseColor
                        border.width: 1
                        border.color: credentialMouse.containsMouse
                                      ? Qt.rgba(0.71, 0.86, 1.0, 0.50)
                                      : Qt.rgba(0.83, 0.89, 0.95, 0.56)

                        Behavior on color {
                            ColorAnimation {
                                duration: 120
                                easing.type: Easing.OutQuad
                            }
                        }

                        Behavior on border.color {
                            ColorAnimation {
                                duration: 120
                                easing.type: Easing.OutQuad
                            }
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 34
                        height: 34
                        radius: 17
                        color: Qt.rgba(0.82, 0.90, 0.98, 0.95)
                        border.width: 1
                        border.color: Qt.rgba(0.71, 0.86, 1.0, 0.55)

                        Text {
                            anchors.centerIn: parent
                            text: credentialDelegate.email.length > 0 ? credentialDelegate.email.charAt(0).toUpperCase() : "@"
                            color: "#2f5d7f"
                            font.pixelSize: 15
                            font.bold: true
                        }
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 58
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Text {
                            text: credentialDelegate.email
                            color: "#203246"
                            font.pixelSize: 14
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            text: "点击填入邮箱"
                            color: "#6b7f92"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        id: credentialMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.credentialSelected(credentialDelegate.index)
                    }
                }
            }
        }
    }
}
