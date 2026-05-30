#include "ChatMessageDispatcher.h"

#include <QDebug>

ChatMessageDispatcher::ChatMessageDispatcher(QObject* parent)
    : QObject(parent)
{
    initHandlers();
}

void ChatMessageDispatcher::initHandlers()
{
    registerAuthHandlers();
    registerContactHandlers();
    registerPrivateMessageHandlers();
    registerGroupHandlers();
    registerSystemHandlers();
}

void ChatMessageDispatcher::dispatchMessage(ReqId id, int len, const QByteArray& data)
{
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end())
    {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}
