import QtQuick 2.15
import QtQuick.Controls 2.15

Row {
    id: root
    property alias checked: termsCheck.checked
    property string agreementText: "已阅读并同意<a href=\"https://example.com/agreement\">服务协议</a>和<a href=\"https://example.com/privacy\">Memo隐私保护指引</a>"
    signal linkActivated(string link)

    width: 0
    height: 24
    spacing: 6

    CheckBox {
        id: termsCheck
        checked: false
        anchors.verticalCenter: parent.verticalCenter
        focusPolicy: Qt.NoFocus
        hoverEnabled: false
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0
        implicitWidth: 18
        implicitHeight: 18

        indicator: Rectangle {
            implicitWidth: 18
            implicitHeight: 18
            radius: 9
            border.width: 1
            border.color: "#8ea0b4"
            color: termsCheck.checked ? "#4f9edd" : "transparent"

            Canvas {
                anchors.fill: parent
                visible: termsCheck.checked
                antialiasing: true
                onVisibleChanged: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!visible) {
                        return
                    }
                    ctx.strokeStyle = "#ffffff"
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(width * 0.28, height * 0.55)
                    ctx.lineTo(width * 0.45, height * 0.72)
                    ctx.lineTo(width * 0.74, height * 0.36)
                    ctx.stroke()
                }
            }
        }

        background: Item { }
        contentItem: Item { }
    }

    Text {
        width: parent.width - termsCheck.implicitWidth - parent.spacing
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        textFormat: Text.RichText
        font.pixelSize: 11
        color: "#6d7280"
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        text: root.agreementText
        onLinkActivated: function(link) { root.linkActivated(link) }
    }
}
