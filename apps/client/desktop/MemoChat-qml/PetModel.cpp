#include "PetModel.h"

#include <QJsonObject>
#include <QtGlobal>
#include <cmath>

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

    const QString next_emotion = event.value(QStringLiteral("emotion")).toString(_emotion);
    if (_emotion != next_emotion) {
        _emotion = next_emotion;
        changed = true;
    }

    const QString next_expression = event.value(QStringLiteral("expression")).toString(_expression);
    if (_expression != next_expression) {
        _expression = next_expression;
        changed = true;
    }

    const QString next_motion = event.value(QStringLiteral("motion")).toString(_motion);
    if (_motion != next_motion) {
        _motion = next_motion;
        changed = true;
    }

    const qreal next_intensity = boundedUnit(event.value(QStringLiteral("intensity")).toDouble(_intensity), _intensity);
    if (!qFuzzyCompare(_intensity + 1.0, next_intensity + 1.0)) {
        _intensity = next_intensity;
        changed = true;
    }

    const QJsonObject gaze = event.value(QStringLiteral("gaze")).toObject();
    const qreal next_gaze_x = boundedUnit(gaze.value(QStringLiteral("x")).toDouble(_gaze_x), _gaze_x);
    const qreal next_gaze_y = boundedUnit(gaze.value(QStringLiteral("y")).toDouble(_gaze_y), _gaze_y);
    if (!qFuzzyCompare(_gaze_x + 1.0, next_gaze_x + 1.0)) {
        _gaze_x = next_gaze_x;
        changed = true;
    }
    if (!qFuzzyCompare(_gaze_y + 1.0, next_gaze_y + 1.0)) {
        _gaze_y = next_gaze_y;
        changed = true;
    }

    const QJsonObject lip_sync = event.value(QStringLiteral("lip_sync")).toObject();
    const qreal next_lip = boundedUnit(lip_sync.value(QStringLiteral("value")).toDouble(_lip_sync_value), _lip_sync_value);
    if (!qFuzzyCompare(_lip_sync_value + 1.0, next_lip + 1.0)) {
        _lip_sync_value = next_lip;
        changed = true;
    }

    const QJsonObject speech = event.value(QStringLiteral("speech")).toObject();
    const QString delta = speech.value(QStringLiteral("text_delta")).toString();
    if (!delta.isEmpty()) {
        _speech_text += delta;
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
