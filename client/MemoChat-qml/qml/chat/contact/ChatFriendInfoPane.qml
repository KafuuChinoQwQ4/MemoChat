import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#f5f7fa"

    property bool hasContact: false
    property string contactName: ""
    property string contactNick: ""
    property string contactIcon: "qrc:/res/head_1.jpg"
    property string contactBack: ""
    property int contactSex: 0

    signal messageChatClicked()
    signal voiceChatClicked()
    signal videoChatClicked()

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 120
        anchors.rightMargin: 120
        anchors.topMargin: 90
        anchors.bottomMargin: 80
        spacing: 26

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            Rectangle {
                Layout.preferredWidth: 85
                Layout.preferredHeight: 85
                radius: 6
                clip: true
                color: "#e1e6ef"

                Image {
                    anchors.fill: parent
                    source: root.contactIcon
                    fillMode: Image.PreserveAspectCrop
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: root.contactName
                    color: "#161616"
                    font.pixelSize: 22
                    font.bold: true
                }

                RowLayout {
                    spacing: 6
                    Image {
                        width: 18
                        height: 18
                        source: root.contactSex === 1 ? "qrc:/res/female.png" : "qrc:/res/male.png"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                RowLayout {
                    spacing: 8
                    Label {
                        text: "备注:"
                        color: "#161616"
                        font.pixelSize: 14
                    }
                    Label {
                        text: root.contactBack.length > 0 ? root.contactBack : root.contactNick
                        color: "#161616"
                        font.pixelSize: 14
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 40

            ContactActionIconButton {
                normalSource: "qrc:/res/msg_chat_normal.png"
                hoverSource: "qrc:/res/msg_chat_hover.png"
                pressedSource: "qrc:/res/msg_chat_press.png"
                enabled: root.hasContact
                onClicked: root.messageChatClicked()
            }

            ContactActionIconButton {
                normalSource: "qrc:/res/voice_chat_normal.png"
                hoverSource: "qrc:/res/voice_chat_hover.png"
                pressedSource: "qrc:/res/voice_chat_press.png"
                enabled: root.hasContact
                onClicked: root.voiceChatClicked()
            }

            ContactActionIconButton {
                normalSource: "qrc:/res/video_chat_normal.png"
                hoverSource: "qrc:/res/video_chat_hover.png"
                pressedSource: "qrc:/res/video_chat_press.png"
                enabled: root.hasContact
                onClicked: root.videoChatClicked()
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Label {
            visible: !root.hasContact
            text: "请选择联系人"
            color: "#8b97a8"
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
