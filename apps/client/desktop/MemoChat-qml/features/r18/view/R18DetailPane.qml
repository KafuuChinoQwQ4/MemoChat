import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property string coverSource: ""
    property string title: ""
    property string description: ""
    property var chapterModel: null
    property bool favorite: false
    property color panelStrongFillColor: Qt.rgba(1, 1, 1, 0.20)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal backRequested()
    signal favoriteToggled(bool favorite)
    signal chapterOpened(string sourceId, string chapterId)

    RowLayout {
        anchors.fill: parent
        spacing: 14

        Rectangle {
            Layout.preferredWidth: Math.min(240, parent.width * 0.34)
            Layout.fillHeight: true
            radius: 12
            color: root.panelStrongFillColor
            border.color: root.fieldStrokeColor
            clip: true

            Image {
                anchors.fill: parent
                anchors.margins: 10
                source: root.coverSource
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                GlassButton {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 32
                    text: "返回"
                    textColor: root.textSecondaryColor
                    cornerRadius: 8
                    normalColor: root.secondaryButtonColor
                    hoverColor: root.secondaryButtonHoverColor
                    pressedColor: root.secondaryButtonPressedColor
                    onClicked: root.backRequested()
                }

                GlassButton {
                    Layout.preferredWidth: 86
                    Layout.preferredHeight: 32
                    text: root.favorite ? "已收藏" : "收藏"
                    textColor: root.primaryButtonTextColor
                    cornerRadius: 8
                    normalColor: root.primaryButtonColor
                    hoverColor: root.primaryButtonHoverColor
                    pressedColor: root.primaryButtonPressedColor
                    onClicked: root.favoriteToggled(!root.favorite)
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.title
                color: root.textPrimaryColor
                font.pixelSize: 24
                font.bold: true
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.description
                color: root.textSecondaryColor
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: "章节"
                color: root.textSecondaryColor
                font.pixelSize: 15
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                reuseItems: true
                cacheBuffer: 480
                spacing: 6
                model: root.chapterModel
                ScrollBar.vertical: GlassScrollBar {}

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 48
                    radius: 8
                    color: chapterMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                    border.color: root.fieldStrokeColor

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        text: model.title
                        color: root.textPrimaryColor
                        font.pixelSize: 14
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        id: chapterMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.chapterOpened(model.sourceId, model.itemId)
                    }
                }
            }
        }
    }
}
