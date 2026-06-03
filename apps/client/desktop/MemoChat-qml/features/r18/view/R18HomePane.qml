pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root

    property string keyword: ""
    property int historyCount: 0
    property int localCount: 0
    property int followCount: 0
    property int sourceCount: 0
    property int favoriteCount: 0
    property color homeFieldFillColor: "#eceef3"
    property color homeFieldStrokeColor: "#d7dce5"
    property color homeCardFillColor: "#ffffff"
    property color homeCardStrokeColor: "#d8dde6"
    property color homeBadgeFillColor: "#dce6f8"
    property color homeBadgeTextColor: "#526173"
    property color homeArrowColor: "#2f343c"
    property color homeImportButtonColor: "#0c4f92"
    property color homeImportButtonHoverColor: "#0f61b0"
    property color homeImportButtonPressedColor: "#093d72"
    property color textPrimaryColor: "#263241"
    property color textMutedColor: "#7b8794"
    property color placeholderColor: "#8493a3"

    signal keywordEdited(string keyword)
    signal searchRequested(string keyword)
    signal entryActivated(string entryAction, int modeValue)
    signal importRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        TextField {
            id: homeSearchField
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            placeholderText: "搜索"
            placeholderTextColor: root.placeholderColor
            color: root.textPrimaryColor
            text: root.keyword
            selectByMouse: true
            leftPadding: 50
            rightPadding: 18
            font.pixelSize: 16
            background: Rectangle {
                radius: 28
                color: root.homeFieldFillColor
                border.color: root.homeFieldStrokeColor
            }
            onTextEdited: root.keywordEdited(text)
            onAccepted: root.searchRequested(text)

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
                text: "⌕"
                color: root.textMutedColor
                font.pixelSize: 24
            }
        }

        ListView {
            id: homeListView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 12
            model: ListModel {
                ListElement { title: "历史"; modeValue: 5; entryAction: "mode"; countKey: "history" }
                ListElement { title: "本地"; modeValue: 4; entryAction: "import"; countKey: "local" }
                ListElement { title: "追更"; modeValue: 1; entryAction: "mode"; countKey: "follow" }
                ListElement { title: "漫画源"; modeValue: 4; entryAction: "mode"; countKey: "source" }
                ListElement { title: "图片收藏"; modeValue: 1; entryAction: "favorite"; countKey: "favorite" }
            }
            ScrollBar.vertical: GlassScrollBar {}
            delegate: R18HomeCard {
                required property int index

                readonly property var entry: homeListView.model.get(index)

                title: entry.title
                count: countForKey(entry.countKey)
                modeValue: entry.modeValue
                entryAction: entry.entryAction
                textPrimaryColor: root.textPrimaryColor
                cardFillColor: root.homeCardFillColor
                cardStrokeColor: root.homeCardStrokeColor
                badgeFillColor: root.homeBadgeFillColor
                badgeTextColor: root.homeBadgeTextColor
                arrowColor: root.homeArrowColor
                importButtonColor: root.homeImportButtonColor
                importButtonHoverColor: root.homeImportButtonHoverColor
                importButtonPressedColor: root.homeImportButtonPressedColor
                onActivated: function(action, modeValue) {
                    root.entryActivated(action, modeValue)
                }
                onImportRequested: root.importRequested()
            }
        }
    }

    function countForKey(countKey) {
        if (countKey === "history") {
            return root.historyCount
        }
        if (countKey === "local") {
            return root.localCount
        }
        if (countKey === "follow") {
            return root.followCount
        }
        if (countKey === "source") {
            return root.sourceCount
        }
        if (countKey === "favorite") {
            return root.favoriteCount
        }
        return 0
    }
}
