import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root
    property var r18Controller: null
    property var backdrop: null
    property string keyword: ""
    property int viewMode: 0 // 0 search, 1 detail, 2 reader, 3 sources
    readonly property color panelFillColor: Qt.rgba(1.0, 0.72, 0.84, 0.20)
    readonly property color panelStrokeColor: Qt.rgba(1.0, 0.86, 0.94, 0.34)
    readonly property color fieldFillColor: Qt.rgba(1.0, 0.90, 0.95, 0.14)
    readonly property color fieldStrokeColor: Qt.rgba(1.0, 0.93, 0.97, 0.22)
    readonly property color itemFillColor: Qt.rgba(1.0, 0.86, 0.93, 0.13)
    readonly property color itemSelectedFillColor: Qt.rgba(1.0, 0.56, 0.76, 0.24)
    readonly property color primaryButtonColor: Qt.rgba(1.0, 0.48, 0.70, 0.24)
    readonly property color primaryButtonHoverColor: Qt.rgba(1.0, 0.48, 0.70, 0.34)
    readonly property color primaryButtonPressedColor: Qt.rgba(1.0, 0.48, 0.70, 0.42)
    readonly property color secondaryButtonColor: Qt.rgba(1.0, 0.68, 0.82, 0.18)
    readonly property color secondaryButtonHoverColor: Qt.rgba(1.0, 0.68, 0.82, 0.28)
    readonly property color secondaryButtonPressedColor: Qt.rgba(1.0, 0.68, 0.82, 0.36)

    function absoluteUrl(url) {
        if (!url) {
            return ""
        }
        if (url.indexOf("http://") === 0 || url.indexOf("https://") === 0 || url.indexOf("qrc:/") === 0) {
            return url
        }
        return gate_url_prefix + url
    }

    Component.onCompleted: {
        if (root.r18Controller) {
            root.r18Controller.refreshSources()
            root.r18Controller.search("", 1)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 10

        GlassSurface {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            backdrop: root.backdrop
            cornerRadius: 12
            blurEnabled: false
            fillColor: root.panelFillColor
            strokeColor: root.panelStrokeColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    text: "R18"
                    color: "#fff2f7"
                    font.pixelSize: 24
                    font.bold: true
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "搜索漫画"
                    placeholderTextColor: Qt.rgba(255, 242, 247, 0.56)
                    color: "#fff4f8"
                    text: root.keyword
                    selectByMouse: true
                    background: Rectangle {
                        radius: 9
                        color: root.fieldFillColor
                        border.color: root.fieldStrokeColor
                    }
                    onAccepted: {
                        root.keyword = text
                        root.viewMode = 0
                        if (root.r18Controller) root.r18Controller.search(text, 1)
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    text: "搜索"
                    textColor: "#fff4f8"
                    cornerRadius: 9
                    normalColor: root.primaryButtonColor
                    hoverColor: root.primaryButtonHoverColor
                    pressedColor: root.primaryButtonPressedColor
                    onClicked: {
                        root.keyword = searchField.text
                        root.viewMode = 0
                        if (root.r18Controller) root.r18Controller.search(searchField.text, 1)
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    text: "源管理"
                    textColor: "#fff4f8"
                    cornerRadius: 9
                    normalColor: root.secondaryButtonColor
                    hoverColor: root.secondaryButtonHoverColor
                    pressedColor: root.secondaryButtonPressedColor
                    onClicked: {
                        root.viewMode = 3
                        if (root.r18Controller) root.r18Controller.refreshSources()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.22)
                }

                Text {
                    text: "漫画源"
                    color: "#f9d6e4"
                    font.pixelSize: 13
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.r18Controller ? root.r18Controller.sourceModel : null
                    spacing: 6
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 46
                        radius: 8
                        color: model.sourceId === (root.r18Controller ? root.r18Controller.currentSourceId : "") ? root.itemSelectedFillColor : root.itemFillColor
                        border.color: root.fieldStrokeColor

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 2
                            Text {
                                width: parent.width
                                text: model.title
                                color: "#fff3f7"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                            Text {
                                width: parent.width
                                text: model.enabled ? model.status : "disabled"
                                color: "#e6bdcb"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.r18Controller) {
                                    root.r18Controller.openComic(model.sourceId, "mock-1-1")
                                }
                            }
                        }
                    }
                }
            }
        }

        GlassSurface {
            Layout.fillWidth: true
            Layout.fillHeight: true
            backdrop: root.backdrop
            cornerRadius: 12
            blurEnabled: false
            fillColor: root.panelFillColor
            strokeColor: root.panelStrokeColor

            Item {
                anchors.fill: parent
                anchors.margins: 12

                Text {
                    anchors.centerIn: parent
                    visible: root.r18Controller && root.r18Controller.loading
                    text: "加载中"
                    color: "#fff2f7"
                    font.pixelSize: 18
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    visible: root.r18Controller && root.r18Controller.error.length > 0
                    text: root.r18Controller ? root.r18Controller.error : ""
                    color: "#ffcedd"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    z: 5
                }

                GridView {
                    anchors.fill: parent
                    visible: root.viewMode === 0
                    clip: true
                    cellWidth: Math.max(170, Math.floor(width / Math.max(1, Math.floor(width / 190))))
                    cellHeight: 255
                    model: root.r18Controller ? root.r18Controller.comicModel : null
                    delegate: Rectangle {
                        width: GridView.view.cellWidth - 10
                        height: 242
                        radius: 10
                        clip: true
                        color: root.itemFillColor
                        border.color: root.fieldStrokeColor

                        Image {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 176
                            source: root.absoluteUrl(model.cover)
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 10
                            text: model.title
                            color: "#fff4f8"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.viewMode = 1
                                if (root.r18Controller) root.r18Controller.openComic(model.sourceId, model.itemId)
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    visible: root.viewMode === 1
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        GlassButton {
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 32
                            text: "返回"
                            textColor: "#fff4f8"
                            cornerRadius: 8
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: root.viewMode = 0
                        }
                        Text {
                            Layout.fillWidth: true
                            text: root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.title || "" : ""
                            color: "#fff2f7"
                            font.pixelSize: 20
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.r18Controller && root.r18Controller.currentComic ? root.r18Controller.currentComic.description || "" : ""
                        color: "#efd0dc"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.r18Controller ? root.r18Controller.chapterModel : null
                        spacing: 6
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 48
                            radius: 8
                            color: root.itemFillColor
                            border.color: root.fieldStrokeColor
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                text: model.title
                                color: "#fff4f8"
                                font.pixelSize: 14
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.viewMode = 2
                                    if (root.r18Controller) root.r18Controller.openChapter(model.sourceId, model.itemId)
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    visible: root.viewMode === 2
                    spacing: 8
                    GlassButton {
                        Layout.preferredWidth: 92
                        Layout.preferredHeight: 32
                        text: "返回章节"
                        textColor: "#fff4f8"
                        cornerRadius: 8
                        normalColor: root.secondaryButtonColor
                        hoverColor: root.secondaryButtonHoverColor
                        pressedColor: root.secondaryButtonPressedColor
                        onClicked: root.viewMode = 1
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 10
                        model: root.r18Controller ? root.r18Controller.pageModel : null
                        delegate: Image {
                            width: ListView.view.width
                            height: Math.max(420, width * 1.48)
                            source: root.absoluteUrl(model.url)
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    visible: root.viewMode === 3
                    spacing: 10
                    Text {
                        text: "源管理"
                        color: "#fff2f7"
                        font.pixelSize: 22
                        font.bold: true
                    }
                    TextField {
                        id: sourcePackagePath
                        Layout.fillWidth: true
                        placeholderText: "插件 zip 路径"
                        placeholderTextColor: Qt.rgba(255, 242, 247, 0.56)
                        color: "#fff4f8"
                        selectByMouse: true
                        background: Rectangle {
                            radius: 9
                            color: root.fieldFillColor
                            border.color: root.fieldStrokeColor
                        }
                    }
                    TextArea {
                        id: manifestText
                        Layout.fillWidth: true
                        Layout.preferredHeight: 92
                        placeholderText: "可选 manifest.json"
                        placeholderTextColor: Qt.rgba(255, 242, 247, 0.56)
                        color: "#fff4f8"
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        background: Rectangle {
                            radius: 9
                            color: root.fieldFillColor
                            border.color: root.fieldStrokeColor
                        }
                    }
                    RowLayout {
                        GlassButton {
                            Layout.preferredWidth: 76
                            Layout.preferredHeight: 32
                            text: "导入"
                            textColor: "#fff4f8"
                            cornerRadius: 8
                            normalColor: root.primaryButtonColor
                            hoverColor: root.primaryButtonHoverColor
                            pressedColor: root.primaryButtonPressedColor
                            onClicked: {
                                if (root.r18Controller) root.r18Controller.importSourcePackage(sourcePackagePath.text, manifestText.text)
                            }
                        }
                        GlassButton {
                            Layout.preferredWidth: 76
                            Layout.preferredHeight: 32
                            text: "刷新"
                            textColor: "#fff4f8"
                            cornerRadius: 8
                            normalColor: root.secondaryButtonColor
                            hoverColor: root.secondaryButtonHoverColor
                            pressedColor: root.secondaryButtonPressedColor
                            onClicked: {
                                if (root.r18Controller) root.r18Controller.refreshSources()
                            }
                        }
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: root.r18Controller ? root.r18Controller.sourceModel : null
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 54
                            radius: 8
                            color: root.itemFillColor
                            border.color: root.fieldStrokeColor
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                Text {
                                    Layout.fillWidth: true
                                    text: model.title + "  " + model.status
                                    color: "#fff4f8"
                                    elide: Text.ElideRight
                                }
                                Switch {
                                    checked: model.enabled
                                    onToggled: if (root.r18Controller) root.r18Controller.enableSource(model.sourceId, checked)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
