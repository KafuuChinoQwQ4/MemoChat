#include "PetModel.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtGlobal>
#include <cmath>

namespace {

QString jsonString(const QJsonObject &object, const QString &key, const QString &fallback)
{
    const QJsonValue value = object.value(key);
    if (value.isString()) {
        return value.toString();
    }
    return fallback;
}

int jsonInt(const QJsonObject &object, const QString &key, int fallback)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return value.toInt(fallback);
    }
    if (value.isString()) {
        bool ok = false;
        const int parsed = value.toString().toInt(&ok);
        return ok ? parsed : fallback;
    }
    return fallback;
}

qreal jsonReal(const QJsonObject &object, const QString &key, qreal fallback)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return value.toDouble(fallback);
    }
    if (value.isString()) {
        bool ok = false;
        const qreal parsed = value.toString().toDouble(&ok);
        return ok ? parsed : fallback;
    }
    return fallback;
}

bool jsonBool(const QJsonObject &object, const QString &key, bool fallback)
{
    const QJsonValue value = object.value(key);
    if (value.isBool()) {
        return value.toBool(fallback);
    }
    if (value.isDouble()) {
        return !qFuzzyIsNull(value.toDouble());
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("1") || text == QStringLiteral("yes")) {
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("0") || text == QStringLiteral("no")) {
            return false;
        }
    }
    return fallback;
}

bool updateString(QString &target, const QString &value)
{
    if (target == value) {
        return false;
    }
    target = value;
    return true;
}

bool isChineseLanguage(const QString &language)
{
    const QString normalized = language.trimmed().toLower();
    return normalized.isEmpty()
           || normalized == QStringLiteral("zh")
           || normalized.startsWith(QStringLiteral("zh-"))
           || normalized.contains(QStringLiteral("chinese"))
           || normalized.contains(QStringLiteral("中文"));
}

QString displayTextForSpeech(const QString &speechText, const QString &translation, const QString &language)
{
    const QString original = speechText.trimmed();
    const QString translated = translation.trimmed();
    if (original.isEmpty()) {
        return translated;
    }
    if (!isChineseLanguage(language) && !translated.isEmpty() && translated != original) {
        return original + QStringLiteral("\n") + translated;
    }
    return original;
}

}

PetModel::PetModel(QObject *parent)
    : QObject(parent)
{
}

