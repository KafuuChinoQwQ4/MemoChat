list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/contact/controller/ContactController.cpp
    features/contact/controller/ContactEventService.cpp
    features/contact/controller/ContactRequestPayloads.cpp
    features/contact/model/ApplyRequestModel.cpp
    features/contact/model/FriendListModel.cpp
    features/contact/model/FriendListModelMutations.cpp
    features/contact/model/FriendListModelState.cpp
    features/contact/model/FriendListModelQueries.cpp
    features/contact/model/SearchResultModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/contact/controller/ContactController.h
    features/contact/controller/ContactEventService.h
    features/contact/controller/ContactRequestPayloads.h
    features/contact/model/ApplyRequestModel.h
    features/contact/model/FriendListModel.h
    features/contact/model/SearchResultModel.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/contact/resources/contact.qrc
)
