#ifndef GROUPMANAGEMENTEFFECTAPPLIER_H
#define GROUPMANAGEMENTEFFECTAPPLIER_H

#include "GroupManagementEffectPorts.h"
#include "GroupManagementEventService.h"
#include "GroupManagementResponseService.h"

namespace memochat::group
{

class GroupManagementEffectApplier
{
public:
    static void apply(const GroupManagementEffect& effect, const GroupManagementEventEffectPort& port);
    static void apply(const GroupManagementResponseEffect& effect, const GroupManagementResponseEffectPort& port);
};

} // namespace memochat::group

#endif // GROUPMANAGEMENTEFFECTAPPLIER_H
