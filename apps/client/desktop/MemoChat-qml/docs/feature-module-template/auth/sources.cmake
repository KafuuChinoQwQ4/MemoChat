# Template only. Do not include this file from the current build.

list(APPEND MEMOCHAT_QML_AUTH_SOURCES
    features/auth/viewmodel/AuthViewModel.cpp
    features/auth/service/AuthService.cpp
)

list(APPEND MEMOCHAT_QML_AUTH_HEADERS
    features/auth/viewmodel/AuthViewModel.h
    features/auth/model/AuthState.h
    features/auth/service/AuthService.h
)

list(APPEND MEMOCHAT_QML_AUTH_RESOURCES
    features/auth/resources/auth.qrc
)

