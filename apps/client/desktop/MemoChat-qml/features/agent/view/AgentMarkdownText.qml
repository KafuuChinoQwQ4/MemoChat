pragma ComponentBehavior: Bound

import QtQuick 2.15
import "../runtime/AgentMarkdownRuntime.js" as AgentMarkdownRuntime

Column {
    id: root

    property string text: ""
    property color textColor: "#253247"
    property color codeTextColor: "#314158"
    property int textPixelSize: 14
    property int codePixelSize: 12
    property real maxCodeBlockHeight: 520
    property var keywordCache: ({})
    readonly property var segments: AgentMarkdownRuntime.parseSegments(text)

    spacing: 7

    Repeater {
        model: root.segments

        delegate: Item {
            id: segmentDelegate
            required property var modelData

            width: Math.max(80, root.width)
            height: modelData.kind === "code" ? codeBlock.implicitHeight : paragraphText.implicitHeight

            TextEdit {
                id: paragraphText
                visible: segmentDelegate.modelData.kind !== "code"
                width: parent.width
                text: AgentMarkdownRuntime.renderRichText(segmentDelegate.modelData.text || "")
                color: root.textColor
                font.pixelSize: root.textPixelSize
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                cursorVisible: false
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
            }

            AgentMarkdownCodeBlock {
                id: codeBlock
                visible: segmentDelegate.modelData.kind === "code"
                width: parent.width
                text: segmentDelegate.modelData.text || ""
                language: segmentDelegate.modelData.language || ""
                codeTextColor: root.codeTextColor
                codePixelSize: root.codePixelSize
                maxCodeBlockHeight: root.maxCodeBlockHeight
                runtime: AgentMarkdownRuntime
                keywordCache: root.keywordCache
            }
        }
    }
}
