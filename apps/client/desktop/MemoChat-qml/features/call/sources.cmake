list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/call/controller/CallController.cpp
    features/call/controller/CallRequestPayloads.cpp
    features/call/model/CallSessionModel.cpp
    features/call/bridge/LivekitBridge.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/call/controller/CallController.h
    features/call/controller/CallRequestPayloads.h
    features/call/model/CallSessionModel.h
    features/call/bridge/LivekitBridge.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/call/resources/call.qrc
)
