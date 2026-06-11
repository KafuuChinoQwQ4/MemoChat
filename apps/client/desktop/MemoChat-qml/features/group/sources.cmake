list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/group/controller/GroupController.cpp
    features/group/controller/GroupManagementEffectApplier.cpp
    features/group/controller/GroupManagementEventService.cpp
    features/group/controller/GroupManagementResponseService.cpp
    features/group/controller/GroupResponseErrorService.cpp
    features/group/controller/GroupRequestPayloads.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/group/controller/GroupController.h
    features/group/controller/GroupManagementEffectApplier.h
    features/group/controller/GroupManagementEffectPorts.h
    features/group/controller/GroupManagementEventService.h
    features/group/controller/GroupManagementResponseService.h
    features/group/controller/GroupResponseErrorService.h
    features/group/controller/GroupRequestPayloads.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/group/resources/group.qrc
)
