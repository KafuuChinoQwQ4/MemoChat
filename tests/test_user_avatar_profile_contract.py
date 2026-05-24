import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
SHARED_CLIENT = REPO_ROOT / "apps/client/desktop/MemoChatShared"
GATE_CORE = REPO_ROOT / "apps/server/core/GateServerCore"
GATE_H1 = REPO_ROOT / "apps/server/core/GateServer"
GATE_H2 = REPO_ROOT / "apps/server/core/GateServerHttp2"
GATE_H3 = REPO_ROOT / "apps/server/core/GateServerHttp3"


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
                return source[start:index + 1]
    raise AssertionError(f"Function body not found for {signature}")


class UserAvatarProfileContractTests(unittest.TestCase):
    def test_http_login_profile_icon_seeds_current_user_before_pet_window_sync(self):
        session = read(CLIENT_QML / "AppControllerSession.cpp")
        state = read(CLIENT_QML / "AppControllerState.cpp")

        login = extract_function(session, "void AppController::onLoginHttpFinished")
        self.assertIn("setIconDownloadAuthContext(_pending_uid, _pending_token);", login)
        self.assertIn(
            'applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);',
            login,
        )
        self.assertLess(
            login.index('applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);'),
            login.index("_gateway.chatTransport()->connectToServer(server_info);"),
        )

        switch = extract_function(session, "void AppController::onSwitchToChat")
        self.assertIn("applyCurrentUserProfile(user_info->_uid", switch)
        self.assertIn("user_info->_icon", switch)
        self.assertIn("true);", switch)
        self.assertNotIn("_current_user_icon = icon;", switch)

        profile_helper = extract_function(
            state,
            "void AppController::applyCurrentUserProfile(int uid, const QString &name, const QString &nick, const QString &icon,",
        )
        self.assertIn("preserveExistingIcon && nextIcon == kDefaultIcon", profile_helper)
        self.assertIn("_current_user_icon != kDefaultIcon", profile_helper)
        self.assertIn("userInfo->_icon = nextIcon;", profile_helper)
        self.assertIn("emit currentUserChanged();", profile_helper)

    def test_chat_login_response_does_not_replace_seeded_icon_with_empty_value(self):
        dispatcher = read(SHARED_CLIENT / "ChatMessageDispatcher.cpp")
        handler = dispatcher[dispatcher.index("_handlers.insert(ID_CHAT_LOGIN_RSP"):]
        handler = handler[:handler.index("_handlers.insert(ID_GET_RELATION_BOOTSTRAP_RSP")]

        self.assertIn("auto user_info = UserMgr::GetInstance()->GetUserInfo();", handler)
        self.assertIn('const QString responseIcon = jsonObj["icon"].toString();', handler)
        self.assertIn("if (!responseIcon.trimmed().isEmpty())", handler)
        self.assertIn("user_info->_icon = responseIcon;", handler)
        self.assertNotIn("UserMgr::GetInstance()->SetUserInfo(user_info);", handler.split("} else {", 1)[1])

    def test_post_login_bootstrap_waits_for_authenticated_chat_transport(self):
        session = read(CLIENT_QML / "AppControllerSession.cpp")

        switch = extract_function(session, "void AppController::onSwitchToChat")
        self.assertIn("beginPostLoginBootstrap();", switch)
        self.assertLess(
            switch.index("applyCurrentUserProfile(user_info->_uid"),
            switch.index("beginPostLoginBootstrap();"),
        )

        bootstrap = extract_function(session, "void AppController::beginPostLoginBootstrap")
        self.assertIn("!isChatTransportReady()", bootstrap)
        self.assertLess(
            bootstrap.index("!isChatTransportReady()"),
            bootstrap.index("runPostLoginBootstrap();"),
        )

    def test_icon_normalizer_accepts_media_download_urls_and_raw_media_keys(self):
        icon_utils = read(CLIENT_QML / "IconPathUtils.h")

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
        support_h = read(GATE_CORE / "AuthLoginSupport.h")
        support_cpp = read(GATE_CORE / "AuthLoginSupport.cpp")
        h1 = read(GATE_H1 / "GateServerH1Routes.cpp")
        h2 = read(GATE_H2 / "Http2AuthSupport.cpp")
        h3 = read(GATE_H3 / "GateHttp3ServiceRoutes.cpp")

        self.assertIn("bool RefreshLoginProfileFromDb(const std::string& email, UserInfo& userInfo);", support_h)
        refresh = extract_function(support_cpp, "bool RefreshLoginProfileFromDb")
        self.assertIn("PostgresMgr::GetInstance()->GetUserInfo(userInfo.uid, dbUserInfo)", refresh)
        self.assertIn("userInfo.icon = dbUserInfo.icon;", refresh)
        self.assertIn("CacheLoginProfile(email, userInfo);", refresh)

        for source in (h1, h2, h3):
            self.assertIn("gateauthsupport::RefreshLoginProfileFromDb(email, userInfo);", source)


if __name__ == "__main__":
    unittest.main()
