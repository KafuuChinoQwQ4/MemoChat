import QtQuick 2.15

Item {
    id: root
    property url iconSource: ""
    signal clicked()

    width: 14
    height: 14

    Image {
        anchors.fill: parent
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
