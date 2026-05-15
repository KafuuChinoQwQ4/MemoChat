import QtQuick 2.15
import QtMultimedia

Item {
    id: root
    property var petController: null
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property int captureIntervalMs: 4500
    property int frameWidth: Math.round(videoOutput.width)
    property int frameHeight: Math.round(videoOutput.height)
    property string statusText: cameraEnabled ? "摄像头准备中" : "摄像头关闭"
    signal statusChanged(string text)

    width: 1
    height: 1
    visible: false

    function updateStatus(text) {
        statusText = text
        statusChanged(text)
    }

    function captureFrame() {
        if (!root.cameraEnabled || !root.petController || root.petController.sessionId.length === 0) {
            return
        }
        var path = root.petController.nextVisionCaptureFilePath()
        if (path.length === 0) {
            updateStatus("摄像头缓存路径不可用")
            return
        }
        imageCapture.captureToFile(path)
    }

    MediaDevices {
        id: mediaDevices
    }

    Camera {
        id: camera
        active: root.cameraEnabled
        cameraDevice: mediaDevices.defaultVideoInput
        onErrorOccurred: function(error, errorString) {
            root.updateStatus(errorString.length > 0 ? errorString : "摄像头不可用")
        }
    }

    CaptureSession {
        id: captureSession
        camera: camera
        imageCapture: imageCapture
        videoOutput: videoOutput
    }

    VideoOutput {
        id: videoOutput
        width: 320
        height: 180
        visible: false
    }

    ImageCapture {
        id: imageCapture
        quality: ImageCapture.LowQuality
        onImageSaved: function(id, fileName) {
            if (!root.petController || !root.cameraEnabled) {
                return
            }
            root.petController.captureVisionFrameFile(fileName, root.frameWidth, root.frameHeight)
            root.updateStatus(root.cloudVisionEnabled ? "摄像头帧已分析" : "摄像头本地分析")
        }
        onErrorOccurred: function(id, error, errorString) {
            root.updateStatus(errorString.length > 0 ? errorString : "摄像头帧捕获失败")
        }
    }

    Timer {
        id: captureTimer
        interval: root.captureIntervalMs
        repeat: true
        running: root.cameraEnabled && root.petController && root.petController.sessionId.length > 0
        triggeredOnStart: true
        onTriggered: root.captureFrame()
    }

    onCameraEnabledChanged: {
        if (cameraEnabled) {
            updateStatus("摄像头本地捕捉已开启")
        } else {
            updateStatus("摄像头关闭")
        }
    }
}
