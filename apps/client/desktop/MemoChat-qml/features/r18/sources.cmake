list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/r18/controller/R18Controller.cpp
    features/r18/controller/R18ControllerNetwork.cpp
    features/r18/controller/R18ControllerResponses.cpp
    features/r18/controller/R18ControllerSources.cpp
    features/r18/controller/R18ControllerState.cpp
    features/r18/model/R18ListModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/r18/controller/R18Controller.h
    features/r18/controller/R18ControllerPrivate.h
    features/r18/model/R18ListModel.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/r18/resources/r18.qrc
)
