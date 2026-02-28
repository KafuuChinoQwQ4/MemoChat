import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

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
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#f7f8fa"
            border.color: "#e5e8ef"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 4
                    color: "#dde3ec"
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
                    color: "#354052"
                    font.pixelSize: 14
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.openApplyRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: "#f2f4f7"
            border.color: "#e8ecf2"

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: "联系人"
                color: "#7d8898"
                font.pixelSize: 12
            }
        }

        ListView {
            id: contactList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.contactModel
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
                width: ListView.view.width
                height: 58
                color: ListView.isCurrentItem ? "#eef2f8" : "transparent"

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
                        color: "#dfe4eb"
                        Image {
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectCrop
                            source: icon
                        }
                    }

                    Label {
                        text: name
                        color: "#354052"
                        Layout.fillWidth: true
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        contactList.currentIndex = index
                        root.contactIndexSelected(index)
                    }
                }
            }
        }
    }
}
