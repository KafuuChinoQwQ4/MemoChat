# Auth Feature Template

This is a sample feature-first module layout for the login/register/reset flow.

```text
auth/
  view/
    LoginPage.qml
    RegisterPage.qml
    ResetPage.qml
  viewmodel/
    AuthViewModel.h
    AuthViewModel.cpp
  model/
    AuthState.h
  service/
    AuthService.h
    AuthService.cpp
  resources/
    auth.qrc
  sources.cmake
```

Expected read path:

1. `view/LoginPage.qml`: what the user sees and which signals are emitted.
2. `viewmodel/AuthViewModel.h`: QML-facing properties and commands.
3. `service/AuthService.h`: network/business boundary.
4. `model/AuthState.h`: state fields shared by viewmodel and service.
5. `sources.cmake` and `resources/auth.qrc`: how the module joins CMake and QRC.

During real migration, keep the old `controller` API as a compatibility facade until all QML call sites have moved to an `auth` context object or `controller.auth`.

