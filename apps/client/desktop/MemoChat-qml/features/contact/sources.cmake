list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/contact/controller/ContactController.cpp
    features/contact/model/ApplyRequestModel.cpp
    features/contact/model/FriendListModel.cpp
    features/contact/model/FriendListModelMutations.cpp
    features/contact/model/FriendListModelState.cpp
    features/contact/model/FriendListModelQueries.cpp
    features/contact/model/SearchResultModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/contact/controller/ContactController.h
    features/contact/model/ApplyRequestModel.h
    features/contact/model/FriendListModel.h
    features/contact/model/SearchResultModel.h
)
