import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root

    property string title: ""
    property var comboId: null
    property color textSecondaryColor: "#4e5d74"

    Layout.fillWidth: true
    spacing: 5

    Label {
        Layout.fillWidth: true
        text: root.title
        color: root.textSecondaryColor
        font.pixelSize: 12
        font.bold: true
        elide: Text.ElideRight
    }

    ComboBox {
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        model: root.comboId ? root.comboId.model : []
        currentIndex: root.comboId ? root.comboId.currentIndex : 0
        font.pixelSize: 13
        onActivated: function(index) {
            if (root.comboId) {
                root.comboId.currentIndex = index
            }
        }
        background: Rectangle {
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.34)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.46)
        }
    }
}
