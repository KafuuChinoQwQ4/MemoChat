pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root

    property var agentData: ({})
    property int agentIndex: -1
    property var availableModels: []

    signal fieldChanged(int agentIndex, string key, var value)
    signal removeRequested(int agentIndex)

    function modelLabel(model) {
        if (!model) {
            return "ollama:qwen2.5:7b"
        }
        return (model.model_type || "") + ":" + (model.model_name || "")
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 262
    radius: 8
    color: Qt.rgba(1, 1, 1, 0.38)
    border.color: Qt.rgba(1, 1, 1, 0.38)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 7

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                text: root.agentData.display_name || ""
                placeholderText: "Agent 名称"
                selectByMouse: true
                onEditingFinished: root.fieldChanged(root.agentIndex, "display_name", text)
            }

            GlassButton {
                Layout.preferredWidth: 54
                Layout.preferredHeight: 30
                text: "移除"
                textPixelSize: 11
                cornerRadius: 7
                normalColor: Qt.rgba(0.80, 0.34, 0.34, 0.16)
                hoverColor: Qt.rgba(0.80, 0.34, 0.34, 0.26)
                onClicked: root.removeRequested(root.agentIndex)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.preferredWidth: 92
                Layout.preferredHeight: 30
                text: root.agentData.role_key || ""
                placeholderText: "role"
                selectByMouse: true
                onEditingFinished: root.fieldChanged(root.agentIndex, "role_key", text)
            }

            ComboBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                textRole: "label"
                valueRole: "value"
                model: {
                    var rows = []
                    for (var i = 0; i < root.availableModels.length; ++i) {
                        rows.push({ "label": root.modelLabel(root.availableModels[i]), "value": i })
                    }
                    if (rows.length === 0) rows.push({ "label": root.modelLabel(null), "value": 0 })
                    return rows
                }
                currentIndex: {
                    for (var i = 0; i < root.availableModels.length; ++i) {
                        var m = root.availableModels[i]
                        if (m.model_type === root.agentData.model_type && m.model_name === root.agentData.model_name) return i
                    }
                    return 0
                }
                onActivated: {
                    var chosen = root.availableModels[currentIndex] || { "model_type": "ollama", "model_name": "qwen2.5:7b" }
                    root.fieldChanged(root.agentIndex, "model_type", chosen.model_type || "ollama")
                    root.fieldChanged(root.agentIndex, "model_name", chosen.model_name || "qwen2.5:7b")
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                text: root.agentData.skill_name || "writer"
                placeholderText: "skill"
                selectByMouse: true
                onEditingFinished: root.fieldChanged(root.agentIndex, "skill_name", text)
            }

            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                text: root.agentData.strategy || "react_reflection"
                placeholderText: "strategy"
                selectByMouse: true
                onEditingFinished: root.fieldChanged(root.agentIndex, "strategy", text)
            }
        }

        TextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            text: root.agentData.environment || ""
            placeholderText: "环境"
            wrapMode: TextArea.Wrap
            selectByMouse: true
            onActiveFocusChanged: if (!activeFocus) root.fieldChanged(root.agentIndex, "environment", text)
        }

        TextArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.agentData.persona || ""
            placeholderText: "角色模板 / 行为约束"
            wrapMode: TextArea.Wrap
            selectByMouse: true
            onActiveFocusChanged: if (!activeFocus) root.fieldChanged(root.agentIndex, "persona", text)
        }
    }
}
