import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    radius: 12
    color: Qt.rgba(1, 1, 1, 0.22)
    border.color: Qt.rgba(1, 1, 1, 0.46)

    property bool gameRoomActive: false
    property bool multiAiRoomActive: false
    property bool multiAiDetailsExpanded: false
    property string sessionTitle: ""
    property string sessionSummary: ""
    property string currentModel: ""
    property color glassPanelColor: Qt.rgba(0.96, 0.98, 1.0, 0.62)
    property color glassPanelHoverColor: Qt.rgba(1, 1, 1, 0.76)
    property color glassPanelPressedColor: Qt.rgba(0.86, 0.92, 1.0, 0.70)
    property color glassBorderColor: Qt.rgba(1, 1, 1, 0.58)

    signal detailsToggled()
    signal modeMenuRequested()
    signal moreMenuRequested()

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
                    text: root.gameRoomActive ? (root.multiAiRoomActive ? "多 AI 聊天" : "Game 模式") : "AI 助手"
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
                        text: root.sessionTitle
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
                    visible: root.currentModel.length > 0 && !root.gameRoomActive

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
                text: root.sessionSummary
                color: "#6a7b92"
                font.pixelSize: 12
                wrapMode: Text.Wrap
                maximumLineCount: 2
            }
        }

        RowLayout {
            Layout.preferredWidth: root.multiAiRoomActive ? 134 : 94
            Layout.preferredHeight: 32
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: 10
                visible: root.multiAiRoomActive
                color: multiAiDetailsMouseArea.pressed ? root.glassPanelPressedColor
                      : multiAiDetailsMouseArea.containsMouse ? root.glassPanelHoverColor
                                                            : root.glassPanelColor
                border.width: 1
                border.color: root.glassBorderColor

                Image {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    source: "qrc:/icons/dropdown.png"
                    fillMode: Image.PreserveAspectFit
                    rotation: root.multiAiDetailsExpanded ? 180 : 0
                    opacity: 0.78
                    smooth: true
                }

                MouseArea {
                    id: multiAiDetailsMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.detailsToggled()
                }
            }

            Rectangle {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 32
                radius: 10
                color: modeMouseArea.pressed ? root.glassPanelPressedColor
                      : modeMouseArea.containsMouse ? root.glassPanelHoverColor
                                                    : root.glassPanelColor
                border.width: 1
                border.color: root.glassBorderColor

                Text {
                    anchors.centerIn: parent
                    text: "模式"
                    color: "#4e5d74"
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: modeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.modeMenuRequested()
                }
            }

            Rectangle {
                Layout.preferredWidth: 38
                Layout.preferredHeight: 32
                radius: 10
                color: moreMouseArea.pressed ? root.glassPanelPressedColor
                      : moreMouseArea.containsMouse ? root.glassPanelHoverColor
                                                    : root.glassPanelColor
                border.width: 1
                border.color: root.glassBorderColor

                Text {
                    anchors.centerIn: parent
                    text: "..."
                    color: "#4e5d74"
                    font.pixelSize: 18
                    font.bold: true
                }

                MouseArea {
                    id: moreMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.moreMenuRequested()
                }
            }
        }
    }
}
