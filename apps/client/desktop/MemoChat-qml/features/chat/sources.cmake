list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/chat/controller/ChatController.cpp
    features/chat/model/ChatMessageModelContent.cpp
    features/chat/model/ChatMessageModel.cpp
    features/chat/model/ChatMessageModelMutations.cpp
    features/chat/model/ChatMessageModelPresentation.cpp
    features/chat/model/ChatMessageModelQueries.cpp
    features/chat/services/ChatDispatchService.cpp
    features/chat/services/ConversationSyncService.cpp
    features/chat/services/DialogListEntryBuilder.cpp
    features/chat/services/DialogListService.cpp
    features/chat/services/MessageContentCodec.cpp
    features/chat/services/MessagePayloadService.cpp
    features/chat/cache/ChatCacheMessageCodec.cpp
    features/chat/cache/GroupChatCacheStore.cpp
    features/chat/cache/PrivateChatCacheStore.cpp
    features/chat/viewmodel/ChatViewModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/chat/controller/ChatController.h
    features/chat/model/ChatMessageModel.h
    features/chat/model/ChatUiState.h
    features/chat/services/ChatDispatchService.h
    features/chat/services/ConversationSyncService.h
    features/chat/services/DialogListEntryBuilder.h
    features/chat/services/DialogListService.h
    features/chat/services/DialogListTypes.h
    features/chat/services/MessageContentCodec.h
    features/chat/services/MessagePayloadService.h
    features/chat/cache/ChatCacheMessageCodec.h
    features/chat/cache/GroupChatCacheStore.h
    features/chat/cache/PrivateChatCacheStore.h
    features/chat/viewmodel/ChatViewModel.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/chat/resources/chat.qrc
)
