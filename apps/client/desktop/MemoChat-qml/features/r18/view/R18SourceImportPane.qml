import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

ColumnLayout {
    id: root

    property string sourceCatalogInput: ""
    property bool sourceHelpVisible: false
    property color homeFieldStrokeColor: "#d7dce5"
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal sourceCatalogInputEdited(string text)
    signal officialCatalogRequested()
    signal sourceCatalogPathRequested()
    signal sourceHelpToggled()
    signal officialCatalogRefreshRequested()

    spacing: 18

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 22

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Image {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                source: "qrc:/icons/r18_datasource.png"
                fillMode: Image.PreserveAspectFit
                opacity: 0.88
                mipmap: true
            }

            Text {
                Layout.fillWidth: true
                text: "漫画源目录"
                color: root.textPrimaryColor
                font.pixelSize: 18
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                Layout.fillWidth: true
                text: "URL"
                color: root.textPrimaryColor
                font.pixelSize: 15
                elide: Text.ElideRight
            }

            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                text: root.sourceCatalogInput
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
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        GlassButton {
            Layout.preferredWidth: 168
            Layout.preferredHeight: 40
            text: "漫画源列表"
            textPixelSize: 14
            textColor: root.textSecondaryColor
            cornerRadius: 20
            normalColor: root.secondaryButtonColor
            hoverColor: root.secondaryButtonHoverColor
            pressedColor: root.secondaryButtonPressedColor
            onClicked: root.officialCatalogRequested()
        }

        GlassButton {
            Layout.preferredWidth: 174
            Layout.preferredHeight: 40
            text: "使用配置文件"
            textPixelSize: 14
            textColor: root.textSecondaryColor
            cornerRadius: 20
            normalColor: root.secondaryButtonColor
            hoverColor: root.secondaryButtonHoverColor
            pressedColor: root.secondaryButtonPressedColor
            onClicked: root.sourceCatalogPathRequested()
        }

        GlassButton {
            Layout.preferredWidth: 118
            Layout.preferredHeight: 40
            text: "帮助"
            textPixelSize: 14
            textColor: root.textSecondaryColor
            cornerRadius: 20
            normalColor: root.secondaryButtonColor
            hoverColor: root.secondaryButtonHoverColor
            pressedColor: root.secondaryButtonPressedColor
            onClicked: root.sourceHelpToggled()
        }

        GlassButton {
            Layout.preferredWidth: 150
            Layout.preferredHeight: 40
            text: "检查更新"
            textPixelSize: 14
            textColor: root.textSecondaryColor
            cornerRadius: 20
            normalColor: root.secondaryButtonColor
            hoverColor: root.secondaryButtonHoverColor
            pressedColor: root.secondaryButtonPressedColor
            onClicked: root.officialCatalogRefreshRequested()
        }

        Item { Layout.fillWidth: true }
    }

    Text {
        Layout.fillWidth: true
        visible: root.sourceHelpVisible
        text: "该地址应指向 index.json 格式的漫画源目录文件，刷新后会读取其中的源列表。"
        color: root.textMutedColor
        font.pixelSize: 13
        wrapMode: Text.WordWrap
    }
}
