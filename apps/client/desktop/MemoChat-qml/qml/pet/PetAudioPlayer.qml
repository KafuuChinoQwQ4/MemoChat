import QtQuick 2.15
import QtMultimedia

Item {
    id: root
    property string sourceUrl: ""
    property string playbackState: "idle"
    property string lastStartedAudioKey: ""
    property string lastPlayedAudioKey: ""
    property bool playbackActive: false
    property int retryCount: 0
    readonly property int maxRetryCount: 2

    visible: false

    function shouldStopForState() {
        return playbackState === "listening"
                || playbackState === "interrupted"
                || playbackState === "stopped"
                || playbackState === "error"
    }

    function canPlayForState() {
        return playbackState === "ready" || playbackState === "playing"
    }

    function hasPlayableAudio() {
        return sourceUrl.length > 0
    }

    function audioPlaybackKey(url) {
        var text = String(url)
        var fragmentIndex = text.indexOf("#")
        if (fragmentIndex >= 0) {
            text = text.substring(0, fragmentIndex)
        }
        var queryIndex = text.indexOf("?")
        if (queryIndex < 0) {
            return text
        }
        var base = text.substring(0, queryIndex)
        var query = text.substring(queryIndex + 1)
        var kept = []
        var parts = query.split("&")
        for (var index = 0; index < parts.length; ++index) {
            var part = parts[index]
            if (part.length === 0) {
                continue
            }
            var equalsIndex = part.indexOf("=")
            var name = equalsIndex >= 0 ? part.substring(0, equalsIndex) : part
            if (name === "event" || name === "turn") {
                continue
            }
            kept.push(part)
        }
        return kept.length > 0 ? base + "?" + kept.join("&") : base
    }

    function currentSourceAudioKey() {
        return audioPlaybackKey(petAudioPlayer.source)
    }

    function playableAudioKey() {
        return audioPlaybackKey(root.sourceUrl)
    }

    function refreshPlaybackActivity() {
        var active = root.lastStartedAudioKey.length > 0
                || petAudioPlayer.playbackState === MediaPlayer.PlayingState
                || petAudioPlayer.playbackState === MediaPlayer.PausedState
        if (root.playbackActive !== active) {
            root.playbackActive = active
        }
    }

    function assignSource() {
        if (currentSourceAudioKey() !== audioPlaybackKey(sourceUrl)) {
            petAudioPlayer.source = sourceUrl
        }
    }

    function playPlayableAudio(forceReplay) {
        var key = playableAudioKey()
        if (key.length === 0
                || root.lastStartedAudioKey === key
                || (!forceReplay && root.lastPlayedAudioKey === key)) {
            return
        }
        assignSource()
        root.lastStartedAudioKey = key
        root.lastPlayedAudioKey = key
        refreshPlaybackActivity()
        petAudioPlayer.play()
    }

    function retryPlayableAudio() {
        if (!hasPlayableAudio() || !canPlayForState()) {
            return
        }
        root.lastStartedAudioKey = ""
        assignSource()
        playPlayableAudio(true)
    }

    function applyPlayback() {
        if (shouldStopForState()) {
            petAudioPlayer.stop()
            root.lastStartedAudioKey = ""
            refreshPlaybackActivity()
            return
        }
        if (!hasPlayableAudio()) {
            petAudioPlayer.stop()
            petAudioPlayer.source = ""
            root.lastStartedAudioKey = ""
            refreshPlaybackActivity()
            return
        }
        assignSource()
        if (canPlayForState()) {
            playPlayableAudio()
        }
    }

    MediaPlayer {
        id: petAudioPlayer
        audioOutput: AudioOutput {
            volume: 1.0
        }

        onErrorOccurred: function(error, errorString) {
            if (error !== MediaPlayer.NoError) {
                console.warn("pet audio playback failed:", errorString)
                if (root.hasPlayableAudio()
                        && root.canPlayForState()
                        && root.retryCount < root.maxRetryCount) {
                    root.retryCount += 1
                    petAudioPlayer.stop()
                    petAudioPlayer.source = ""
                    root.lastStartedAudioKey = ""
                    retryTimer.restart()
                    root.refreshPlaybackActivity()
                    return
                }
                petAudioPlayer.stop()
                petAudioPlayer.source = ""
                root.lastStartedAudioKey = ""
                root.refreshPlaybackActivity()
            }
        }

        onPlaybackStateChanged: {
            if (petAudioPlayer.playbackState === MediaPlayer.StoppedState
                    && root.lastStartedAudioKey.length > 0) {
                root.lastStartedAudioKey = ""
            }
            root.refreshPlaybackActivity()
        }
    }

    onSourceUrlChanged: {
        retryTimer.stop()
        retryCount = 0
        applyPlayback()
        refreshPlaybackActivity()
    }

    Timer {
        id: retryTimer
        interval: 650
        repeat: false
        onTriggered: root.retryPlayableAudio()
    }

    onPlaybackStateChanged: {
        if (!hasPlayableAudio() || shouldStopForState()) {
            petAudioPlayer.stop()
            if (shouldStopForState()) {
                root.lastStartedAudioKey = ""
            }
            refreshPlaybackActivity()
            return
        }
        if (canPlayForState()) {
            playPlayableAudio()
        }
        refreshPlaybackActivity()
    }
}
