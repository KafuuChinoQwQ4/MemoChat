import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "transparent"
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

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Qt.rgba(0.16, 0.22, 0.31, 0.44)
        border.color: Qt.rgba(1, 1, 1, 0.16)
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 24
        spacing: 28

        Rectangle {
            width: 35
            height: 35
            radius: 17
            clip: true
            color: Qt.rgba(0.47, 0.63, 0.83, 0.36)
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
                    color: root.currentTab === modelData
                           ? Qt.rgba(0.75, 0.87, 1.0, 0.28)
                           : "transparent"
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
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tabSelected(modelData)
                }
            }
        }
    }
}
