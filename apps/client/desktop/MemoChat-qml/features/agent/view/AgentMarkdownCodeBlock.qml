pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import "AgentMarkdownRuntime.js" as AgentMarkdownRuntime

Rectangle {
    id: root

    property string text: ""
    property string language: ""
    property color codeTextColor: "#314158"
    property int codePixelSize: 12
    property real maxCodeBlockHeight: 520
    property var runtime: AgentMarkdownRuntime
    property var keywordCache: ({})

    implicitHeight: codeHeader.height + codeBody.height
    radius: 9
    color: Qt.rgba(0.90, 0.95, 1.0, 0.72)
    border.width: 1
    border.color: Qt.rgba(0.35, 0.61, 0.90, 0.26)
    clip: true

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    Rectangle {
        id: codeHeader
        width: parent.width
        height: 30
        color: Qt.rgba(0.82, 0.90, 1.0, 0.54)

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: root.runtime.languageLabel(root.language)
            color: "#55708f"
            font.pixelSize: 11
            font.bold: true
            elide: Text.ElideRight
            width: parent.width - 20
        }
    }

    Rectangle {
        id: codeBody
        anchors.top: codeHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.clamp(codeFlick.contentHeight + 14, 42, root.maxCodeBlockHeight)
        color: Qt.rgba(0.97, 0.99, 1.0, 0.66)

        Flickable {
            id: codeFlick
            anchors.fill: parent
            anchors.margins: 7
            clip: true
            contentWidth: Math.max(width, codeText.implicitWidth + 2)
            contentHeight: codeText.implicitHeight + 2
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalAndVerticalFlick
            interactive: contentWidth > width || contentHeight > height

            TextEdit {
                id: codeText
                x: 0
                y: 0
                width: Math.max(codeFlick.width, implicitWidth)
                text: root.runtime.highlightedCode(root.text, root.language, root.codeTextColor, root.keywordCache)
                color: root.codeTextColor
                font.family: "Consolas"
                font.pixelSize: root.codePixelSize
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.NoWrap
                readOnly: true
                selectByMouse: true
                cursorVisible: false
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
            }

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
                height: 6
                contentItem: Rectangle {
                    implicitHeight: 4
                    radius: 2
                    color: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                }
                background: Rectangle {
                    implicitHeight: 6
                    radius: 3
                    color: Qt.rgba(0.35, 0.61, 0.90, 0.10)
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                width: 6
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                }
                background: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: Qt.rgba(0.35, 0.61, 0.90, 0.10)
                }
            }
        }
    }
}
