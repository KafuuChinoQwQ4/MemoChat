import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root

    property string sourceId: ""
    property string title: ""
    property string itemId: ""
    property string statusText: ""
    property string sourceUrl: ""
    property bool enabledState: true
    property bool hasEnabledState: false
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

    signal enableToggled(string sourceId, bool enabled)
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

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: root.title || root.itemId || "漫画源"
                    color: root.textPrimaryColor
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    visible: root.hasEnabledState
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 22
                    radius: 11
                    color: root.enabledState ? Qt.rgba(0.66, 0.82, 0.70, 0.34)
                                             : Qt.rgba(0.90, 0.60, 0.60, 0.26)

                    Text {
                        anchors.centerIn: parent
                        text: root.enabledState ? "启用" : "停用"
                        color: root.textPrimaryColor
                        font.pixelSize: 11
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.statusText
                color: root.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
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
            Layout.preferredWidth: 72
            Layout.preferredHeight: 34
            visible: root.hasEnabledState
            text: root.enabledState ? "停用" : "启用"
            textPixelSize: 13
            textColor: root.textSecondaryColor
            cornerRadius: 17
            normalColor: root.secondaryButtonColor
            hoverColor: root.secondaryButtonHoverColor
            pressedColor: root.secondaryButtonPressedColor
            onClicked: root.enableToggled(root.sourceId, !root.enabledState)
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
