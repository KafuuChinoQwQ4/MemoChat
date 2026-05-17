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
    property var liveVideoFrame: null
    readonly property bool cameraAvailable: mediaDevices.videoInputs.length > 0
    property string statusText: cameraEnabled ? "摄像头准备中" : "摄像头关闭"
    signal statusChanged(string text)

    width: 1
    height: 1
    visible: false

    function updateStatus(text) {
        statusText = text
        statusChanged(text)
    }

    function cameraStatusText() {
        if (!root.cameraEnabled) {
            return "摄像头关闭"
        }
        if (!root.cameraAvailable) {
            return "未检测到摄像头"
        }
        if (!root.petController || root.petController.sessionId.length === 0) {
            return "等待桌宠会话"
        }
        return root.cloudVisionEnabled ? "摄像头本地分析，云视觉待授权" : "摄像头本地捕捉已开启"
    }

    function captureFrame() {
        if (!root.cameraEnabled || !root.petController || root.petController.sessionId.length === 0) {
            updateStatus(root.cameraStatusText())
            return
        }
        if (!root.cameraAvailable) {
            updateStatus("未检测到摄像头")
            return
        }
        if (root.liveVideoFrame !== null && typeof root.petController.captureVisionVideoFrame === "function") {
            var liveCaptured = root.petController.captureVisionVideoFrame(
                        root.liveVideoFrame, root.frameWidth, root.frameHeight)
            root.liveVideoFrame = null
            if (liveCaptured) {
                updateStatus(root.cloudVisionEnabled ? "摄像头实时帧已分析" : "摄像头实时本地分析")
                return
            }
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
        onVideoInputsChanged: root.updateStatus(root.cameraStatusText())
    }

    Camera {
        id: camera
        active: root.cameraEnabled && root.cameraAvailable
        cameraDevice: mediaDevices.defaultVideoInput
        onErrorOccurred: function(error, errorString) {
            root.liveVideoFrame = null
            root.updateStatus(errorString.length > 0 ? errorString : "摄像头不可用")
        }
    }

    CaptureSession {
        id: captureSession
        camera: camera
        imageCapture: imageCapture
        videoOutput: videoOutput
    }

    Connections {
        target: videoOutput.videoSink
        function onVideoFrameChanged(frame) {
            if (root.cameraEnabled) {
                root.liveVideoFrame = frame
            }
        }
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
        running: root.cameraEnabled && root.cameraAvailable
                 && root.petController && root.petController.sessionId.length > 0
        triggeredOnStart: true
        onTriggered: root.captureFrame()
    }

    onCameraEnabledChanged: {
        root.liveVideoFrame = null
        updateStatus(root.cameraStatusText())
    }
}
