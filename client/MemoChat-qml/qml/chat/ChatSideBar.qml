import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "#2f3844"
    width: 56

    property int currentTab: 0
    property string userIcon: "qrc:/res/head_1.jpg"
    property bool hasPendingApply: false
    signal tabSelected(int tab)

    function iconForTab(tab, selected) {
        if (tab === 0) {
            return selected ? "qrc:/res/chat_icon_select_press.png" : "qrc:/res/chat_icon.png"
        }
        if (tab === 1) {
            return selected ? "qrc:/res/contact_list_press.png" : "qrc:/res/contact_list.png"
        }
        return selected ? "qrc:/res/settings_select_press.png" : "qrc:/res/settings.png"
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 28
        spacing: 28

        Rectangle {
            width: 35
            height: 35
            radius: 17
            clip: true
            color: "#415164"
            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: root.userIcon
            }
        }

        Repeater {
            model: [0, 1, 2]
            delegate: Item {
                width: 32
                height: 32

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: root.currentTab === modelData ? "#455469" : "transparent"
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: root.iconForTab(modelData, root.currentTab === modelData)
                    fillMode: Image.PreserveAspectFit
                }

                Rectangle {
                    visible: modelData === 1 && root.hasPendingApply
                    width: 8
                    height: 8
                    radius: 4
                    color: "#e84141"
                    anchors.right: parent.right
                    anchors.top: parent.top
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.tabSelected(modelData)
                }
            }
        }
    }
}
