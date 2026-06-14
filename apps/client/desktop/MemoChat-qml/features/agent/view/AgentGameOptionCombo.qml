pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import "../runtime/AgentGameRuntime.js" as AgentGameRuntime

ComboBox {
    id: root

    property var optionModel: []
    property var selectedValue: ""
    property var fallbackValue: ""
    property var customValues: []
    property int placeholderWidth: 260
    property bool showHints: true

    signal optionActivated(var option, int index)

    function options() {
        return root.optionModel || []
    }

    function optionAt(index) {
        var rows = root.options()
        if (index < 0 || index >= rows.length) {
            return ({})
        }
        return rows[index] || ({})
    }

    function optionText(index) {
        return AgentGameRuntime.optionText(
                    root.options(), index, root.customValues || [])
    }

    textRole: "label"
    valueRole: "value"
    model: root.optionModel || []
    currentIndex: AgentGameRuntime.optionIndex(
                      root.options(), root.selectedValue || "",
                      root.fallbackValue || "")
    displayText: root.optionText(currentIndex)

    onActivated: function(index) {
        root.optionActivated(root.optionAt(index), index)
    }

    delegate: ItemDelegate {
        id: optionDelegate
        width: ListView.view ? ListView.view.width : root.placeholderWidth
        height: root.showHints ? (root.placeholderWidth >= 280 ? 46 : 42) : 30
        highlighted: root.highlightedIndex === index

        required property int index
        required property var modelData

        contentItem: Column {
            spacing: 1
            Text {
                width: parent.width
                text: root.optionText(optionDelegate.index)
                color: "#243145"
                font.pixelSize: 12
                font.bold: true
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: optionDelegate.modelData.hint || ""
                visible: root.showHints && text.length > 0
                color: "#6c7a8e"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            color: optionDelegate.highlighted ? Qt.rgba(0.35, 0.61, 0.90, 0.16) : "transparent"
            radius: 6
        }
    }
    background: Rectangle {
        radius: 7
        color: root.pressed ? Qt.rgba(0.88, 0.93, 1.0, 0.72)
                            : root.hovered ? Qt.rgba(1, 1, 1, 0.62)
                                           : Qt.rgba(1, 1, 1, 0.46)
        border.color: root.hovered || root.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.42) : Qt.rgba(1, 1, 1, 0.38)
    }
    contentItem: Text {
        leftPadding: 10
        rightPadding: 28
        text: root.displayText
        color: "#243145"
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Image {
        x: root.width - width - 9
        y: (root.height - height) / 2
        width: 12
        height: 12
        source: "qrc:/icons/dropdown.png"
        fillMode: Image.PreserveAspectFit
        rotation: root.popup.visible ? 180 : 0
        opacity: root.enabled ? 0.78 : 0.38
    }
}
