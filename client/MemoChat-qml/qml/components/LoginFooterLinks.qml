import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: root
    property Item moreOptionsAnchor: moreOptionsText
    signal scanLoginClicked()
    signal moreOptionsClicked()

    spacing: 14

    Label {
        text: "扫码登录"
        color: "#2c73df"
        font.pixelSize: 13

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.scanLoginClicked()
        }
    }

    Label {
        text: "|"
        color: "#b4b4b4"
        font.pixelSize: 13
    }

    Label {
        id: moreOptionsText
        text: "更多选项"
        color: "#2c73df"
        font.pixelSize: 13

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.moreOptionsClicked()
        }
    }
}
