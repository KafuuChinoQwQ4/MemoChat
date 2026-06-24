import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property Item backdrop: null
    property var contactModel: null
    property int selectedUid: 0

    signal friendSelected(int uid, string displayName)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            radius: 10
            property bool selected: root.selectedUid <= 0
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
                onClicked: root.friendSelected(0, "")
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
                    property bool selected: uid === root.selectedUid
                    readonly property string identityText: userId && userId.length > 0
                                                           ? ("ID: " + userId)
                                                           : "ID: 未分配"
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
                                text: momentFriendDelegate.identityText
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
                            root.friendSelected(uid, displayName)
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
}
