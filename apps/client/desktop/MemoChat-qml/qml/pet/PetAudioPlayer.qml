import QtQuick 2.15
import QtMultimedia
import MemoChat 1.0

Item {
    id: root
    property string sourceUrl: ""
    property string playbackState: "idle"
    property string speechText: ""
    property string speechLanguage: "zh-CN"
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

    function shouldUseTextFallback() {
        return (sourceUrl.length === 0 || isDeterministicFallbackAudio())
                && playbackState !== "ready"
                && playbackState !== "playing"
                && playbackState !== "loading"
                && playbackState !== "buffering"
    }

    function normalizedSpeechLocale() {
        var locale = root.speechLanguage.trim()
        if (locale.length === 0) {
            locale = "zh-CN"
        }
        var lower = locale.toLowerCase().replace("_", "-")
        if (lower.indexOf("ja") === 0 || lower === "jp") {
            return "ja-JP"
        }
        if (lower.indexOf("zh") === 0) {
            return "zh-CN"
        }
        if (lower.indexOf("en") === 0) {
            return "en-US"
        }
        return locale
    }

    function qtLocaleName() {
        return normalizedSpeechLocale().replace("-", "_")
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
            speechEngine = Qt.createQmlObject('import QtTextToSpeech; TextToSpeech { volume: 1.0; rate: 0.0; pitch: 0.0 }',
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
        var locale = normalizedSpeechLocale()
        var key = (root.speechKey.length > 0 ? root.speechKey : "speech") + ":" + locale + ":" + root.speechText
        if (!root.speechFinal || root.speechText.trim().length === 0 || key === root.lastSpokenKey) {
            return
        }
        if (nativeSpeech.speak(root.speechText, locale)) {
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
        if (engine.locale !== undefined) {
            engine.locale = Qt.locale(qtLocaleName())
        }
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
            if (shouldUseTextFallback()) {
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
                petAudioPlayer.stop()
                petAudioPlayer.source = ""
                root.maybeSpeakText()
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
            } else if (shouldUseTextFallback()) {
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
    onSpeechLanguageChanged: maybeSpeakText()
    onSpeechFinalChanged: applyPlayback()
    onSpeechKeyChanged: maybeSpeakText()
}
