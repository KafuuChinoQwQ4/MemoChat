import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property var sessions: []
    property string currentSessionId: ""
    property string currentModel: ""
    property var availableModels: []
    property bool modelRefreshBusy: false
    property bool apiProviderBusy: false
    property string apiProviderStatus: ""
    property bool thinkingEnabled: root.agentController ? root.agentController.thinkingEnabled : false
    property bool currentModelSupportsThinking: root.agentController ? root.agentController.currentModelSupportsThinking : false
    property var knowledgeBases: root.agentController ? root.agentController.knowledgeBases : []
    property string knowledgeSearchResult: root.agentController ? root.agentController.knowledgeSearchResult : ""
    property bool knowledgeBusy: false
    property string knowledgeStatusText: ""
    property string knowledgeError: ""
    property string currentTraceId: root.agentController ? root.agentController.currentTraceId : ""
    property string currentSkill: root.agentController ? root.agentController.currentSkill : ""
    property string currentFeedbackSummary: root.agentController ? root.agentController.currentFeedbackSummary : ""
    property var traceEvents: root.agentController ? root.agentController.traceEvents : []
    property var traceObservations: root.agentController ? root.agentController.traceObservations : []
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    readonly property int headerActionWidth: 64
    readonly property int headerActionHeight: 26
    readonly property int headerActionTextSize: 12
    readonly property int headerActionRadius: 7

    signal backRequested()

    function currentSessionTitle() {
        if (!sessions || currentSessionId.length === 0) {
            return "未选择会话"
        }
        for (var i = 0; i < sessions.length; ++i) {
            var session = sessions[i]
            if (session.session_id === currentSessionId) {
                return session.title && session.title.length > 0 ? session.title : "新会话"
            }
        }
        return "当前会话"
    }

    function sessionSummary() {
        if (currentSessionId.length === 0) {
            return "从左侧选择或新建会话开始。"
        }
        if (streaming) {
            return "AI 正在生成回复。"
        }
        if (loading) {
            return "AI 正在处理你的问题。"
        }
        return "当前会话已就绪，可以继续追问。"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 78
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.22)
            border.color: Qt.rgba(1, 1, 1, 0.46)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 8

                        Label {
                            text: "AI 助手"
                            color: "#2a3649"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Rectangle {
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: sessionTitleLabel.implicitWidth + 18
                            Layout.maximumWidth: 110
                            radius: 12
                            color: Qt.rgba(1, 1, 1, 0.32)
                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.36)

                            Label {
                                id: sessionTitleLabel
                                anchors.centerIn: parent
                                width: parent.width - 14
                                text: root.currentSessionTitle()
                                color: "#4e5d74"
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: modelChipLabel.implicitWidth + 16
                            Layout.maximumWidth: 132
                            radius: 12
                            color: Qt.rgba(0.36, 0.62, 0.92, 0.16)
                            visible: root.currentModel.length > 0

                            Label {
                                id: modelChipLabel
                                anchors.centerIn: parent
                                width: parent.width - 12
                                text: root.currentModel
                                color: "#2d6fb4"
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.sessionSummary()
                        color: "#6a7b92"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                    }
                }

                GridLayout {
                    id: headerActionsGrid
                    Layout.preferredWidth: root.headerActionWidth * 2 + headerActionsGrid.columnSpacing
                    Layout.preferredHeight: root.headerActionHeight * 2 + headerActionsGrid.rowSpacing
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    columns: 2
                    rowSpacing: 5
                    columnSpacing: 6

                    GlassButton {
                        Layout.preferredWidth: root.headerActionWidth
                        Layout.preferredHeight: root.headerActionHeight
                        text: root.streaming ? "停止" : "新会话"
                        textPixelSize: root.headerActionTextSize
                        cornerRadius: root.headerActionRadius
                        enableScaleFeedback: false
                        normalColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.22) : Qt.rgba(0.35, 0.61, 0.90, 0.22)
                        hoverColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.32) : Qt.rgba(0.35, 0.61, 0.90, 0.32)
                        pressedColor: root.streaming ? Qt.rgba(0.89, 0.27, 0.27, 0.42) : Qt.rgba(0.35, 0.61, 0.90, 0.40)
                        onClicked: {
                            if (!root.agentController) {
                                return
                            }
                            if (root.streaming) {
                                root.agentController.cancelStream()
                            } else {
                                root.agentController.createSession()
                            }
                        }
                    }

                    GlassButton {
                        Layout.preferredWidth: root.headerActionWidth
                        Layout.preferredHeight: root.headerActionHeight
                        text: "模型"
                        textPixelSize: root.headerActionTextSize
                        cornerRadius: root.headerActionRadius
                        enableScaleFeedback: false
                        normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                        hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                        pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                        onClicked: {
                            const nextActive = !modelSettingsLoader.active
                            modelSettingsLoader.active = nextActive
                            if (nextActive && root.agentController) {
                                root.agentController.refreshModelList()
                            }
                        }
                    }

                    GlassButton {
                        Layout.preferredWidth: root.headerActionWidth
                        Layout.preferredHeight: root.headerActionHeight
                        text: "知识库"
                        textPixelSize: root.headerActionTextSize
                        cornerRadius: root.headerActionRadius
                        enableScaleFeedback: false
                        normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                        hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                        pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                        onClicked: {
                            if (root.agentController) {
                                root.agentController.listKnowledgeBases()
                            }
                            knowledgeBaseLoader.active = true
                        }
                    }

                    GlassButton {
                        Layout.preferredWidth: root.headerActionWidth
                        Layout.preferredHeight: root.headerActionHeight
                        text: "轨迹"
                        textPixelSize: root.headerActionTextSize
                        cornerRadius: root.headerActionRadius
                        enableScaleFeedback: false
                        normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                        hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                        pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                        onClicked: traceLoader.active = true
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.42)

            AgentConversationPane {
                anchors.fill: parent
                anchors.margins: 12
                agentController: root.agentController
                messageModel: root.messageModel
                loading: root.loading
                streaming: root.streaming
                errorMsg: root.errorMsg
                sessions: root.sessions
                currentSessionId: root.currentSessionId
                selfAvatar: root.selfAvatar
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 148
            radius: 12
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.44)

            AgentComposerBar {
                anchors.fill: parent
                enabledComposer: !root.loading && !root.streaming
                onSendComposer: function(text) {
                    if (root.agentController) {
                        root.agentController.sendStreamMessage(text)
                    }
                }
            }
        }
    }

    Loader {
        id: modelSettingsLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(20, 28, 40, 0.22)

                MouseArea {
                    anchors.fill: parent
                    onClicked: modelSettingsLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(460, Math.max(0, root.width - 32))
                    height: Math.min(560, Math.max(0, root.height - 32))
                    radius: 14
                    clip: true
                    color: Qt.rgba(0.96, 0.98, 1.0, 0.84)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Label {
                            text: "选择模型"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#2a3649"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.currentModel.length > 0 ? ("当前使用: " + root.currentModel) : "当前模型尚未加载"
                            color: "#6a7b92"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: root.currentModelSupportsThinking
                            spacing: 10

                            Label {
                                Layout.fillWidth: true
                                text: "Think"
                                color: "#2a3649"
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Switch {
                                id: thinkingSwitch
                                checked: root.thinkingEnabled
                                text: checked ? "开" : "关"
                                onToggled: {
                                    if (root.agentController) {
                                        root.agentController.thinkingEnabled = checked
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: apiFormColumn.implicitHeight + 18
                            radius: 10
                            color: Qt.rgba(1, 1, 1, 0.22)
                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.30)

                            ColumnLayout {
                                id: apiFormColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 9
                                spacing: 7

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        Layout.fillWidth: true
                                        text: "API 接入"
                                        color: "#2a3649"
                                        font.pixelSize: 13
                                        font.bold: true
                                    }

                                    Label {
                                        text: root.apiProviderBusy ? "解析中" : ""
                                        color: "#6a7b92"
                                        font.pixelSize: 11
                                        visible: root.apiProviderBusy
                                    }
                                }

                                TextField {
                                    id: apiProviderNameField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    placeholderText: "名称，例如 gpt"
                                    text: "gpt"
                                    font.pixelSize: 12
                                    selectByMouse: true
                                }

                                TextField {
                                    id: apiBaseUrlField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    placeholderText: "API 地址，例如 https://api.openai.com/v1"
                                    text: "https://api.openai.com/v1"
                                    font.pixelSize: 12
                                    selectByMouse: true
                                }

                                TextField {
                                    id: apiKeyField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    placeholderText: "API Key"
                                    echoMode: TextInput.Password
                                    font.pixelSize: 12
                                    selectByMouse: true
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.apiProviderStatus
                                        color: root.apiProviderStatus.indexOf("已接入") >= 0 ? "#4d7f5c" : "#6a7b92"
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }

                                    GlassButton {
                                        Layout.preferredWidth: 90
                                        Layout.preferredHeight: 30
                                        text: root.apiProviderBusy ? "解析中" : "接入"
                                        textPixelSize: 12
                                        cornerRadius: 8
                                        enabled: !root.apiProviderBusy
                                        normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                        hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                        pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                        onClicked: {
                                            if (root.agentController) {
                                                root.agentController.registerApiProvider(
                                                    apiProviderNameField.text,
                                                    apiBaseUrlField.text,
                                                    apiKeyField.text)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.availableModels
                            spacing: 6

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 48
                                radius: 8
                                color: {
                                    const fullName = (modelData.model_type || "") + ":" + (modelData.model_name || "")
                                    if (fullName === root.currentModel) {
                                        return Qt.rgba(0.54, 0.70, 0.93, 0.18)
                                    }
                                    return modelMouseArea.containsMouse ? Qt.rgba(0.54, 0.70, 0.93, 0.10) : "transparent"
                                }
                                border.color: Qt.rgba(1, 1, 1, 0.32)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.display_name || modelData.model_name || ""
                                            color: "#2a3649"
                                            font.pixelSize: 14
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: (modelData.model_type || "") + (modelData.supports_thinking ? " · Think" : "")
                                            color: "#6a7b92"
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                MouseArea {
                                    id: modelMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.agentController && modelData.model_type && modelData.model_name) {
                                            root.agentController.switchModel(modelData.model_type, modelData.model_name)
                                        }
                                        modelSettingsLoader.active = false
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 72
                            radius: 10
                            color: Qt.rgba(1, 1, 1, 0.24)
                            border.color: Qt.rgba(1, 1, 1, 0.30)
                            visible: root.availableModels.length === 0

                            Label {
                                anchors.centerIn: parent
                                width: parent.width - 24
                                text: root.modelRefreshBusy ? "正在刷新模型列表..." : "当前没有可用模型。请刷新模型列表，或检查 AIServer / AIOrchestrator 配置。"
                                color: "#6a7b92"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Item { Layout.fillWidth: true }

                            GlassButton {
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 32
                                text: "刷新模型"
                                textPixelSize: 13
                                cornerRadius: 8
                                enabled: !root.modelRefreshBusy
                                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                onClicked: {
                                    if (root.agentController) {
                                        root.agentController.refreshModelList()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: knowledgeBaseLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(20, 28, 40, 0.22)

                MouseArea {
                    anchors.fill: parent
                    onClicked: knowledgeBaseLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(560, Math.max(0, root.width - 32))
                    height: Math.min(640, Math.max(0, root.height - 32))
                    radius: 14
                    clip: true
                    color: Qt.rgba(0.96, 0.98, 1.0, 0.84)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: "知识库"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#2a3649"
                            }

                            GlassButton {
                                Layout.preferredWidth: 72
                                Layout.preferredHeight: 30
                                text: "关闭"
                                textPixelSize: 12
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                                onClicked: knowledgeBaseLoader.active = false
                            }
                        }

                        KnowledgeBasePane {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            agentController: root.agentController
                            kbList: root.knowledgeBases
                            searchResult: root.knowledgeSearchResult
                            busy: root.knowledgeBusy
                            statusText: root.knowledgeStatusText
                            errorText: root.knowledgeError
                            onReloadRequested: {
                                if (root.agentController) {
                                    root.agentController.listKnowledgeBases()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: traceLoader
        anchors.fill: parent
        active: false
        sourceComponent: Component {
            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(20, 28, 40, 0.22)

                MouseArea {
                    anchors.fill: parent
                    onClicked: traceLoader.active = false
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(720, Math.max(0, root.width - 32))
                    height: Math.min(660, Math.max(0, root.height - 32))
                    radius: 14
                    clip: true
                    color: Qt.rgba(0.96, 0.98, 1.0, 0.84)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: "AI Agent 轨迹"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#2a3649"
                            }

                            GlassButton {
                                Layout.preferredWidth: 72
                                Layout.preferredHeight: 30
                                text: "关闭"
                                textPixelSize: 12
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                                onClicked: traceLoader.active = false
                            }
                        }

                        AgentTracePane {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            traceId: root.currentTraceId
                            skill: root.currentSkill
                            feedbackSummary: root.currentFeedbackSummary
                            observations: root.traceObservations
                            events: root.traceEvents
                        }
                    }
                }
            }
        }
    }
}
