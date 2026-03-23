import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "transparent"

    property bool hasContact: false
    property string contactName: ""
    property string contactNick: ""
    property string contactIcon: "qrc:/res/head_1.jpg"
    property string contactBack: ""
    property int contactSex: 0
    property string contactUserId: ""

    signal messageChatClicked()
    signal voiceChatClicked()
    signal videoChatClicked()

    readonly property string genderIconSource: {
        if (root.contactSex === 1) return "qrc:/icons/gender_female.png"
        if (root.contactSex === 0) return "qrc:/icons/gender_male.png"
        return "qrc:/icons/gender_unknown.png"
    }
    readonly property string genderLabelText: {
        if (root.contactSex === 1) return "女"
        if (root.contactSex === 0) return "男"
        return ""
    }

    readonly property int avatarSide: 72
    readonly property int actionIcon: 28

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.topMargin: 28
        anchors.bottomMargin: 24
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 16

            // 左：头像 + 三按钮（按钮整体在头像正下方、水平居中）
            ColumnLayout {
                id: leftBlock
                spacing: 12
                Layout.alignment: Qt.AlignTop

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: root.avatarSide
                    height: root.avatarSide
                    radius: 8
                    clip: true
                    color: Qt.rgba(0.73, 0.83, 0.93, 0.45)

                    Image {
                        anchors.fill: parent
                        source: root.contactIcon
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        mipmap: true
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.alignment: Qt.AlignHCenter

                    ContactActionIconButton {
                        iconSize: root.actionIcon
                        labelText: "发消息"
                        normalSource: "qrc:/icons/contact_message.png"
                        hoverSource: "qrc:/icons/contact_message.png"
                        pressedSource: "qrc:/icons/contact_message.png"
                        enabled: root.hasContact
                        onClicked: root.messageChatClicked()
                    }

                    ContactActionIconButton {
                        iconSize: root.actionIcon
                        labelText: "语音"
                        normalSource: "qrc:/icons/phone.png"
                        hoverSource: "qrc:/icons/phone.png"
                        pressedSource: "qrc:/icons/phone.png"
                        enabled: root.hasContact
                        onClicked: root.voiceChatClicked()
                    }

                    ContactActionIconButton {
                        iconSize: root.actionIcon
                        labelText: "视频"
                        normalSource: "qrc:/icons/video.png"
                        hoverSource: "qrc:/icons/video.png"
                        pressedSource: "qrc:/icons/video.png"
                        enabled: root.hasContact
                        onClicked: root.videoChatClicked()
                    }
                }
            }

            // 右：昵称、性别、备注、ID — 与头像顶对齐、左齐
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 6

                Label {
                    text: root.contactName
                    color: "#1e2a3a"
                    font.pixelSize: 18
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: 6
                    Layout.preferredHeight: 18

                    Item {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        Layout.maximumWidth: 16
                        Layout.maximumHeight: 16
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: root.genderIconSource
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                        }
                    }

                    Label {
                        visible: root.genderLabelText.length > 0
                        text: root.genderLabelText
                        color: "#3d4f66"
                        font.pixelSize: 13
                    }
                }

                RowLayout {
                    spacing: 6
                    Label {
                        text: "备注"
                        color: "#5a6b82"
                        font.pixelSize: 13
                    }
                    Label {
                        text: root.contactBack.length > 0 ? root.contactBack : root.contactNick
                        color: "#1e2a3a"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: 6
                    Label {
                        text: "ID"
                        color: "#5a6b82"
                        font.pixelSize: 13
                    }
                    Label {
                        text: root.contactUserId
                        color: "#1e2a3a"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Label {
            visible: !root.hasContact
            text: "请选择联系人"
            color: "#62758d"
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
