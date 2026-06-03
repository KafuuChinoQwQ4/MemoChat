pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var officialSourceModel: null
    property string sourceCatalogInput: ""
    property bool sourceHelpVisible: false
    property bool loading: false
    property color homeCardFillColor: "#ffffff"
    property color homeCardStrokeColor: "#d8dde6"
    property color homeFieldStrokeColor: "#d7dce5"
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color placeholderColor: "#8493a3"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    property color importButtonColor: "#0c4f92"
    property color importButtonHoverColor: "#0f61b0"
    property color importButtonPressedColor: "#093d72"

    signal sourceCatalogInputEdited(string text)
    signal officialCatalogRefreshRequested()
    signal sourceHelpToggled()
    signal officialSourceImportRequested(int sourceIndex)

    function modelCount(model) {
        return model && model.count !== undefined ? model.count : 0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.sourceHelpVisible ? 232 : 184
            radius: 8
            color: root.homeCardFillColor
            border.color: root.homeCardStrokeColor
            antialiasing: true

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 16
                anchors.bottomMargin: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Image {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        source: "qrc:/icons/file.png"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.82
                        mipmap: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "仓库地址"
                        color: root.textPrimaryColor
                        font.pixelSize: 18
                        elide: Text.ElideRight
                    }
                }

                TextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    text: root.sourceCatalogInput
                    placeholderText: "https://.../index.json 或 D:\\Venera"
                    placeholderTextColor: root.placeholderColor
                    color: root.textPrimaryColor
                    selectByMouse: true
                    leftPadding: 14
                    rightPadding: 14
                    font.pixelSize: 15
                    background: Rectangle {
                        color: "transparent"
                        border.color: "transparent"
                    }
                    onTextEdited: root.sourceCatalogInputEdited(text)
                    onAccepted: root.officialCatalogRefreshRequested()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: root.homeFieldStrokeColor
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.sourceHelpVisible
                    text: "该地址应指向 index.json；也可以选择本地目录，例如 D:\\Venera，刷新后会读取目录中的 index.json。"
                    color: root.textSecondaryColor
                    font.pixelSize: 13
                    lineHeight: 1.2
                    wrapMode: Text.WordWrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.sourceHelpVisible
                    text: "不会默认填入或加载任何仓库地址；需要由用户手动输入或选择目录。"
                    color: root.textMutedColor
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Item { Layout.fillWidth: true }

                    GlassButton {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 38
                        text: "帮助"
                        textPixelSize: 14
                        textColor: "#0c4f92"
                        cornerRadius: 19
                        normalColor: "transparent"
                        hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.18)
                        pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.26)
                        onClicked: root.sourceHelpToggled()
                    }

                    GlassButton {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 38
                        text: "刷新"
                        textPixelSize: 14
                        textColor: root.textSecondaryColor
                        cornerRadius: 19
                        normalColor: root.secondaryButtonColor
                        hoverColor: root.secondaryButtonHoverColor
                        pressedColor: root.secondaryButtonPressedColor
                        onClicked: root.officialCatalogRefreshRequested()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.modelCount(root.officialSourceModel) === 0 && !root.loading
            text: "暂无漫画源，刷新仓库地址后会出现在这里"
            color: root.textMutedColor
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: root.officialSourceModel
            ScrollBar.vertical: GlassScrollBar {}

            delegate: R18OfficialSourceRow {
                required property int index
                required property string title
                required property string itemId
                required property var status
                required property var message
                required property var url

                sourceIndex: index
                statusText: status || message || url || ""
                textPrimaryColor: root.textPrimaryColor
                textSecondaryColor: root.textSecondaryColor
                importButtonColor: root.importButtonColor
                importButtonHoverColor: root.importButtonHoverColor
                importButtonPressedColor: root.importButtonPressedColor
                onImportRequested: function(sourceIndex) {
                    root.officialSourceImportRequested(sourceIndex)
                }
            }
        }
    }
}
