# Settings Feature Boundary

`features/settings` is a UI-only feature for the chat shell settings and "more" surfaces.

It owns QML composition and resources:

- `view/MorePane.qml`
- `view/SettingsPane.qml`
- `view/SettingsAvatarCard.qml`
- `view/SettingsProfileForm.qml`
- `resources/settings.qrc`

It intentionally does not own a C++ controller or viewmodel. Profile-related actions from `SettingsPane` are exposed as QML signals and delegated by the shell to `ProfileController`. Account switching remains shell/session coordination.
