import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var backdrop: null
    property int attachmentCount: 0
    property bool loading: false
    property bool publishEnabled: false

    signal backRequested()
    signal publishRequested()

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 0
        blurRadius: 12
        fillColor: Qt.rgba(1, 1, 1, 0.82)
        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 14
        spacing: 8

        ToolButton {
            id: backButton
            text: "←"
            font.pixelSize: 18
            implicitWidth: 36
            implicitHeight: 36
            background: Rectangle {
                radius: width / 2
                color: backButton.down ? "#dbe7f7" : Qt.rgba(1, 1, 1, 0.86)
                border.width: 1
                border.color: "#d5dfec"
            }
            contentItem: Label {
                text: "←"
                color: "#2d3b4f"
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.backRequested()
        }

        ColumnLayout {
            spacing: 1

            Label {
                text: "发布朋友圈"
                font.pixelSize: 17
                font.weight: Font.Medium
                color: "#1c2027"
            }

            Label {
                text: root.attachmentCount > 0 ? ("已选 " + root.attachmentCount + " 个素材") : ""
                font.pixelSize: 11
                color: "#79818f"
                visible: text.length > 0
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            id: publishButton
            text: root.loading ? "发布中..." : "发布"
            enabled: root.publishEnabled && !root.loading
            implicitHeight: 38
            leftPadding: 18
            rightPadding: 18
            topPadding: 7
            bottomPadding: 7
            background: Rectangle {
                radius: 19
                color: publishButton.enabled ? "#2b6ff2" : "#b9c4d6"
                border.width: 1
                border.color: publishButton.enabled ? "#2a67de" : "#b9c4d6"
            }
            contentItem: Label {
                text: publishButton.text
                color: "#ffffff"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.publishRequested()
        }
    }
}
