import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property Item backdrop: null
    property string avatarSource: ""
    property string avatarFallback: "qrc:/icons/modelive2d.png"
    property string characterName: ""
    property string modelPath: ""
    property bool hasImportedModel: false

    readonly property var setupRows: [
        { "title": "资源包", "subtitle": "model3.json、贴图、动作和表情", "status": "本地" },
        { "title": "语音资源", "subtitle": "音色、短音效、口型同步", "status": "草稿" },
        { "title": "人物人设", "subtitle": "身份、背景、关系和边界", "status": "可编辑" },
        { "title": "说话风格", "subtitle": "语气、长度、语言和口头禅", "status": "可调" },
        { "title": "行为记忆", "subtitle": "待机动作、视线、记忆和权限", "status": "配置" }
    ]

    function trimmed(value) {
        return (value || "").toString().replace(/^\s+|\s+$/g, "")
    }
    function displayCharacterName() {
        if (!root.hasImportedModel) {
            return "未导入角色"
        }
        var name = root.trimmed(root.characterName)
        return name.length > 0 ? name : "未命名角色"
    }
    function displayModelPath() {
        if (!root.hasImportedModel) {
            return "等待导入 Live2D model3.json"
        }
        var path = root.trimmed(root.modelPath)
        return path.length > 0 ? path : "已导入 Live2D 角色"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 128
            radius: 10
            color: Qt.rgba(0.82, 0.90, 0.98, 0.25)
            border.color: Qt.rgba(1, 1, 1, 0.42)
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 54
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.34)
                    border.color: Qt.rgba(1, 1, 1, 0.54)
                    clip: true

                    Image {
                        id: live2dEntryAvatarImage
                        anchors.centerIn: parent
                        width: 48
                        height: 48
                        source: root.hasImportedModel ? root.avatarSource : ""
                        fillMode: Image.PreserveAspectCrop
                        sourceSize.width: 96
                        sourceSize.height: 96
                        cache: false
                        visible: root.hasImportedModel && status === Image.Ready
                        mipmap: true
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        source: root.avatarFallback
                        fillMode: Image.PreserveAspectFit
                        visible: !root.hasImportedModel || live2dEntryAvatarImage.status !== Image.Ready
                        opacity: root.hasImportedModel ? 1.0 : 0.48
                        mipmap: true
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.displayCharacterName()
                    color: "#273449"
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: root.displayModelPath()
                    color: "#6a7b92"
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: "配置目录"
            color: "#687991"
            font.pixelSize: 12
            font.bold: true
            elide: Text.ElideRight
        }

        ListView {
            id: live2dSetupList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: root.setupRows
            interactive: contentHeight > height
            ScrollBar.vertical: GlassScrollBar { }

            delegate: Rectangle {
                width: ListView.view.width
                implicitHeight: 64
                radius: 10
                color: rowMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.06)
                border.color: Qt.rgba(1, 1, 1, 0.26)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.fillHeight: true
                        radius: 4
                        color: modelData.status === "本地" ? "#4f82c4"
                              : modelData.status === "草稿" ? "#5f9a78"
                              : modelData.status === "配置" ? "#b46d63"
                              : "#8a7ac3"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: modelData.title
                            color: "#273449"
                            font.pixelSize: 13
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.subtitle
                            color: "#6a7b92"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: Math.max(44, statusText.implicitWidth + 14)
                        Layout.preferredHeight: 24
                        radius: 8
                        color: Qt.rgba(0.35, 0.61, 0.90, 0.14)

                        Label {
                            id: statusText
                            anchors.centerIn: parent
                            text: modelData.status
                            color: "#4d6280"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }
        }
    }
}
