import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root
    property Item backdrop: null

    readonly property var moreModules: [
        { "title": "账号与安全", "desc": "登录设备、密码策略、异常告警。", "state": "即将上线" },
        { "title": "通知中心", "desc": "提醒样式、免打扰与优先级。", "state": "计划中" },
        { "title": "隐私与权限", "desc": "可见范围、加好友策略、黑名单。", "state": "计划中" },
        { "title": "主题外观", "desc": "配色、字体与界面密度。", "state": "预留" },
        { "title": "会话管理", "desc": "聊天置顶、归档、批量操作。", "state": "预留" },
        { "title": "存储与清理", "desc": "媒体缓存与本地空间治理。", "state": "预留" }
    ]

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Column {
            id: contentColumn
            width: flick.width
            spacing: 12

            GlassSurface {
                width: parent.width
                height: 78
                backdrop: root.backdrop !== null ? root.backdrop : root
                cornerRadius: 12
                blurRadius: 18
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.46)
                glowTopColor: Qt.rgba(1, 1, 1, 0.24)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.04)

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 12
                    spacing: 4

                    Label {
                        text: "更多"
                        color: "#223247"
                        font.pixelSize: 20
                        font.bold: true
                    }

                    Label {
                        text: "未来能力统一挂载区，首版提供结构化占位。"
                        color: "#607289"
                        font.pixelSize: 12
                    }
                }
            }

            GridLayout {
                id: cardGrid
                width: parent.width
                columns: width >= 980 ? 3 : (width >= 650 ? 2 : 1)
                rowSpacing: 12
                columnSpacing: 12

                Repeater {
                    model: root.moreModules
                    delegate: GlassSurface {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                        backdrop: root.backdrop !== null ? root.backdrop : root
                        cornerRadius: 11
                        blurRadius: 18
                        fillColor: Qt.rgba(1, 1, 1, 0.16)
                        strokeColor: Qt.rgba(1, 1, 1, 0.40)
                        glowTopColor: Qt.rgba(1, 1, 1, 0.19)
                        glowBottomColor: Qt.rgba(1, 1, 1, 0.03)

                        Column {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                text: modelData.title
                                color: "#25364b"
                                font.pixelSize: 15
                                font.bold: true
                            }

                            Label {
                                width: parent.width
                                text: modelData.desc
                                color: "#607188"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                            }

                            Item { width: 1; height: 1 }

                            Rectangle {
                                radius: 9
                                color: Qt.rgba(0.48, 0.55, 0.64, 0.18)
                                border.width: 1
                                border.color: Qt.rgba(0.47, 0.53, 0.61, 0.45)
                                width: stateTag.implicitWidth + 14
                                height: 22

                                Label {
                                    id: stateTag
                                    anchors.centerIn: parent
                                    text: modelData.state
                                    color: "#5c6f85"
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
