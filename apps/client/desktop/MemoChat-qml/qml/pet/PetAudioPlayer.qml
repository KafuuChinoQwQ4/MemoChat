import QtQuick 2.15
import QtMultimedia
import MemoChat 1.0

Item {
    id: root
    property string sourceUrl: ""
    property string playbackState: "idle"
    property string speechText: ""
    property bool speechFinal: false
    property bool textToSpeechFallbackEnabled: false
    property string speechKey: ""
    property string lastSpokenKey: ""
    property var speechEngine: null

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
        return sourceUrl.length > 0 && !isDeterministicFallbackAudio()
    }

    function isDeterministicFallbackAudio() {
        return sourceUrl.indexOf("/audio/deterministic-voice-") >= 0
                || sourceUrl.indexOf("deterministic-voice-") >= 0
    }

    function assignSource() {
        if (String(petAudioPlayer.source) !== sourceUrl) {
            petAudioPlayer.source = sourceUrl
        }
    }

    function stopSpeech() {
        nativeSpeech.stop()
        if (speechEngine && speechEngine.stop) {
            speechEngine.stop()
        }
    }

    function ensureSpeechEngine() {
        if (speechEngine) {
            return speechEngine
        }
        try {
            speechEngine = Qt.createQmlObject('import QtTextToSpeech; TextToSpeech { locale: Qt.locale("zh_CN"); volume: 1.0; rate: 0.0; pitch: 0.0 }',
                                              root,
                                              "PetTextToSpeechFallback")
        } catch (error) {
            console.warn("pet text-to-speech unavailable:", error)
            speechEngine = null
        }
        return speechEngine
    }

    function maybeSpeakText() {
        if (!root.textToSpeechFallbackEnabled) {
            return
        }
        var key = (root.speechKey.length > 0 ? root.speechKey : "speech") + ":" + root.speechText
        if (!root.speechFinal || root.speechText.trim().length === 0 || key === root.lastSpokenKey) {
            return
        }
        if (nativeSpeech.speak(root.speechText, "zh-CN")) {
            root.lastSpokenKey = key
            petAudioPlayer.stop()
            return
        }
        var engine = ensureSpeechEngine()
        if (!engine || !engine.say) {
            return
        }
        root.lastSpokenKey = key
        petAudioPlayer.stop()
        engine.say(root.speechText)
    }

    function applyPlayback() {
        if (!root.speechFinal && root.speechText.trim().length === 0) {
            stopSpeech()
        }
        if (shouldStopForState()) {
            petAudioPlayer.stop()
            stopSpeech()
            return
        }
        if (!hasPlayableAudio()) {
            petAudioPlayer.stop()
            petAudioPlayer.source = ""
            if (sourceUrl.length === 0) {
                maybeSpeakText()
            }
            return
        }
        assignSource()
        if (canPlayForState()) {
            stopSpeech()
            petAudioPlayer.play()
        }
    }

    PetSpeechSynthesizer {
        id: nativeSpeech
        onErrorOccurred: function(message) {
            if (message.length > 0) {
                console.warn("pet native text-to-speech unavailable:", message)
            }
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
            }
        }
    }

    onSourceUrlChanged: {
        applyPlayback()
    }

    onPlaybackStateChanged: {
        if (!hasPlayableAudio() || shouldStopForState()) {
            petAudioPlayer.stop()
            if (shouldStopForState()) {
                stopSpeech()
            } else if (sourceUrl.length === 0) {
                maybeSpeakText()
            }
            return
        }
        if (canPlayForState()) {
            assignSource()
            stopSpeech()
            petAudioPlayer.play()
        }
    }

    onSpeechTextChanged: maybeSpeakText()
    onSpeechFinalChanged: applyPlayback()
    onSpeechKeyChanged: maybeSpeakText()
}
