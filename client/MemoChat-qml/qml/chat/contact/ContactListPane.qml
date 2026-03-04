import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property var contactModel
    property bool hasPendingApply: false
    property bool canLoadMore: false
    property bool loadingMore: false
    signal openApplyRequested()
    signal contactIndexSelected(int index)
    signal loadMoreRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: newFriendEntry
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: newFriendArea.pressed ? Qt.rgba(1, 1, 1, 0.26)
                                         : newFriendArea.containsMouse ? Qt.rgba(1, 1, 1, 0.20)
                                                                      : Qt.rgba(1, 1, 1, 0.14)
            border.color: Qt.rgba(1, 1, 1, 0.34)

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            RowLayout {
                id: newFriendRow
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8
                scale: newFriendArea.pressed ? 0.96 : (newFriendArea.containsMouse ? 1.02 : 1.0)

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 4
                    color: Qt.rgba(0.74, 0.83, 0.93, 0.50)
                    Image {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        source: "qrc:/res/add_friend.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                Rectangle {
                    visible: root.hasPendingApply
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: "#e84141"
                    Layout.alignment: Qt.AlignTop
                }

                Label {
                    Layout.fillWidth: true
                    text: "新的朋友"
                    color: "#2c3a4f"
                    font.pixelSize: 14
                }
            }

            MouseArea {
                id: newFriendArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openApplyRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Qt.rgba(1, 1, 1, 0.12)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: "联系人"
                color: "#687991"
                font.pixelSize: 12
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: contactList
                anchors.fill: parent
                clip: true
                model: root.contactModel
                ScrollBar.vertical: GlassScrollBar { }
                onContentYChanged: {
                    if (!root.canLoadMore || root.loadingMore) {
                        return
                    }
                    if (contentHeight <= height + 1) {
                        return
                    }
                    if (contentY + height >= contentHeight - 48) {
                        root.loadMoreRequested()
                    }
                }

                footer: Item {
                    width: contactList.width
                    height: (root.loadingMore || root.canLoadMore) ? 40 : 0

                    Label {
                        anchors.centerIn: parent
                        text: root.loadingMore ? "加载中..." : (root.canLoadMore ? "上滑加载更多" : "")
                        color: "#8e9aac"
                        font.pixelSize: 12
                    }
                }

                delegate: Rectangle {
                    id: contactDelegate
                    width: ListView.view.width
                    height: 58
                    color: {
                        if (ListView.isCurrentItem) {
                            return contactArea.pressed ? Qt.rgba(0.77, 0.87, 0.98, 0.44)
                                                       : contactArea.containsMouse ? Qt.rgba(0.77, 0.87, 0.98, 0.36)
                                                                                 : Qt.rgba(0.77, 0.87, 0.98, 0.30)
                        }
                        if (contactArea.pressed) {
                            return Qt.rgba(0.77, 0.87, 0.98, 0.24)
                        }
                        if (contactArea.containsMouse) {
                            return Qt.rgba(0.77, 0.87, 0.98, 0.14)
                        }
                        return "transparent"
                    }
                    scale: contactArea.pressed ? 0.96 : (contactArea.containsMouse ? 1.02 : 1.0)

                    Behavior on color {
                        ColorAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }
                    Behavior on scale {
                        NumberAnimation {
                            duration: 110
                            easing.type: Easing.OutQuad
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 4
                            clip: true
                            color: Qt.rgba(0.74, 0.83, 0.93, 0.50)
                            Image {
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                source: icon
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: name
                                color: "#2c3a4f"
                                verticalAlignment: Text.AlignVCenter
                            }
                            Label {
                                text: "ID: " + (userId && userId.length > 0 ? userId : uid)
                                color: "#6c7c92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }

                    MouseArea {
                        id: contactArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            contactList.currentIndex = index
                            root.contactIndexSelected(index)
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        height: 1
                        color: Qt.rgba(1, 1, 1, 0.26)
                        visible: index < (contactList.count - 1)
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 160
                height: 62
                visible: contactList.count === 0 && !root.loadingMore
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 10
                blurRadius: 28
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                Label {
                    anchors.centerIn: parent
                    text: "暂无联系人"
                    color: "#6a7b92"
                    font.pixelSize: 13
                }
            }
        }
    }
}
