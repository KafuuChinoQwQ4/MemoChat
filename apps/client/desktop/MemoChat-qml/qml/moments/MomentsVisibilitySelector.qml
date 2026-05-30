pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    radius: 14
    color: "#ffffff"
    border.color: "#dce4ef"
    border.width: 1

    property alias currentIndex: visibilityCombo.currentIndex

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10

        Label {
            text: "谁可见"
            font.pixelSize: 14
            color: "#202733"
        }

        Item { Layout.fillWidth: true }

        ComboBox {
            id: visibilityCombo
            Layout.preferredWidth: 150
            model: ["公开", "仅好友可见", "仅自己可见"]
            currentIndex: 0
            font.pixelSize: 13
            leftPadding: 12
            rightPadding: 28
            topPadding: 6
            bottomPadding: 6
            background: Rectangle {
                radius: 12
                color: "#f8fbff"
                border.width: 1
                border.color: "#d5dfec"
            }
            contentItem: Text {
                leftPadding: 0
                rightPadding: 0
                text: visibilityCombo.displayText
                font: visibilityCombo.font
                color: "#243246"
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: Canvas {
                x: visibilityCombo.width - width - 12
                y: (visibilityCombo.height - height) / 2
                width: 10
                height: 6
                contextType: "2d"
                onPaint: {
                    context.reset()
                    context.moveTo(0, 0)
                    context.lineTo(width, 0)
                    context.lineTo(width / 2, height)
                    context.closePath()
                    context.fillStyle = "#607086"
                    context.fill()
                }
            }
            popup: Popup {
                y: visibilityCombo.height + 6
                width: visibilityCombo.width
                padding: 6
                background: Rectangle {
                    radius: 14
                    color: "#ffffff"
                    border.width: 1
                    border.color: "#d5dfec"
                }
                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: visibilityCombo.popup.visible ? visibilityCombo.delegateModel : null
                    currentIndex: visibilityCombo.highlightedIndex
                }
            }
            delegate: ItemDelegate {
                id: visibilityDelegate

                required property int index
                required property string modelData

                width: visibilityCombo.width - 12
                height: 38
                text: visibilityDelegate.modelData
                font.pixelSize: 13
                highlighted: visibilityCombo.highlightedIndex === visibilityDelegate.index
                background: Rectangle {
                    radius: 10
                    color: visibilityDelegate.highlighted ? "#eef4ff" : "transparent"
                }
                contentItem: Text {
                    text: visibilityDelegate.text
                    font: visibilityDelegate.font
                    color: "#243246"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
