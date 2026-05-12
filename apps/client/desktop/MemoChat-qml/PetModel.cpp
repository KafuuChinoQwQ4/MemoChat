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

    const QJsonObject speech = event.value(QStringLiteral("speech")).toObject();
    const QJsonObject text = event.value(QStringLiteral("text")).toObject();
    const QString delta = jsonString(
        text, QStringLiteral("delta"), jsonString(speech, QStringLiteral("text_delta"), QString()));
    const bool text_final = jsonBool(text, QStringLiteral("final"), false);
    const QString display_text = jsonString(text, QStringLiteral("display"), QString());
    const bool has_text_update = !delta.isEmpty() || text_final || !display_text.isEmpty();

    if (has_text_update && !next_turn_id.isEmpty() && _speech_turn_id != next_turn_id) {
        _speech_turn_id = next_turn_id;
        if (!_speech_text.isEmpty()) {
            _speech_text.clear();
            changed = true;
        }
    }
    if (!delta.isEmpty()) {
        _speech_text += delta;
        changed = true;
    }
    if (text_final && !display_text.isEmpty() && _speech_text != display_text) {
        _speech_text = display_text;
        changed = true;
    } else if (delta.isEmpty() && !display_text.isEmpty() && _speech_text != display_text) {
        _speech_text = display_text;
        changed = true;
    }

    emitChangedIf(changed);
}

void PetModel::clearSpeech()
{
    if (_speech_text.isEmpty()) {
        return;
    }
    _speech_text.clear();
    _speech_turn_id.clear();
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
