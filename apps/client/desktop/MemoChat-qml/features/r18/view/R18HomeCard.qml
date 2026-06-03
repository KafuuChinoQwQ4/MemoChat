import QtQuick 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Rectangle {
    id: root

    property string title: ""
    property int count: 0
    property int modeValue: 0
    property string entryAction: "mode"
    property color textPrimaryColor: "#263241"
    property color cardFillColor: "#ffffff"
    property color cardStrokeColor: "#d8dde6"
    property color badgeFillColor: "#dce6f8"
    property color badgeTextColor: "#526173"
    property color arrowColor: "#2f343c"
    property color importButtonColor: "#0c4f92"
    property color importButtonHoverColor: "#0f61b0"
    property color importButtonPressedColor: "#093d72"

    signal activated(string entryAction, int modeValue)
    signal importRequested()

    width: ListView.view ? ListView.view.width : 240
    height: root.entryAction === "import" ? 106 : 72
    radius: 10
    color: homeCardMouse.containsMouse ? Qt.rgba(0.985, 0.988, 0.992, 1.0) : root.cardFillColor
    border.color: root.cardStrokeColor

    MouseArea {
        id: homeCardMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated(root.entryAction, root.modeValue)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 16
        anchors.bottomMargin: root.entryAction === "import" ? 16 : 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: implicitWidth
                text: root.title
                color: root.textPrimaryColor
                font.pixelSize: 17
                elide: Text.ElideRight
            }

            Rectangle {
                Layout.preferredWidth: countText.implicitWidth + 18
                Layout.preferredHeight: 26
                radius: 9
                color: root.badgeFillColor

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: root.count.toString()
                    color: root.badgeTextColor
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "›"
                color: root.arrowColor
                font.pixelSize: 24
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.entryAction === "import"
        }
    }

    GlassButton {
        visible: root.entryAction === "import"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 18
        anchors.bottomMargin: 14
        width: 92
        height: 40
        text: "导入"
        textPixelSize: 14
        textColor: "#ffffff"
        cornerRadius: 18
        normalColor: root.importButtonColor
        hoverColor: root.importButtonHoverColor
        pressedColor: root.importButtonPressedColor
        onClicked: root.importRequested()
    }
}
