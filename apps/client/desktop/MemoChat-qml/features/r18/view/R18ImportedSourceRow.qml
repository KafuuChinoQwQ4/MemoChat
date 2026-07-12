import QtQuick 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Rectangle {
    id: root

    property string sourceId: ""
    property string title: ""
    property string itemId: ""
    property string statusText: ""
    property string sourceUrl: ""
    property bool builtinSource: false
    property bool selected: false
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color itemSelectedFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.30)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    property color importButtonColor: "#0c4f92"
    property color importButtonHoverColor: "#0f61b0"
    property color importButtonPressedColor: "#093d72"

    signal openRequested(string sourceId)

    width: ListView.view ? ListView.view.width : 360
    height: 96
    radius: 8
    color: root.selected ? root.itemSelectedFillColor
          : (importedSourceMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor)
    border.color: root.selected ? Qt.rgba(0.28, 0.45, 0.67, 0.42) : root.fieldStrokeColor

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: root.title || root.itemId || "漫画源"
                color: root.textPrimaryColor
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    visible: root.builtinSource
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: 20
                    radius: 10
                    color: Qt.rgba(0.35, 0.48, 0.62, 0.16)
                    border.color: Qt.rgba(0.28, 0.42, 0.56, 0.22)

                    Text {
                        anchors.centerIn: parent
                        text: "内置源"
                        color: root.textSecondaryColor
                        font.pixelSize: 11
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.statusText
                    color: root.textSecondaryColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.sourceUrl
                color: root.textMutedColor
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        GlassButton {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 34
            text: "进入"
            textPixelSize: 13
            textColor: "#ffffff"
            cornerRadius: 17
            normalColor: root.importButtonColor
            hoverColor: root.importButtonHoverColor
            pressedColor: root.importButtonPressedColor
            onClicked: root.openRequested(root.sourceId)
        }
    }

    MouseArea {
        id: importedSourceMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
