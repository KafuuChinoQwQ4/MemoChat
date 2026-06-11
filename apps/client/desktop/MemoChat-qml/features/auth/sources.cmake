list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/auth/AuthController.cpp
    features/auth/service/AuthCredentialStore.cpp
    features/auth/service/AuthService.cpp
    features/auth/viewmodel/AuthViewModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/auth/AuthController.h
    features/auth/model/AuthState.h
    features/auth/service/AuthCredentialStore.h
    features/auth/service/AuthService.h
    features/auth/viewmodel/AuthViewModel.h
)

list(APPEND MEMOCHAT_QML_FEATURE_RESOURCES
    features/auth/resources/auth.qrc
)
