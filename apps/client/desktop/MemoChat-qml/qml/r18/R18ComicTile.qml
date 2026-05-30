import QtQuick 2.15

Rectangle {
    id: root

    property string coverSource: ""
    property string title: ""
    property string subtitle: ""
    property var tags: []
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color panelFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color textPrimaryColor: "#263241"
    property color textMutedColor: "#7b8794"
    property color badgeFillColor: "#dce6f8"
    property color badgeTextColor: "#526173"

    signal activated()

    height: 280
    radius: 10
    clip: true
    color: tileMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
    border.color: root.fieldStrokeColor

    Image {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 176
        source: root.coverSource
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 104
        color: root.panelFillColor
        border.color: root.fieldStrokeColor

        Column {
            anchors.fill: parent
            anchors.margins: 9
            spacing: 5

            Text {
                width: parent.width
                text: root.title
                color: root.textPrimaryColor
                font.pixelSize: 13
                font.bold: true
                maximumLineCount: 2
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: root.subtitle
                color: root.textMutedColor
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Flow {
                width: parent.width
                height: 24
                spacing: 5
                clip: true

                Repeater {
                    model: root.tags.slice(0, 3)

                    delegate: Rectangle {
                        width: Math.min(74, tagText.implicitWidth + 14)
                        height: 20
                        radius: 10
                        color: root.badgeFillColor

                        Text {
                            id: tagText
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 7
                            anchors.rightMargin: 7
                            text: modelData
                            color: root.badgeTextColor
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: tileMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated()
    }
}
