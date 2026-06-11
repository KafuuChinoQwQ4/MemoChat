#ifndef GROUPRESPONSEERRORSERVICE_H
#define GROUPRESPONSEERRORSERVICE_H

#include "GroupManagementResponseService.h"

#include <QJsonObject>
#include <QString>

#include <functional>

namespace memochat::group
{

enum class GroupResponseErrorTarget
{
    Ignore,
    Tip,
    GroupStatus,
    ManagementEffect
};

struct GroupResponseErrorDecision
{
    GroupResponseErrorTarget target = GroupResponseErrorTarget::Ignore;
    QString statusText;
    bool statusIsError = false;
    GroupManagementResponseEffect managementEffect;
    bool handled = false;
};

class GroupResponseErrorService
{
public:
    static GroupResponseErrorDecision
    reduce(int reqId, int error, const QJsonObject& payload, const std::function<QString(int)>& actionText = {});
};

} // namespace memochat::group

#endif // GROUPRESPONSEERRORSERVICE_H
