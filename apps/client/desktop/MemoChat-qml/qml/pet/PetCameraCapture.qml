import QtQuick 2.15
import QtMultimedia

Item {
    id: root
    property var petController: null
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool segmentCaptureEnabled: true
    property int captureIntervalMs: segmentCaptureEnabled ? 1500 : 4500
    property int frameWidth: videoOutput.sourceRect.width > 0 ? Math.round(videoOutput.sourceRect.width) : Math.round(videoOutput.width)
    property int frameHeight: videoOutput.sourceRect.height > 0 ? Math.round(videoOutput.sourceRect.height) : Math.round(videoOutput.height)
    property var liveVideoFrame: null
    readonly property bool qtCameraAvailable: mediaDevices.videoInputs.length > 0
    readonly property bool windowsCameraBridgeAvailable: !!root.petController
                                                            && !!root.petController.windowsCameraBridgeAvailable
    readonly property bool cameraAvailable: root.qtCameraAvailable || root.windowsCameraBridgeAvailable
    readonly property bool useWindowsCameraBridge: root.cameraEnabled
                                                && !root.qtCameraAvailable
                                                && root.windowsCameraBridgeAvailable
    property string statusText: cameraEnabled ? "摄像头准备中" : "摄像头关闭"
    signal statusChanged(string text)

    width: 320
    height: 180
    visible: root.cameraEnabled
    opacity: 0.01

    function updateStatus(text) {
        statusText = text
        statusChanged(text)
    }

    function cameraStatusText() {
        if (!root.cameraEnabled) {
            return "摄像头关闭"
        }
        if (!root.cameraAvailable) {
            return root.windowsCameraBridgeAvailable ? "Windows 摄像头桥可用" : "未检测到摄像头"
        }
        if (!root.petController || root.petController.sessionId.length === 0) {
            return "等待桌宠会话"
        }
        if (root.useWindowsCameraBridge) {
            return root.petController.windowsCameraBridgeBusy ? "Windows 摄像头桥正在捕捉"
                                                              : "Windows 摄像头桥已开启"
        }
        return root.segmentCaptureEnabled ? "摄像头片段采样已开启"
                                          : (root.cloudVisionEnabled ? "摄像头本地分析，云视觉待授权" : "摄像头本地捕捉已开启")
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
        if (root.useWindowsCameraBridge) {
            if (root.petController.windowsCameraBridgeBusy) {
                updateStatus("Windows 摄像头桥正在捕捉")
                return
            }
            if (typeof root.petController.captureVisionWindowsCameraFrame === "function"
                    && root.petController.captureVisionWindowsCameraFrame()) {
                updateStatus("Windows 摄像头桥正在捕捉")
            } else {
                updateStatus(root.petController.windowsCameraBridgeBusy ? "Windows 摄像头桥正在捕捉"
                                                                        : "Windows 摄像头桥不可用")
            }
            return
        }
        if (root.segmentCaptureEnabled
                && root.liveVideoFrame !== null
                && typeof root.petController.captureVisionSegmentVideoFrame === "function") {
            var segmentStatus = root.petController.captureVisionSegmentVideoFrame(
                        root.liveVideoFrame, root.frameWidth, root.frameHeight)
            root.liveVideoFrame = null
            if (segmentStatus.length > 0) {
                updateStatus(segmentStatus)
                return
            }
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
        active: root.cameraEnabled && root.qtCameraAvailable
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
        anchors.fill: parent
        visible: root.cameraEnabled
        fillMode: VideoOutput.PreserveAspectFit
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

    onWindowsCameraBridgeAvailableChanged: updateStatus(root.cameraStatusText())
    onUseWindowsCameraBridgeChanged: updateStatus(root.cameraStatusText())

    Connections {
        target: root.petController
        function onWindowsCameraBridgeChanged() {
            root.updateStatus(root.cameraStatusText())
        }
    }
}
