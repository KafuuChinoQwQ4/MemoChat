import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "settings"

Item {
    id: root
    property Item backdrop: null
    property string userIcon: "qrc:/res/head_1.jpg"
    property string userNick: ""
    property string userName: ""
    property string userDesc: ""
    property string userId: ""
    property string statusText: ""
    property bool statusError: false
    signal backRequested()
    signal chooseAvatarRequested()
    signal saveProfileRequested(string nick, string desc)
    signal statusCleared()

    readonly property int zoneColumns: width >= 1320 ? 3 : (width >= 910 ? 2 : 1)
    readonly property var accountModules: [
        { "title": "账号安全", "desc": "密码强度、设备管理与异地提醒。", "state": "即将上线" },
        { "title": "通知偏好", "desc": "消息提醒、免打扰时段与声音设置。", "state": "计划中" },
        { "title": "登录记录", "desc": "最近登录时间、终端与风险标记。", "state": "计划中" }
    ]
    readonly property var expansionModules: [
        { "title": "隐私权限", "desc": "谁可以找到你、加好友验证方式。", "state": "预留" },
        { "title": "主题外观", "desc": "色彩、气泡样式与界面密度。", "state": "预留" },
        { "title": "存储与缓存", "desc": "媒体缓存占用、清理策略和备份。", "state": "预留" },
        { "title": "插件扩展", "desc": "未来挂载工作流、AI 助手与机器人。", "state": "预留" }
    ]

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: container.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Column {
            id: container
            width: flick.width
            spacing: 12

            GlassSurface {
                width: parent.width
                height: 70
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 12
                blurRadius: 30
                fillColor: Qt.rgba(1, 1, 1, 0.22)
                strokeColor: Qt.rgba(1, 1, 1, 0.48)
                glowTopColor: Qt.rgba(1, 1, 1, 0.24)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.05)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

                    GlassButton {
                        text: "返回"
                        implicitWidth: 72
                        implicitHeight: 32
                        textPixelSize: 13
                        cornerRadius: 9
                        normalColor: Qt.rgba(0.33, 0.56, 0.82, 0.24)
                        hoverColor: Qt.rgba(0.33, 0.56, 0.82, 0.33)
                        pressedColor: Qt.rgba(0.33, 0.56, 0.82, 0.44)
                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                        onClicked: root.backRequested()
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: "个人中心"
                            color: "#223247"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Label {
                            text: "资料、偏好与后续扩展能力统一入口"
                            color: "#607289"
                            font.pixelSize: 12
                        }
                    }
                }
            }

            GridLayout {
                width: parent.width
                columns: root.zoneColumns
                columnSpacing: 12
                rowSpacing: 12

                GlassSurface {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 620
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    cornerRadius: 12
                    blurRadius: 30
                    fillColor: Qt.rgba(1, 1, 1, 0.18)
                    strokeColor: Qt.rgba(1, 1, 1, 0.46)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.22)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.04)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        Label {
                            text: "资料设置"
                            color: "#253549"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        SettingsAvatarCard {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 210
                            backdrop: root.backdrop
                            iconSource: root.userIcon
                            onChooseAvatarRequested: root.chooseAvatarRequested()
                        }

                        SettingsProfileForm {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            backdrop: root.backdrop
                            userName: root.userName
                            userNick: root.userNick
                            userDesc: root.userDesc
                            userId: root.userId
                            statusText: root.statusText
                            statusError: root.statusError
                            onSaveRequested: root.saveProfileRequested(nick, desc)
                            onStatusCleared: root.statusCleared()
                        }
                    }
                }

                GlassSurface {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 620
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    cornerRadius: 12
                    blurRadius: 30
                    fillColor: Qt.rgba(1, 1, 1, 0.16)
                    strokeColor: Qt.rgba(1, 1, 1, 0.42)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.20)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.03)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        Label {
                            text: "偏好与账号"
                            color: "#253549"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "该区域用于承接账号安全、提醒偏好等功能。"
                            color: "#64768c"
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }

                        Repeater {
                            model: root.accountModules
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 96
                                radius: 10
                                color: Qt.rgba(1, 1, 1, 0.34)
                                border.width: 1
                                border.color: Qt.rgba(1, 1, 1, 0.48)

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    Row {
                                        width: parent.width
                                        spacing: 8

                                        Label {
                                            text: modelData.title
                                            color: "#25364b"
                                            font.pixelSize: 14
                                            font.bold: true
                                        }

                                        Rectangle {
                                            height: 20
                                            radius: 10
                                            color: Qt.rgba(0.40, 0.63, 0.89, 0.20)
                                            border.width: 1
                                            border.color: Qt.rgba(0.36, 0.58, 0.85, 0.52)
                                            width: stateTag.implicitWidth + 12

                                            Label {
                                                id: stateTag
                                                anchors.centerIn: parent
                                                text: modelData.state
                                                color: "#305f8c"
                                                font.pixelSize: 11
                                            }
                                        }
                                    }

                                    Label {
                                        width: parent.width
                                        text: modelData.desc
                                        color: "#5f7086"
                                        font.pixelSize: 12
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                GlassSurface {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 620
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    cornerRadius: 12
                    blurRadius: 30
                    fillColor: Qt.rgba(1, 1, 1, 0.14)
                    strokeColor: Qt.rgba(1, 1, 1, 0.40)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.18)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.03)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        Label {
                            text: "扩展预留区"
                            color: "#253549"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "用于后续功能模块拓展，当前作为信息架构预留。"
                            color: "#64768c"
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }

                        Repeater {
                            model: root.expansionModules
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 78
                                radius: 10
                                color: Qt.rgba(1, 1, 1, 0.28)
                                border.width: 1
                                border.color: Qt.rgba(1, 1, 1, 0.42)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            text: modelData.title
                                            color: "#25364b"
                                            font.pixelSize: 14
                                            font.bold: true
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.desc
                                            color: "#5f7086"
                                            font.pixelSize: 12
                                            wrapMode: Text.Wrap
                                        }
                                    }

                                    Rectangle {
                                        Layout.alignment: Qt.AlignVCenter
                                        radius: 9
                                        color: Qt.rgba(0.49, 0.52, 0.57, 0.18)
                                        border.width: 1
                                        border.color: Qt.rgba(0.48, 0.53, 0.61, 0.40)
                                        width: reserveTag.implicitWidth + 12
                                        height: 22

                                        Label {
                                            id: reserveTag
                                            anchors.centerIn: parent
                                            text: modelData.state
                                            color: "#5e6f84"
                                            font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
