import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Live2DSectionPanel {
    id: root

    property string promptText: ""
    property color textPrimaryColor: "#253247"

    title: "角色提示词预览"
    subtitle: "把当前界面内容整理成之后可发给 agent 的角色配置"

    TextArea {
        Layout.fillWidth: true
        Layout.preferredHeight: 186
        text: root.promptText
        readOnly: true
        selectByMouse: true
        wrapMode: TextArea.Wrap
        color: root.textPrimaryColor
        font.pixelSize: 13
        background: Rectangle {
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.38)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.46)
        }
    }
}
