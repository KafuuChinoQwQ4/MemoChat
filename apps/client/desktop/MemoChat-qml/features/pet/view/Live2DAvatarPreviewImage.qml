import QtQuick 2.15

Item {
    id: root

    property string imageSource: ""
    property string fallbackSource: "qrc:/icons/modelive2d.png"
    property int fallbackInset: 8

    Image {
        id: avatarImage
        anchors.fill: parent
        source: root.imageSource
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: Math.max(1, width * 2)
        sourceSize.height: Math.max(1, height * 2)
        cache: false
        mipmap: true
        visible: status === Image.Ready
    }

    Image {
        anchors.fill: parent
        anchors.margins: root.fallbackInset
        source: root.fallbackSource
        fillMode: Image.PreserveAspectFit
        sourceSize.width: Math.max(1, width * 2)
        sourceSize.height: Math.max(1, height * 2)
        mipmap: true
        visible: avatarImage.status !== Image.Ready
    }
}
