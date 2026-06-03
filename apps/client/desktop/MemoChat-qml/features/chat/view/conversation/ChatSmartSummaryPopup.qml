pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

GlassSurface {
    id: root

    required property int index
    required property string summaryTitle
    required property string summaryContent
    required property string summaryStatus
    required property bool summaryBusy
    property real availableWidth: 0

    signal closeRequested(int index)

    width: Math.min(root.availableWidth - 24, 420)
    height: Math.min(220, Math.max(124, summaryColumn.implicitHeight + 22))
    x: Math.max(0, root.availableWidth - width - (root.index % 2) * 26)
    y: 12 + root.index * 18
    cornerRadius: 12
    blurRadius: 18
    fillColor: Qt.rgba(1, 1, 1, 0.88)
    strokeColor: Qt.rgba(1, 1, 1, 0.62)

    ColumnLayout {
        id: summaryColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 7

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: root.summaryTitle
                color: "#26364d"
                font.pixelSize: 14
                font.bold: true
                elide: Text.ElideRight
            }

            ToolButton {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 26
                text: "x"
                onClicked: root.closeRequested(root.index)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.summaryBusy || root.summaryContent.length === 0
            text: root.summaryStatus
            color: root.summaryBusy ? "#4f6788" : "#9b4b4b"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.summaryContent.length > 0
            clip: true

            TextEdit {
                width: parent.width
                text: root.summaryContent
                readOnly: true
                selectByMouse: true
                cursorVisible: false
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                textFormat: Text.PlainText
                color: "#223047"
                font.pixelSize: 13
            }
        }
    }
}
