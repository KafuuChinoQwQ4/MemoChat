#include "AgentController.h"

#include "AgentGameClient.h"
#include "ClientGateway.h"
#include "usermgr.h"

#include <QJsonObject>
#include <QUrl>

int AgentController::currentUid() const
{
    return _gateway && _gateway->userMgr() ? _gateway->userMgr()->GetUid() : 0;
}

void AgentController::setGameBusy(bool busy, const QString& statusText)
{
    if (_game_busy == busy && _game_status_text == statusText)
    {
        return;
    }
    _game_busy = busy;
    _game_status_text = statusText;
    emit gameStateChanged();
}

void AgentController::setGameError(const QString& error)
{
    if (_game_error == error)
    {
        return;
    }
    _game_error = error;
    emit gameStateChanged();
}

void AgentController::clearGameError()
{
    if (_game_error.isEmpty())
    {
        return;
    }
    _game_error.clear();
    emit gameStateChanged();
}

void AgentController::sendGameGet(const QUrl& url, const QString& op, const QString& statusText)
{
    const int uid = scopedUid();
    clearGameError();
    setGameBusy(true, statusText);
    _gameClient->get(url, op, statusText, uid);
}

void AgentController::sendGamePost(const QUrl& url,
                                   const QJsonObject& payload,
                                   const QString& op,
                                   const QString& statusText)
{
    const int uid = scopedUid();
    clearGameError();
    setGameBusy(true, statusText);
    _gameClient->post(url, payload, op, statusText, uid);
}

void AgentController::sendGameDelete(const QUrl& url, const QString& op, const QString& statusText)
{
    const int uid = scopedUid();
    clearGameError();
    setGameBusy(true, statusText);
    _gameClient->deleteResource(url, op, statusText, uid);
}

void AgentController::handleGameNetworkError(const QString& op, const QString& errorText, int uid)
{
    if (uid != 0 && uid != currentUid())
    {
        return;
    }
    if (op == QStringLiteral("delete_room"))
    {
        _pendingDeleteGameRoomId.clear();
    }
    setGameBusy(false, QStringLiteral("请求失败"));
    setGameError(QStringLiteral("Game 服务请求失败: %1").arg(errorText));
}

void AgentController::handleGameFormatError(const QString& op, int uid)
{
    if (uid != 0 && uid != currentUid())
    {
        return;
    }
    if (op == QStringLiteral("delete_room"))
    {
        _pendingDeleteGameRoomId.clear();
    }
    setGameBusy(false, QStringLiteral("响应格式错误"));
    setGameError(QStringLiteral("Game 服务响应格式错误"));
}
