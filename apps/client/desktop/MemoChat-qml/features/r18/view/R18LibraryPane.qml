import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var libraryModel: null
    property var folderModel: null
    property string selectedFolderId: ""
    property string newFolderName: ""
    property bool loading: false
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color itemActiveFillColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
    property color fieldFillColor: Qt.rgba(1, 1, 1, 0.16)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color primaryButtonTextColor: "#203246"
    property color primaryButtonColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
    property color primaryButtonHoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
    property color primaryButtonPressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    property string gatewayBaseUrl: ""

    signal refreshRequested()
    signal folderSelected(string folderId)
    signal createFolderRequested(string name)
    signal deleteFolderRequested(string folderId)
    signal comicRequested(string sourceId, string comicId)
    signal unfavoriteRequested(string sourceId, string comicId)
    signal newFolderNameEdited(string name)

    function folderName(folderId) {
        if (!root.folderModel)
            return folderId
        for (var i = 0; i < root.folderModel.count; ++i) {
            var f = root.folderModel.get(i)
            if (f && (f.id === folderId || f.folder_id === folderId))
                return f.name || f.title || folderId
        }
        return folderId || "全部"
    }

    function itemInFolder(item, folderId) {
        if (!folderId || folderId.length === 0)
            return true
        var data = item && item.data ? item.data : item
        var ids = (data && (data.folder_ids || data.folderIds)) || []
        if (typeof ids === "string")
            return ids.indexOf(folderId) >= 0
        if (!ids || ids.length === undefined)
            return false
        for (var i = 0; i < ids.length; ++i) {
            if (String(ids[i]) === folderId)
                return true
        }
        return false
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    text: "我的收藏"
                    color: root.textPrimaryColor
                    font.pixelSize: 20
                    font.bold: true
                }
                Text {
                    text: "自定义收藏夹归档喜欢的本子"
                    color: root.textMutedColor
                    font.pixelSize: 12
                }
            }

            GlassButton {
                Layout.preferredWidth: 86
                Layout.preferredHeight: 34
                text: "刷新"
                textPixelSize: 13
                textColor: root.textSecondaryColor
                cornerRadius: 17
                normalColor: root.secondaryButtonColor
                hoverColor: root.secondaryButtonHoverColor
                pressedColor: root.secondaryButtonPressedColor
                onClicked: root.refreshRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Folders column
            Rectangle {
                Layout.preferredWidth: 210
                Layout.fillHeight: true
                radius: 12
                color: root.itemFillColor
                border.color: root.fieldStrokeColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 10
                        color: root.selectedFolderId === "" ? root.itemActiveFillColor : "transparent"
                        border.color: root.fieldStrokeColor
                        Text {
                            anchors.centerIn: parent
                            text: "全部收藏"
                            color: root.textPrimaryColor
                            font.pixelSize: 13
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.folderSelected("")
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.folderModel
                        spacing: 6
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 40
                            radius: 10
                            readonly property string folderId: model.sourceId || model.itemId || ""
                            color: folderId === root.selectedFolderId ? root.itemActiveFillColor
                                                                      : (folderMouse.containsMouse ? root.itemHoverFillColor : "transparent")
                            border.color: root.fieldStrokeColor
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 8
                                Text {
                                    Layout.fillWidth: true
                                    text: model.title || folderId || "收藏夹"
                                    color: root.textPrimaryColor
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                }
                                Text {
                                    visible: folderId !== "default" && folderId.length > 0
                                    text: "×"
                                    color: root.textMutedColor
                                    font.pixelSize: 16
                                    MouseArea {
                                        anchors.fill: parent
                                        anchors.margins: -6
                                        onClicked: root.deleteFolderRequested(folderId)
                                    }
                                }
                            }
                            MouseArea {
                                id: folderMouse
                                anchors.fill: parent
                                anchors.rightMargin: 28
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.folderSelected(folderId)
                            }
                        }
                    }

                    TextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: root.newFolderName
                        placeholderText: "新收藏夹，如 全彩"
                        color: root.textPrimaryColor
                        selectByMouse: true
                        background: Rectangle {
                            radius: 10
                            color: root.fieldFillColor
                            border.color: root.fieldStrokeColor
                        }
                        onTextEdited: root.newFolderNameEdited(text)
                        onAccepted: root.createFolderRequested(text)
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: "新建收藏夹"
                        textPixelSize: 12
                        textColor: root.primaryButtonTextColor
                        cornerRadius: 12
                        normalColor: root.primaryButtonColor
                        hoverColor: root.primaryButtonHoverColor
                        pressedColor: root.primaryButtonPressedColor
                        enabled: root.newFolderName.trim().length > 0
                        onClicked: root.createFolderRequested(root.newFolderName)
                    }
                }
            }

            // Items grid
            GridView {
                id: libraryGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cellWidth: Math.max(160, Math.floor(width / Math.max(1, Math.floor(width / 180))))
                cellHeight: 280
                model: root.libraryModel
                ScrollBar.vertical: GlassScrollBar {}

                delegate: Item {
                    width: libraryGrid.cellWidth - 8
                    height: libraryGrid.cellHeight - 8
                    visible: root.itemInFolder(model, root.selectedFolderId)
                    readonly property string rowSourceId: model.sourceId || ""
                    readonly property string rowComicId: model.itemId || ""

                    Rectangle {
                        anchors.fill: parent
                        radius: 12
                        color: root.itemFillColor
                        border.color: root.fieldStrokeColor
                        clip: true
                        opacity: parent.visible ? 1 : 0

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 6

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 180
                                radius: 8
                                color: "#e8edf5"
                                Image {
                                    anchors.fill: parent
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    source: {
                                        var c = model.cover || ""
                                        if (!c) return ""
                                        if (c.indexOf("http") === 0 || c.indexOf("qrc:") === 0) return c
                                        if (c.indexOf("/") === 0 && root.gatewayBaseUrl.length > 0)
                                            return root.gatewayBaseUrl + c
                                        return c
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.comicRequested(rowSourceId, rowComicId)
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: model.title || rowComicId || "未命名"
                                color: root.textPrimaryColor
                                font.pixelSize: 12
                                font.bold: true
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: rowSourceId
                                color: root.textMutedColor
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                text: "取消收藏"
                                textPixelSize: 11
                                textColor: root.textSecondaryColor
                                cornerRadius: 10
                                normalColor: root.secondaryButtonColor
                                hoverColor: root.secondaryButtonHoverColor
                                pressedColor: root.secondaryButtonPressedColor
                                onClicked: root.unfavoriteRequested(rowSourceId, rowComicId)
                            }
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.libraryModel || root.libraryModel.count === 0
            text: root.loading ? "加载收藏中…" : "还没有收藏，打开详情页点收藏吧"
            color: root.textMutedColor
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
