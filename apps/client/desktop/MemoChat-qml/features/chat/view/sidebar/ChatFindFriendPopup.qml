import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Popup {
    id: root
    modal: true
    focus: true
    width: 380
    height: 420
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property Item backdrop: null
    property var searchModel
    property bool searchPending: false
    property string searchStatusText: ""
    property bool searchStatusError: false

    signal searchRequested(string uidText)
    signal searchCleared()
    signal addFriendDialogRequested(int uid, string name, string description, string icon)

    function openFresh() {
        friendSearchInput.text = ""
        root.searchCleared()
        open()
        friendSearchInput.forceActiveFocus()
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(24, 32, 44, 0.28)
    }

    background: GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: 30
        cornerRadius: 12
        fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.90)
        strokeColor: Qt.rgba(1, 1, 1, 0.62)
        glowTopColor: Qt.rgba(1, 1, 1, 0.30)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.06)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: "添加好友"
            color: "#263448"
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            GlassTextField {
                id: friendSearchInput
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                backdrop: root.backdrop
                blurRadius: 30
                cornerRadius: 9
                leftInset: 12
                rightInset: 12
                textPixelSize: 14
                textHorizontalAlignment: TextInput.AlignLeft
                placeholderText: "用户ID（u#########）"
                onTextChanged: root.searchCleared()
                onAccepted: root.searchRequested(friendSearchInput.text)
            }

            GlassButton {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 38
                text: "查找"
                textPixelSize: 13
                enabled: !root.searchPending
                cornerRadius: 9
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.44)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.searchRequested(friendSearchInput.text)
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.searchStatusText
            visible: text.length > 0
            color: root.searchStatusError ? "#cc4a4a" : "#2a7f62"
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: friendSearchResultList
                anchors.fill: parent
                clip: true
                model: root.searchModel
                ScrollBar.vertical: GlassScrollBar { }

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 86
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.30)
                    border.color: Qt.rgba(1, 1, 1, 0.48)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 50
                            Layout.preferredHeight: 50
                            radius: 6
                            clip: true
                            color: Qt.rgba(0.74, 0.82, 0.92, 0.50)

                            Image {
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                property string fallbackSource: "qrc:/res/head_1.jpg"
                                property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                property bool loadFailed: false
                                source: loadFailed ? fallbackSource : baseSource
                                cache: true
                                asynchronous: true
                                opacity: status === Image.Ready ? 1.0 : 0.0
                                Behavior on opacity { NumberAnimation { duration: 200 } }
                                onBaseSourceChanged: loadFailed = false
                                onStatusChanged: if (status === Image.Error) { loadFailed = true }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: name
                                font.bold: true
                                color: "#273449"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "ID: " + (userId && userId.length > 0 ? userId : "未分配")
                                color: "#5f6f85"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: desc
                                color: "#5f6f85"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        GlassButton {
                            text: "申请"
                            implicitWidth: 58
                            implicitHeight: 30
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                            onClicked: {
                                root.close()
                                root.addFriendDialogRequested(uid, name, desc, icon)
                            }
                        }
                    }
                }
            }

            GlassSurface {
                anchors.centerIn: parent
                width: 190
                height: 64
                visible: !root.searchPending && friendSearchResultList.count === 0
                backdrop: root.backdrop
                cornerRadius: 10
                blurRadius: 16
                fillColor: Qt.rgba(1, 1, 1, 0.20)
                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                Label {
                    anchors.centerIn: parent
                    text: friendSearchInput.text.length > 0 ? "未找到匹配用户" : "输入用户ID后查找"
                    color: "#6a7b92"
                    font.pixelSize: 13
                }
            }
        }
    }
}
