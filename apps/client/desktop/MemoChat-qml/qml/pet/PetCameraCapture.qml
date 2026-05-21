import QtQuick 2.15
import QtMultimedia

Item {
    id: root
    property var petController: null
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool segmentCaptureEnabled: true
    property int captureIntervalMs: root.useWindowsCameraBridge ? 12000 : 1000
    property int analysisIntervalMs: 1000
    property int latestFrameHoldMs: 1800
    property int fallbackStillCaptureDelayMs: 2500
    property int frameWidth: videoOutput.sourceRect.width > 0 ? Math.round(videoOutput.sourceRect.width) : Math.round(videoOutput.width)
    property int frameHeight: videoOutput.sourceRect.height > 0 ? Math.round(videoOutput.sourceRect.height) : Math.round(videoOutput.height)
    property var liveVideoFrame: null
    property var latestVideoFrame: null
    property double latestVideoFrameAtMs: 0
    property double cameraStartedAtMs: 0
    property int latestVideoFrameSequence: 0
    property int latestSubmittedFrameSequence: 0
    property bool videoSinkFrameSeen: false
    property bool firstFrameSubmitted: false
    property bool pendingLatestFrameAnalysis: false
    property bool latestFrameCapturePending: false
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

    function visionBusy() {
        return !!root.petController && !!root.petController.visionRequestInFlight
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
        if (visionBusy()) {
            return "摄像头最新帧分析中"
        }
        return root.segmentCaptureEnabled ? "摄像头最新帧采样已开启"
                                          : (root.cloudVisionEnabled ? "摄像头本地分析，云视觉待授权" : "摄像头本地捕捉已开启")
    }

    function resetVideoMailbox() {
        root.liveVideoFrame = null
        root.latestVideoFrame = null
        root.latestVideoFrameSequence = 0
        root.latestSubmittedFrameSequence = 0
        root.latestVideoFrameAtMs = 0
        root.videoSinkFrameSeen = false
        root.firstFrameSubmitted = false
        root.pendingLatestFrameAnalysis = false
        root.latestFrameCapturePending = false
        root.cameraStartedAtMs = root.cameraEnabled ? Date.now() : 0
    }

    function scheduleLatestFrameAnalysis(delayMs) {
        latestFrameAnalysisTimer.interval = Math.max(20, delayMs === undefined ? 80 : delayMs)
        latestFrameAnalysisTimer.restart()
    }

    function maybeCaptureStillFallback() {
        if (root.videoSinkFrameSeen || root.latestFrameCapturePending) {
            return false
        }
        var now = Date.now()
        if (root.cameraStartedAtMs <= 0 || (now - root.cameraStartedAtMs) < root.fallbackStillCaptureDelayMs) {
            updateStatus("等待摄像头视频流")
            return false
        }
        var path = root.petController.nextVisionCaptureFilePath()
        if (path.length > 0 && root.segmentCaptureEnabled && imageCapture) {
            root.latestFrameCapturePending = true
            imageCapture.captureToFile(path)
            updateStatus("摄像头视频流未就绪，使用单帧兜底")
            return true
        }
        return false
    }

    function submitLatestVideoFrame(reason) {
        if (!root.cameraEnabled || !root.petController || root.petController.sessionId.length === 0) {
            updateStatus(root.cameraStatusText())
            return false
        }
        if (!root.cameraAvailable) {
            updateStatus("未检测到摄像头")
            return false
        }
        if (root.useWindowsCameraBridge) {
            if (root.petController.windowsCameraBridgeBusy) {
                updateStatus("Windows 摄像头桥正在捕捉")
                return false
            }
            if (typeof root.petController.captureVisionWindowsCameraFrame === "function"
                    && root.petController.captureVisionWindowsCameraFrame()) {
                updateStatus("Windows 摄像头桥正在捕捉")
                return true
            } else {
                updateStatus(root.petController.windowsCameraBridgeBusy ? "Windows 摄像头桥正在捕捉"
                                                                        : "Windows 摄像头桥不可用")
            }
            return false
        }
        if (root.latestVideoFrameAtMs > 0 && (Date.now() - root.latestVideoFrameAtMs) > root.latestFrameHoldMs) {
            root.latestVideoFrame = null
            root.pendingLatestFrameAnalysis = false
            updateStatus("摄像头画面已过期，等待新帧")
            return false
        }
        if (root.latestVideoFrame === null) {
            if (root.latestFrameCapturePending) {
                updateStatus("摄像头缓存帧等待回调")
                return false
            }
            if (maybeCaptureStillFallback()) {
                return true
            }
            updateStatus(visionBusy() ? "摄像头最新帧分析中" : root.cameraStatusText())
            return false
        }
        if (visionBusy()) {
            root.pendingLatestFrameAnalysis = true
            updateStatus("摄像头最新帧分析中")
            return false
        }
        if (typeof root.petController.captureVisionVideoFrame !== "function") {
            updateStatus("摄像头分析接口不可用")
            return false
        }
        var frame = root.latestVideoFrame
        var frameSeq = root.latestVideoFrameSequence
        if (!frame || frameSeq <= root.latestSubmittedFrameSequence) {
            root.pendingLatestFrameAnalysis = false
            updateStatus("摄像头最新帧等待分析")
            return false
        }
        var captured = root.petController.captureVisionVideoFrame(frame, root.frameWidth, root.frameHeight)
        if (captured) {
            root.latestSubmittedFrameSequence = frameSeq
            root.firstFrameSubmitted = true
            root.pendingLatestFrameAnalysis = false
            updateStatus(root.cloudVisionEnabled ? "摄像头最新帧已提交分析" : "摄像头最新帧已提交本地分析")
            return true
        } else {
            if (!visionBusy() && frameSeq > root.latestSubmittedFrameSequence) {
                root.latestSubmittedFrameSequence = frameSeq
                root.firstFrameSubmitted = true
                root.pendingLatestFrameAnalysis = false
                updateStatus("摄像头画面未变化，已跳过")
            } else {
                root.pendingLatestFrameAnalysis = true
                updateStatus("摄像头最新帧分析中")
            }
        }
        return false
    }

    function captureFrame() {
        submitLatestVideoFrame("tick")
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
                root.latestVideoFrame = frame
                root.latestFrameCapturePending = false
                root.videoSinkFrameSeen = true
                root.latestVideoFrameSequence += 1
                root.latestVideoFrameAtMs = Date.now()
                if (!root.firstFrameSubmitted && !root.visionBusy()) {
                    root.submitLatestVideoFrame("first_video_sink_frame")
                } else if (root.visionBusy()) {
                    root.pendingLatestFrameAnalysis = true
                }
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
            root.latestFrameCapturePending = false
            if (root.visionBusy()) {
                root.pendingLatestFrameAnalysis = true
                root.updateStatus("摄像头最新帧分析中")
                return
            }
            root.petController.captureVisionFrameFile(fileName, root.frameWidth, root.frameHeight)
            root.firstFrameSubmitted = true
            root.pendingLatestFrameAnalysis = false
            root.updateStatus(root.cloudVisionEnabled ? "摄像头帧已提交分析" : "摄像头帧已提交本地分析")
        }
        onErrorOccurred: function(id, error, errorString) {
            root.latestFrameCapturePending = false
            root.updateStatus(errorString.length > 0 ? errorString : "摄像头帧捕获失败")
        }
    }

    Timer {
        id: analysisTimer
        interval: root.analysisIntervalMs
        repeat: true
        running: root.cameraEnabled && root.cameraAvailable
                 && root.petController && root.petController.sessionId.length > 0
        triggeredOnStart: true
        onTriggered: root.captureFrame()
    }

    onCameraEnabledChanged: {
        root.resetVideoMailbox()
        updateStatus(root.cameraStatusText())
    }
    onWindowsCameraBridgeAvailableChanged: updateStatus(root.cameraStatusText())
    onUseWindowsCameraBridgeChanged: updateStatus(root.cameraStatusText())

    Timer {
        id: latestFrameAnalysisTimer
        interval: 80
        repeat: false
        onTriggered: root.submitLatestVideoFrame("latest_frame_ready")
    }

    Connections {
        target: root.petController
        function onStateChanged() {
            if (!root.visionBusy()
                    && root.pendingLatestFrameAnalysis
                    && root.latestVideoFrameSequence > root.latestSubmittedFrameSequence) {
                root.scheduleLatestFrameAnalysis(80)
            }
        }
        function onWindowsCameraBridgeChanged() {
            root.updateStatus(root.cameraStatusText())
        }
    }

    Component.onCompleted: root.resetVideoMailbox()
}
