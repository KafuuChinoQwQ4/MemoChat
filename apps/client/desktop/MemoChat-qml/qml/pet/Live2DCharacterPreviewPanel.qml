import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Live2DSectionPanel {
    id: root

    property Item backdrop: null
    property string characterName: ""
    property string roleIdentity: ""
    property string modelRoot: ""
    property string modelJson: ""
    property string characterAvatarSource: ""
    property string characterAvatarFallback: "qrc:/icons/modelive2d.png"
    property color textPrimaryColor: "#253247"
    property color textMutedColor: "#6e7f95"

    signal characterNameEdited(string text)
    signal roleIdentityEdited(string text)
    signal modelRootEdited(string text)
    signal modelJsonEdited(string text)
    signal modelJsonPickRequested()
    signal modelRootPickRequested()

    title: "角色预览"
    subtitle: "选择当前角色基础信息和资源目录"

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 170
            Layout.preferredHeight: 220
            Layout.minimumWidth: 140
            radius: 8
            color: Qt.rgba(0.85, 0.91, 0.98, 0.45)
            border.color: Qt.rgba(1, 1, 1, 0.62)
            clip: true

            Live2DAvatarPreviewImage {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 9
                width: Math.min(parent.width - 18, parent.height - 72)
                height: width
                imageSource: root.characterAvatarSource
                fallbackSource: root.characterAvatarFallback
                fallbackInset: 18
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 56
                color: Qt.rgba(1, 1, 1, 0.50)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: root.characterName.length > 0 ? root.characterName : "未命名角色"
                        color: root.textPrimaryColor
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.roleIdentity
                        color: root.textMutedColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Live2DFormFieldBlock {
                    backdrop: root.backdrop
                    title: "角色名"
                    text: root.characterName
                    placeholderText: "例如 Kafuu Chino"
                    onTextChanged: root.characterNameEdited(text)
                }

                Live2DFormFieldBlock {
                    backdrop: root.backdrop
                    title: "角色定位"
                    text: root.roleIdentity
                    placeholderText: "例如 桌宠助手"
                    onTextChanged: root.roleIdentityEdited(text)
                }
            }

            Live2DFormFieldBlock {
                backdrop: root.backdrop
                title: "模型根目录"
                text: root.modelRoot
                placeholderText: "resources/live2d/KafuuChino/香风智乃live2D"
                onTextChanged: root.modelRootEdited(text)
            }

            Live2DFormFieldBlock {
                backdrop: root.backdrop
                title: "model3.json"
                text: root.modelJson
                placeholderText: "选择 Live2D model3.json"
                onTextChanged: root.modelJsonEdited(text)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                GlassButton {
                    Layout.preferredWidth: 92
                    Layout.preferredHeight: 32
                    text: "选模型"
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                    onClicked: root.modelJsonPickRequested()
                }

                GlassButton {
                    Layout.preferredWidth: 92
                    Layout.preferredHeight: 32
                    text: "选目录"
                    textPixelSize: 12
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.32, 0.60, 0.44, 0.18)
                    hoverColor: Qt.rgba(0.32, 0.60, 0.44, 0.28)
                    pressedColor: Qt.rgba(0.32, 0.60, 0.44, 0.36)
                    onClicked: root.modelRootPickRequested()
                }

                Label {
                    Layout.fillWidth: true
                    text: "选择模型文件、动作目录或整套角色资源。"
                    color: root.textMutedColor
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }
    }
}
