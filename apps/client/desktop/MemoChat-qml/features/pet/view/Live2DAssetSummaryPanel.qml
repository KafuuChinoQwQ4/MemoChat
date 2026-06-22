import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

ColumnLayout {
    id: root

    property string statusLabel: ""
    property color statusColor: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property string statusText: ""
    property int motionCount: 0
    property int expressionCount: 0
    property int textureCount: 0
    property int voiceCount: 0
    property int referencedFileCount: 0
    property string issueText: ""
    property string physicsLabel: ""
    property string poseLabel: ""
    property string userDataLabel: ""
    property string mappingLabel: ""
    property string checksumText: ""
    property var actionItems: []
    property color textPrimaryColor: "#253247"
    property color textSecondaryColor: "#4e5d74"
    property color textMutedColor: "#6e7f95"
    property color accentBlue: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color accentGreen: Qt.rgba(0.32, 0.60, 0.44, 1.0)
    property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)

    signal validateRequested()

    spacing: 8

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Live2DStatusChip {
            text: root.statusLabel
            colorBase: root.statusColor
            textColor: root.textSecondaryColor
        }

        Label {
            Layout.fillWidth: true
            text: root.statusText
            color: root.textSecondaryColor
            font.pixelSize: 12
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        GlassButton {
            Layout.preferredWidth: 74
            Layout.preferredHeight: 30
            text: "校验"
            textPixelSize: 12
            cornerRadius: 8
            normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
            hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
            pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
            onClicked: root.validateRequested()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Live2DStatusChip { text: "动作 " + root.motionCount; textColor: root.textSecondaryColor }
        Live2DStatusChip { text: "表情 " + root.expressionCount; colorBase: root.accentRose; textColor: root.textSecondaryColor }
        Live2DStatusChip { text: "贴图 " + root.textureCount; colorBase: root.accentGreen; textColor: root.textSecondaryColor }
        Live2DStatusChip { text: "语音 " + root.voiceCount; textColor: root.textSecondaryColor }
        Live2DStatusChip { text: "文件 " + root.referencedFileCount; colorBase: root.accentGreen; textColor: root.textSecondaryColor }

        Label {
            Layout.fillWidth: true
            text: root.issueText
            visible: text.length > 0
            color: root.textMutedColor
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
