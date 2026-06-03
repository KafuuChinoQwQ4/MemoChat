import QtQuick 2.15

Item {
    id: root

    property string imageSource: ""
    property real maxWidth: 280
    property real maxHeight: 240

    function targetSize() {
        const srcWidth = img.status === Image.Ready ? Math.max(1, img.implicitWidth) : 180
        const srcHeight = img.status === Image.Ready ? Math.max(1, img.implicitHeight) : 132
        const downScale = Math.min(root.maxWidth / srcWidth,
                                   root.maxHeight / srcHeight,
                                   1.0)
        let width = srcWidth * downScale
        let height = srcHeight * downScale
        if (img.status === Image.Ready && width < 96 && height < 96) {
            const upScale = Math.min(root.maxWidth / srcWidth,
                                     root.maxHeight / srcHeight,
                                     96 / Math.max(width, height))
            width = srcWidth * upScale
            height = srcHeight * upScale
        }
        return Qt.size(Math.round(width), Math.round(height))
    }

    readonly property size fittedSize: targetSize()
    implicitWidth: fittedSize.width
    implicitHeight: fittedSize.height

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: Qt.rgba(0.88, 0.93, 0.98, 0.42)
        border.color: Qt.rgba(0.44, 0.61, 0.82, 0.20)
    }

    Image {
        id: img
        anchors.fill: parent
        anchors.margins: img.status === Image.Error ? 12 : 0
        fillMode: Image.PreserveAspectFit
        source: root.imageSource
    }

    Text {
        anchors.centerIn: parent
        visible: img.status === Image.Loading
        text: "图片加载中..."
        color: "#5f728b"
        font.pixelSize: 12
    }

    Text {
        anchors.centerIn: parent
        visible: img.status === Image.Error
        text: "图片加载失败"
        color: "#6c7d92"
        font.pixelSize: 12
    }
}