void PetModel::applyControlEvent(const QJsonObject &event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control")) {
        return;
    }

    bool changed = false;
    const int next_sequence = event.value(QStringLiteral("seq")).toInt(_sequence);
    if (_sequence != next_sequence) {
        _sequence = next_sequence;
        changed = true;
    }

    const int next_schema_version = jsonInt(event, QStringLiteral("schema_version"), _schema_version);
    if (_schema_version != next_schema_version) {
        _schema_version = next_schema_version;
        changed = true;
    }

    const QString next_event_id = jsonString(event, QStringLiteral("event_id"), _event_id);
    if (updateString(_event_id, next_event_id)) {
        changed = true;
    }

    const QString next_turn_id = jsonString(event, QStringLiteral("turn_id"), _turn_id);
    if (updateString(_turn_id, next_turn_id)) {
        changed = true;
    }

    const QString next_phase = jsonString(event, QStringLiteral("phase"), _phase);
    if (updateString(_phase, next_phase)) {
        changed = true;
    }
    const bool reset_speech_for_waiting_phase =
        next_phase == QStringLiteral("listening") || next_phase == QStringLiteral("thinking");

    const QString next_emotion = jsonString(event, QStringLiteral("emotion"), _emotion);
    if (updateString(_emotion, next_emotion)) {
        changed = true;
    }

    const QJsonObject animation = event.value(QStringLiteral("animation")).toObject();
    const QString next_expression = jsonString(
        animation, QStringLiteral("expression"), jsonString(event, QStringLiteral("expression"), _expression));
    if (updateString(_expression, next_expression)) {
        changed = true;
    }

    const QString next_motion = jsonString(
        animation,
        QStringLiteral("motion"),
        jsonString(animation, QStringLiteral("motion_group"), jsonString(event, QStringLiteral("motion"), _motion)));
    if (updateString(_motion, next_motion)) {
        changed = true;
    }

    const qreal next_intensity = boundedUnit(jsonReal(event, QStringLiteral("intensity"), _intensity), _intensity);
    if (!qFuzzyCompare(_intensity + 1.0, next_intensity + 1.0)) {
        _intensity = next_intensity;
        changed = true;
    }

    const QJsonObject gaze = event.value(QStringLiteral("gaze")).toObject();
    const qreal next_gaze_x = boundedUnit(jsonReal(gaze, QStringLiteral("x"), _gaze_x), _gaze_x);
    const qreal next_gaze_y = boundedUnit(jsonReal(gaze, QStringLiteral("y"), _gaze_y), _gaze_y);
    if (!qFuzzyCompare(_gaze_x + 1.0, next_gaze_x + 1.0)) {
        _gaze_x = next_gaze_x;
        changed = true;
    }
    if (!qFuzzyCompare(_gaze_y + 1.0, next_gaze_y + 1.0)) {
        _gaze_y = next_gaze_y;
        changed = true;
    }

    const QJsonObject lip_sync = event.value(QStringLiteral("lip_sync")).toObject();
    const QJsonObject audio = event.value(QStringLiteral("audio")).toObject();
    const qreal next_lip = boundedUnit(
        jsonReal(lip_sync, QStringLiteral("value"), jsonReal(audio, QStringLiteral("rms"), _lip_sync_value)),
        _lip_sync_value);
    if (!qFuzzyCompare(_lip_sync_value + 1.0, next_lip + 1.0)) {
        _lip_sync_value = next_lip;
        changed = true;
    }
    const QString next_audio_url = jsonString(audio, QStringLiteral("url"), QString());
    const QString next_audio_state = jsonString(audio, QStringLiteral("state"), _audio_state);
    if (reset_speech_for_waiting_phase && !_audio_url.isEmpty()) {
        _audio_url.clear();
        changed = true;
    } else if (!next_audio_url.isEmpty() && updateString(_audio_url, next_audio_url)) {
        changed = true;
    }
    if (updateString(_audio_state, next_audio_state)) {
        changed = true;
    }

    const QJsonObject speech = event.value(QStringLiteral("speech")).toObject();
    const QJsonObject text = event.value(QStringLiteral("text")).toObject();
    const QString delta = jsonString(
        text, QStringLiteral("delta"), jsonString(speech, QStringLiteral("text_delta"), QString()));
    const bool text_final = jsonBool(text, QStringLiteral("final"), false);
    const QString display_text = jsonString(text, QStringLiteral("display"), QString());
    const QString text_language = jsonString(text, QStringLiteral("language"), _speech_language);
    const QString translated_text = jsonString(
        text,
        QStringLiteral("translation"),
        jsonString(speech,
                   QStringLiteral("translation"),
                   jsonString(text, QStringLiteral("display_translation"), QString())));
    const bool has_text_update = !delta.isEmpty()
                                 || text_final
                                 || !display_text.isEmpty()
                                 || !translated_text.isEmpty();

    if (has_text_update && !next_turn_id.isEmpty() && _speech_turn_id != next_turn_id) {
        _speech_turn_id = next_turn_id;
        if (!_speech_text.isEmpty() || !_speech_translation.isEmpty() || !_speech_display_text.isEmpty() || _speech_final) {
            _speech_text.clear();
            _speech_translation.clear();
            _speech_display_text.clear();
            _speech_final = false;
            changed = true;
        }
    }
    if (reset_speech_for_waiting_phase
        && (!_speech_text.isEmpty() || !_speech_translation.isEmpty() || !_speech_display_text.isEmpty() || _speech_final)) {
        _speech_text.clear();
        _speech_translation.clear();
        _speech_display_text.clear();
        _speech_final = false;
        changed = true;
    }
    if (updateString(_speech_language, text_language)) {
        changed = true;
    }
    if (has_text_update && _speech_final != text_final) {
        _speech_final = text_final;
        changed = true;
    }
    if (!delta.isEmpty()) {
        _speech_text += delta;
        changed = true;
    }
    if (text_final) {
        if (!display_text.isEmpty() && _speech_text != display_text) {
            _speech_text = display_text;
            changed = true;
        }
    }
    if (text_final && !translated_text.isEmpty() && updateString(_speech_translation, translated_text)) {
        changed = true;
    }
    const QString next_display_text = displayTextForSpeech(_speech_text, _speech_translation, _speech_language);
    if (updateString(_speech_display_text, next_display_text)) {
        changed = true;
    }

    emitChangedIf(changed);
}

void PetModel::clearSpeech()
{
    if (_speech_text.isEmpty() && _speech_translation.isEmpty() && _speech_display_text.isEmpty()
        && !_speech_final && _audio_url.isEmpty() && _audio_state == QStringLiteral("idle")) {
        return;
    }
    _speech_text.clear();
    _speech_translation.clear();
    _speech_display_text.clear();
    _speech_language = QStringLiteral("zh-CN");
    _speech_turn_id.clear();
    _speech_final = false;
    _audio_url.clear();
    _audio_state = QStringLiteral("idle");
    emit changed();
}

qreal PetModel::boundedUnit(qreal value, qreal fallback)
{
    if (std::isnan(value)) {
        return fallback;
    }
    return qBound(0.0, value, 1.0);
}

void PetModel::emitChangedIf(bool changed)
{
    if (changed) {
        emit this->changed();
    }
}
