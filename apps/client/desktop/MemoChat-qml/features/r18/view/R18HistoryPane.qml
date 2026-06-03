import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var historyModel: null
    property bool loading: false
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)

    signal refreshRequested()
    signal comicRequested(string sourceId, string itemId)

    function modelCount(model) {
        return model && model.count !== undefined ? model.count : 0
    }

    function absoluteUrl(url) {
        if (!url) {
            return ""
        }
        if (url.indexOf("http://") === 0 || url.indexOf("https://") === 0 || url.indexOf("qrc:/") === 0) {
            return url
        }
        if (url.indexOf("/") === 0 && typeof gateMediaUrlPrefix === "string" && gateMediaUrlPrefix.length > 0) {
            return gateMediaUrlPrefix + url
        }
        if (url.indexOf("/") === 0 && typeof gateUrlPrefix === "string" && gateUrlPrefix.length > 0) {
            return gateUrlPrefix + url
        }
        return url
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: "阅读历史"
                color: root.textPrimaryColor
                font.pixelSize: 24
                font.bold: true
            }

            GlassButton {
                Layout.preferredWidth: 76
                Layout.preferredHeight: 32
                text: "刷新"
                textColor: root.textSecondaryColor
                cornerRadius: 8
                normalColor: root.secondaryButtonColor
                hoverColor: root.secondaryButtonHoverColor
                pressedColor: root.secondaryButtonPressedColor
                onClicked: root.refreshRequested()
            }
        }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            cacheBuffer: 480
            spacing: 8
            model: root.historyModel
            ScrollBar.vertical: GlassScrollBar {}

            delegate: Rectangle {
                width: ListView.view.width
                height: 72
                radius: 10
                color: historyMouse.containsMouse ? root.itemHoverFillColor : root.itemFillColor
                border.color: root.fieldStrokeColor

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    Image {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 56
                        source: root.absoluteUrl(model.cover)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: model.title || model.itemId || "阅读记录"
                            color: root.textPrimaryColor
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: model.subtitle || model.sourceId
                            color: root.textMutedColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }

                MouseArea {
                    id: historyMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.comicRequested(model.sourceId, model.itemId)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.modelCount(root.historyModel) === 0 && !root.loading
            text: "暂无阅读历史"
            color: root.textMutedColor
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
