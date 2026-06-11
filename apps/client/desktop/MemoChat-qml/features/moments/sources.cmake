list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/moments/controller/MomentsController.cpp
    features/moments/controller/MomentsControllerPublish.cpp
    features/moments/controller/MomentsControllerRequests.cpp
    features/moments/controller/MomentsControllerResponses.cpp
    features/moments/controller/MomentsControllerFeedResponses.cpp
    features/moments/controller/MomentsControllerPostResponses.cpp
    features/moments/controller/MomentsControllerCommentResponses.cpp
    features/moments/parsing/MomentsControllerParsing.cpp
    features/moments/parsing/MomentsEntryParser.cpp
    features/moments/parsing/MomentsPublishPayload.cpp
    features/moments/parsing/MomentsResponseParser.cpp
    features/moments/model/MomentsModel.cpp
    features/moments/model/MomentsModelMutations.cpp
    features/moments/model/MomentsModelQueries.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/moments/controller/MomentsController.h
    features/moments/parsing/MomentsEntryParser.h
    features/moments/parsing/MomentsPublishPayload.h
    features/moments/parsing/MomentsResponseParser.h
    features/moments/model/MomentsModel.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/moments/resources/moments.qrc
)
