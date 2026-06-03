import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root

    property string title: ""
    property real value: 0.5
    property real fromValue: 0
    property real toValue: 1
    property string valueText: ""
    property color textSecondaryColor: "#4e5d74"
    property color textMutedColor: "#6e7f95"

    Layout.fillWidth: true
    spacing: 4

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.title
            color: root.textSecondaryColor
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            text: root.valueText
            color: root.textMutedColor
            font.pixelSize: 11
        }
    }

    Slider {
        Layout.fillWidth: true
        from: root.fromValue
        to: root.toValue
        value: root.value
        onMoved: root.value = value
    }
}
