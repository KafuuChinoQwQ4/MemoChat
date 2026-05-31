pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    spacing: 12

    property color accentRose: Qt.rgba(0.78, 0.36, 0.45, 1.0)
    property color textPrimaryColor: "#253247"
    property string promptText: ""

    Live2DPromptPreviewPanel {
        accentColor: root.accentRose
        promptText: root.promptText
        textPrimaryColor: root.textPrimaryColor
    }
}
