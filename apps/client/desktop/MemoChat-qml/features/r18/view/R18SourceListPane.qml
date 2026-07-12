pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property var sourceModel: null
    property string currentSourceId: ""
    property bool loading: false
    property color itemFillColor: Qt.rgba(1, 1, 1, 0.14)
    property color itemHoverFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.22)
    property color itemSelectedFillColor: Qt.rgba(0.75, 0.87, 1.0, 0.30)
    property color fieldStrokeColor: Qt.rgba(1, 1, 1, 0.38)
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color textMutedColor: "#7b8794"
    property color secondaryButtonColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
    property color secondaryButtonHoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
    property color secondaryButtonPressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
    property color importButtonColor: "#0c4f92"
    property color importButtonHoverColor: "#0f61b0"
    property color importButtonPressedColor: "#093d72"

    signal importedSourceOpenRequested(string sourceId)

    function modelCount(model) {
        return model && model.count !== undefined ? model.count : 0
    }

    function sourceStatusText(status, format) {
        var statusText = status || "ok"
        var formatText = format || "native"
        if (statusText === "staged-js") {
            return formatText + " · 已导入，等待 MemoChat JS 源运行适配"
        }
        if (statusText === "staged") {
            return formatText + " · 已暂存"
        }
        return formatText + " · " + statusText
    }

    function sourceIsReserved(sourceId, data) {
        var id = sourceId || ""
        if (id.length === 0 && data) {
            id = data.source_id || data.id || data.key || ""
        }
        if (data && data.builtin === true) {
            return true
        }
        return id === "mock" || id === "jm.official" || id === "picacg.official"
    }

    function sourceIdFromRow(sourceId, itemId, data) {
        if (sourceId && sourceId.length > 0) {
            return sourceId
        }
        if (data) {
            if (data.source_id && data.source_id.length > 0) {
                return data.source_id
            }
            if (data.id && data.id.length > 0) {
                return data.id
            }
            if (data.key && data.key.length > 0) {
                return data.key
            }
        }
        return itemId || ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            Layout.fillWidth: true
            text: "可用漫画源"
            color: root.textPrimaryColor
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: root.modelCount(root.sourceModel) === 0 && !root.loading
            text: "当前没有可用漫画源"
            color: root.textMutedColor
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: root.sourceModel
            ScrollBar.vertical: GlassScrollBar {}

            delegate: R18ImportedSourceRow {
                required property var data
                required property string sourceId
                required property string title
                required property string itemId
                required property string url
                required property var status
                required property var format
                required property var message
                required property bool builtin

                readonly property string resolvedSourceId: root.sourceIdFromRow(sourceId, itemId, data)
                readonly property bool reservedSource: builtin || root.sourceIsReserved(resolvedSourceId, data)
                sourceId: resolvedSourceId
                statusText: root.sourceStatusText(status, format)
                sourceUrl: url || message || ""
                builtinSource: reservedSource
                selected: root.currentSourceId === resolvedSourceId
                itemFillColor: root.itemFillColor
                itemHoverFillColor: root.itemHoverFillColor
                itemSelectedFillColor: root.itemSelectedFillColor
                fieldStrokeColor: root.fieldStrokeColor
                textPrimaryColor: root.textPrimaryColor
                textSecondaryColor: root.textSecondaryColor
                textMutedColor: root.textMutedColor
                secondaryButtonColor: root.secondaryButtonColor
                secondaryButtonHoverColor: root.secondaryButtonHoverColor
                secondaryButtonPressedColor: root.secondaryButtonPressedColor
                importButtonColor: root.importButtonColor
                importButtonHoverColor: root.importButtonHoverColor
                importButtonPressedColor: root.importButtonPressedColor
                onOpenRequested: function(sourceId) {
                    root.importedSourceOpenRequested(sourceId)
                }
            }
        }
    }
}
