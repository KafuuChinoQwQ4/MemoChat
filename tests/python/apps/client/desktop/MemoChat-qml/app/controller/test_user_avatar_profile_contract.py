import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CORE_CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core"
SHARED_CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared"
GATE_CORE = REPO_ROOT / "apps/server/core/GateServer/core"
GATE_H1 = REPO_ROOT / "apps/server/core/GateServer"
GATE_H2_SUPPORT = REPO_ROOT / "apps/server/core/GateServer/transports/h2/support"
GATE_H3_LEGACY_ROUTES = REPO_ROOT / "apps/server/core/GateServer/transports/h3/legacy_routes"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function body not found for {signature}")


class UserAvatarProfileContractTests(unittest.TestCase):
    def test_http_login_profile_icon_seeds_current_user_before_pet_window_sync(self):
        auth_session = read(CLIENT_QML / "app/session/SessionAuthCoordinatorLoginResponse.cpp")
        app_controller = read(CLIENT_QML / "app/controller/AppController.cpp")
        port_binder = read(CLIENT_QML / "app/composition/AppSessionAuthPortBinder.cpp")
        chat_entry = read(CLIENT_QML / "app/session/SessionChatEntryCoordinator.cpp")
        state = read(CLIENT_QML / "app/controller/AppControllerProfileState.cpp")
        profile_controller = read(CLIENT_QML / "features/profile/controller/ProfileController.cpp")

        login = extract_function(auth_session, "void SessionAuthCoordinator::onLoginHttpFinished")
        self.assertIn("_port.applyLoginSuccess(server_info, obj);", login)
        self.assertNotIn("_app.", login)

        self.assertIn("const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();", port_binder)
        self.assertIn("setIconDownloadAuthContext(pending.uid, pending.token);", port_binder)
        self.assertIn(
            'applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);', port_binder
        )
        self.assertNotIn(
            'applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);', app_controller
        )
        self.assertLess(
            port_binder.index('applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);'),
            port_binder.index("_gateway.chatTransport()->connectToServer(serverInfo);"),
        )

        switch = extract_function(chat_entry, "void SessionChatEntryCoordinator::onSwitchToChat")
        self.assertIn("_port.applyLoggedInUserSession(userInfo, snapshot.pendingToken);", switch)
        self.assertNotIn("_user_state.icon = icon;", switch)

        profile_helper = extract_function(
            state,
            "void AppController::applyCurrentUserProfile(int uid,",
        )
        self.assertIn("_features.profileController.applyCurrentUserProfile", profile_helper)
        self.assertNotIn("userInfo->_icon", profile_helper)
        self.assertNotIn("_user_state.icon = ", profile_helper)

        profile_apply = extract_function(
            profile_controller,
            "void ProfileController::applyCurrentUserProfile(int uid,",
        )
        self.assertIn("preserveExistingIcon && nextIcon == kDefaultIcon", profile_apply)
        self.assertIn("snapshot.icon != kDefaultIcon", profile_apply)
        self.assertIn("userInfo->_icon = nextIcon;", profile_apply)
        self.assertIn("_state_port.syncCurrentUser", profile_apply)

    def test_chat_login_response_does_not_replace_seeded_icon_with_empty_value(self):
        dispatcher = read(CORE_CLIENT / "network/ChatMessageDispatcherAuth.cpp")
        handler = dispatcher[dispatcher.index("_handlers.insert(ID_CHAT_LOGIN_RSP") :]
        handler = handler[: handler.index("emit sig_swich_chatdlg();")]

        self.assertIn("auto user_info = UserMgr::GetInstance()->GetUserInfo();", handler)
        self.assertIn('const QString responseIcon = jsonObj["icon"].toString();', handler)
        self.assertIn("if (!responseIcon.trimmed().isEmpty())", handler)
        self.assertIn("user_info->_icon = responseIcon;", handler)
        else_branch = handler[handler.index("else") :]
        self.assertNotIn("UserMgr::GetInstance()->SetUserInfo(user_info);", else_branch)

    def test_post_login_bootstrap_waits_for_authenticated_chat_transport(self):
        session = read(CLIENT_QML / "app/session/SessionChatEntryCoordinator.cpp")

        switch = extract_function(session, "void SessionChatEntryCoordinator::onSwitchToChat")
        self.assertIn("beginPostLoginBootstrap();", switch)
        self.assertLess(
            switch.index("_port.applyLoggedInUserSession(userInfo, snapshot.pendingToken);"),
            switch.index("beginPostLoginBootstrap();"),
        )

        bootstrap = extract_function(session, "void SessionChatEntryCoordinator::beginPostLoginBootstrap")
        self.assertIn("!snapshot.chatTransportReady", bootstrap)
        self.assertLess(
            bootstrap.index("!snapshot.chatTransportReady"),
            bootstrap.index("runPostLoginBootstrap();"),
        )

    def test_icon_normalizer_accepts_media_download_urls_and_raw_media_keys(self):
        icon_utils = read(SHARED_CLIENT / "utils/IconPathUtils.h")

        self.assertIn("inline bool looksLikeMediaKey", icon_utils)
        self.assertIn("inline QString mediaKeyDownloadUrl", icon_utils)
        self.assertIn('QStringLiteral("/media/download?asset=") + mediaKey', icon_utils)
        self.assertIn("return mediaKeyDownloadUrl(icon);", icon_utils)
        self.assertIn("return attachMediaDownloadAuth(withGateMediaUrlPrefix(icon));", icon_utils)
        self.assertIn("inline QString normalizeLocalMediaDownloadUrl", icon_utils)
        self.assertIn("normalized.setPort(parsedBase.port(-1));", icon_utils)
        self.assertNotIn("parsed.setPort(8080)", icon_utils)
        self.assertNotIn("port == 80 || port == 8443", icon_utils)

    def test_gate_login_cache_hit_refreshes_database_profile_before_ticket_issue(self):
        support_h = read(GATE_CORE / "support/AuthLoginSupport.h")
        support_cpp = read(GATE_CORE / "support/AuthLoginSupport.cpp")
        h1_auth_service = read(GATE_H1 / "services/auth/AuthService.cpp")
        h2 = read(GATE_H2_SUPPORT / "Http2AuthSupport.cpp")
        h3 = read(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")

        self.assertIn("bool RefreshLoginProfileFromDb(const std::string& email, UserInfo& userInfo);", support_h)
        refresh = extract_function(support_cpp, "bool RefreshLoginProfileFromDb")
        self.assertIn("PostgresMgr::GetInstance()->GetUserInfo(userInfo.uid, dbUserInfo)", refresh)
        self.assertIn("userInfo.icon = dbUserInfo.icon;", refresh)
        self.assertIn("CacheLoginProfile(email, userInfo);", refresh)

        for source in (h1_auth_service, h2):
            self.assertIn("gateauthsupport::RefreshLoginProfileFromDb(email, userInfo);", source)
        self.assertIn("AuthService::HandleUserLogin", h3)


if __name__ == "__main__":
    unittest.main()
