import QtQuick 2.15
import QtQuick.Layouts 1.15

Live2DSectionPanel {
    id: root

    property bool autoStartPetOnClientStart: false
    property bool idleMotionEnabled: true
    property bool gazeFollowEnabled: true
    property bool memoryEnabled: true
    property bool interruptEnabled: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false

    signal autoStartPetOnClientStartEdited(bool checked)
    signal idleMotionEnabledEdited(bool checked)
    signal gazeFollowEnabledEdited(bool checked)
    signal memoryEnabledEdited(bool checked)
    signal interruptEnabledEdited(bool checked)
    signal cameraEnabledEdited(bool checked)
    signal cloudVisionEnabledEdited(bool checked)

    title: "行为与记忆"
    subtitle: "定义待机动作、上下文记忆和隐私权限"

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "打开客户端自启"
            subtitle: "默认关闭；保存后下次打开客户端自动启动桌宠"
            checked: root.autoStartPetOnClientStart
            onCheckedChanged: root.autoStartPetOnClientStartEdited(checked)
        }

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "待机动作"
            subtitle: "无对话时循环 idle motion"
            checked: root.idleMotionEnabled
            onCheckedChanged: root.idleMotionEnabledEdited(checked)
        }

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "视线跟随"
            subtitle: "根据鼠标位置更新 gaze"
            checked: root.gazeFollowEnabled
            onCheckedChanged: root.gazeFollowEnabledEdited(checked)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "角色记忆"
            subtitle: "允许保存偏好、称呼和长期约束"
            checked: root.memoryEnabled
            onCheckedChanged: root.memoryEnabledEdited(checked)
        }

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "打断响应"
            subtitle: "用户输入时停止当前语音并切换动作"
            checked: root.interruptEnabled
            onCheckedChanged: root.interruptEnabledEdited(checked)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "摄像头权限"
            subtitle: "默认关闭，后续需用户主动开启"
            checked: root.cameraEnabled
            onCheckedChanged: root.cameraEnabledEdited(checked)
        }

        Live2DToggleRow {
            Layout.fillWidth: true
            title: "云端多模态"
            subtitle: "默认仅发送文本，图像另行确认"
            checked: root.cloudVisionEnabled
            onCheckedChanged: root.cloudVisionEnabledEdited(checked)
        }
    }
}
