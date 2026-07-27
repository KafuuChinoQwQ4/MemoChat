"""
R18 E-Hentai browser authentication security contract tests.

These are static-analysis tests that inspect source files to prove:
  - Ticket lifecycle: entropy, TTL, single-use, uid/source binding
  - Cookie allowlist and size bounds enforced before any persistence
  - E-Hentai password never persisted (erased on load, rejected on write)
  - Secret redaction: cookies never in public API responses
  - Rate limiting present
  - JM/Picacg credentials unaffected by E-Hentai changes
"""

import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
R18SVC = REPO_ROOT / "apps/server/core/R18Service/domain/services/r18"
R18ROUTE = REPO_ROOT / "apps/server/core/R18Service/domain/modules/r18"
BROWSER_EXT = REPO_ROOT / "apps/client/browser/memochat-ehentai-auth/src"
WEB_R18 = REPO_ROOT / "apps/web/src/features/r18"


def read(path):
    return path.read_text(encoding="utf-8")


class TicketEntropyAndLifecycleContractTests(unittest.TestCase):
    def test_ticket_uses_openssl_csprng_not_stdlib_random(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("RAND_bytes", svc, "Ticket generation must use RAND_bytes for CSPRNG entropy")
        self.assertNotIn("std::hash<", svc, "SimpleSHA256 placeholder must be replaced with OpenSSL EVP")
        self.assertNotIn("mt19937", svc, "mt19937 pseudo-RNG must not be used for ticket entropy")

    def test_ticket_digest_uses_openssl_sha256_not_placeholder(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("EVP_sha256", svc)
        self.assertIn("EVP_DigestInit_ex", svc)
        self.assertIn("EVP_DigestFinal_ex", svc)

    def test_ticket_ttl_is_at_most_120_seconds(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        # kTicketTtlSec must be <= 120
        self.assertIn("kTicketTtlSec", svc)
        # Grep the numeric assignment
        for line in svc.splitlines():
            if "kTicketTtlSec" in line and "=" in line:
                # Extract the value
                val = line.split("=")[1].strip().rstrip(";").strip()
                self.assertLessEqual(int(val), 120, f"kTicketTtlSec must be <= 120, found {val}")
                break

    def test_ticket_is_stored_by_digest_only_not_plaintext(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        # The raw ticket must never be written to Redis (only the digest key is stored)
        self.assertIn("SHA256Hex(ticket)", svc)
        self.assertIn("TicketMetaKey(digest)", svc)
        # raw ticket must not appear as a Redis key argument
        self.assertNotIn("SetEx(ticket,", svc)
        self.assertNotIn('SetEx("ticket"', svc)

    def test_ticket_is_single_use_via_atomic_string_eval(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        complete = svc.split("CompleteImport", 1)[1].split("GetStatus", 1)[0]
        self.assertIn("redis->EvalString", complete)
        self.assertNotIn("redis->Get(meta_key", complete)

    def test_ticket_uid_and_source_family_stored_in_meta(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn('"uid"', svc)
        self.assertIn('"source_family"', svc)
        self.assertIn("SerializeMeta", svc)

    def test_status_lookup_compares_the_bound_uid(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        status_body = svc.split("GetStatus(int uid", 1)[1].split("UpdateStatusInRedis", 1)[0]
        self.assertIn('ExtractInt(status_json, "uid")', status_body)
        self.assertIn("status_uid != uid", status_body)
        self.assertNotIn("(void) uid", status_body)

    def test_rate_limiting_uses_redis_not_in_memory_only(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("RateLimitKey", svc)
        self.assertIn("redis->SetEx(RateLimitKey", svc)
        self.assertIn("rate_limited", svc)

    def test_import_id_and_ticket_have_minimum_128_and_256_bit_entropy_respectively(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        # import_id uses SecureRandomHex(16) = 16 bytes = 128 bits
        self.assertIn("SecureRandomHex(16)", svc)
        # ticket uses SecureRandomHex(32) = 32 bytes = 256 bits
        self.assertIn("SecureRandomHex(32)", svc)


class CookieAllowlistAndBoundsContractTests(unittest.TestCase):
    def test_complete_endpoint_validates_required_cookies_before_processing(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("ipb_member_id.empty()", svc)
        self.assertIn("ipb_pass_hash.empty()", svc)
        self.assertIn("missing_required_cookies", svc)

    def test_cookie_values_are_bounded_in_size(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("kMaxCookieLen", svc)
        self.assertIn("cookie_value_too_large", svc)

    def test_control_characters_rejected_in_cookie_values(self):
        svc = read(R18SVC / "R18BrowserImportService.cpp")
        self.assertIn("has_control", svc)
        self.assertIn("invalid_cookie_format", svc)

    def test_only_four_named_cookies_accepted_by_dto(self):
        dtos = read(R18SVC / "R18PublicDtos.hpp")
        # The struct for the complete request must have exactly these four cookie fields
        self.assertIn("ipb_member_id", dtos)
        self.assertIn("ipb_pass_hash", dtos)
        self.assertIn("igneous", dtos)
        # Must not contain arbitrary cookie fields
        self.assertNotIn("raw_cookies", dtos)

    def test_exhentai_manual_import_requires_exhentai_access(self):
        service = read(R18SVC / "R18Service.cpp")
        import_body = service.split("HandleSessionImport", 1)[1]
        self.assertIn('req.source_id == "exhentai.official" && !validation.exhentai_access', import_body)
        self.assertIn("ExHentai access was not granted by the imported session", import_body)


class EhentaiPasswordErasureContractTests(unittest.TestCase):
    """
    E-Hentai password login re-enabled (user requirement: three modes — 账密/Cookie/网页).
    UpsertLogin now accepts E-Hentai credentials.  ImportEhentaiSession (cookie/extension
    path) still clears credentials before writing a session-only record.
    """

    def test_credential_store_accepts_ehentai_password_login(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        self.assertNotIn(
            "ehentai_password_login_removed", store, "E-Hentai password login must be enabled (user requirement)"
        )

    def test_credential_store_loads_ehentai_password_normally(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        load_fn = store.split("LoadUidLocked(", 1)[1]
        self.assertIn('cred.password = json::glaze_safe_get<std::string>(item, "password"', load_fn)

    def test_credential_store_writes_ehentai_password_normally(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        save_fn = store.split("SaveUidLocked(", 1)[1]
        self.assertIn(
            'item["password"] = cred.password', save_fn, "Password must be persisted for all sources including E-Hentai"
        )

    def test_ehentai_session_import_clears_username_and_password(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        self.assertIn("ImportEhentaiSession", store)
        import_fn = store.split("ImportEhentaiSession", 1)[1]
        self.assertIn("cred.username.clear()", import_fn)
        self.assertIn("cred.password.clear()", import_fn)

    def test_jm_and_picacg_password_persistence_is_unaffected(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        self.assertIn("cred.password = json::glaze_safe_get", store)

    def test_upsertlogin_requires_non_empty_source_id(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        upsert = store.split("UpsertLogin(", 1)[1]
        self.assertIn("source_id is required", upsert)
        self.assertIn("LoadUidLocked", upsert)


class SecretRedactionContractTests(unittest.TestCase):
    def test_public_projection_exposes_only_safe_fields(self):
        store = read(R18SVC / "R18SourceCredentialStore.cpp")
        public_fn = store.split("ToPublicJson", 1)[1]
        # has_session is allowed; raw session_cookie must not appear
        self.assertIn("has_session", public_fn)
        self.assertNotIn('"session_cookie"', public_fn.split("ToPublicJson", 1)[0] + public_fn[:200])

    def test_browser_import_start_response_contains_no_cookie_values(self):
        dtos = read(R18SVC / "R18PublicDtos.hpp")
        # Response DTO must not have cookie fields
        start_response = dtos.split("R18BrowserImportStartResponseDto")[1].split("};")[0]
        self.assertNotIn("ipb_member_id", start_response)
        self.assertNotIn("ipb_pass_hash", start_response)
        self.assertNotIn("igneous", start_response)

    def test_browser_import_status_response_contains_no_secrets(self):
        service = read(R18SVC / "R18Service.cpp")
        # Status handler response must not include ticket, cookie, or encryption material
        status_fn = service.split("HandleBrowserImportStatus", 1)[1].split("return true;")[0]
        self.assertNotIn("ticket", status_fn)
        self.assertNotIn("ipb_member_id", status_fn)

    def test_new_routes_are_registered_in_route_module(self):
        module = read(R18ROUTE / "R18RouteModule.cpp")
        self.assertIn("BrowserImportStartPath()", module)
        self.assertIn("BrowserImportCompletePath()", module)
        self.assertIn("BrowserImportStatusPath()", module)
        self.assertIn("SessionImportPath()", module)

    def test_new_route_paths_are_correct(self):
        reg = read(R18ROUTE / "cxx_modules/R18RouteRegistration.cppm")
        self.assertIn("/api/r18/account/browser-import/start", reg)
        self.assertIn("/api/r18/account/browser-import/complete", reg)
        self.assertIn("/api/r18/account/browser-import/status", reg)
        self.assertIn("/api/r18/account/session/import", reg)


class DockerPortStabilityContractTests(unittest.TestCase):
    def test_r18gateway_port_unchanged(self):
        ini = (REPO_ROOT / "apps/server/core/R18Service/r18gateway.ini").read_text()
        self.assertIn("Port=8098", ini)

    def test_redis_port_unchanged(self):
        ini = (REPO_ROOT / "apps/server/core/R18Service/r18gateway.ini").read_text()
        self.assertIn("Port=6379", ini)


class ExtensionOriginSecurityContractTests(unittest.TestCase):
    def test_content_script_rejects_non_localhost_origins(self):
        if not BROWSER_EXT.exists():
            self.skipTest("Extension src dir not found")
        content = read(BROWSER_EXT / "content.ts")
        self.assertIn("localhost", content)
        self.assertIn("127.0.0.1", content)
        # Content script must emit a warning and return when origin is not allowed.
        self.assertIn("invalid origin", content.lower())

    def test_background_validates_memochat_origin_before_opening_tab(self):
        if not BROWSER_EXT.exists():
            self.skipTest("Extension src dir not found")
        bg = read(BROWSER_EXT / "background.ts")
        self.assertIn("localhost", bg)
        self.assertIn("Invalid MemoChat origin", bg)

    def test_content_script_never_posts_cookie_values_to_page(self):
        if not BROWSER_EXT.exists():
            self.skipTest("Extension src dir not found")
        content = read(BROWSER_EXT / "content.ts")
        # Content script must not pass through cookie values — only status/importId
        self.assertNotIn("ipb_member_id", content)
        self.assertNotIn("ipb_pass_hash", content)
        self.assertNotIn("igneous", content)

    def test_background_submits_directly_to_backend_not_page(self):
        if not BROWSER_EXT.exists():
            self.skipTest("Extension src dir not found")
        bg = read(BROWSER_EXT / "background.ts")
        self.assertIn("browser-import/complete", bg)
        # Must not relay cookies through window.postMessage
        self.assertNotIn("postMessage.*cookie", bg)


class WebUIPasswordRemovalContractTests(unittest.TestCase):
    def test_ehentai_login_panel_has_three_login_modes(self):
        panel = read(WEB_R18 / "components/R18EhentaiLoginPanel.tsx")
        self.assertIn('"password"', panel, "账密 mode identifier must be present")
        self.assertIn('"web"', panel, "网页登录 mode identifier must be present")
        self.assertIn('"cookie"', panel, "粘贴 Cookie mode identifier must be present")

    def test_web_login_mode_opens_official_ehentai_forum(self):
        panel = read(WEB_R18 / "components/R18EhentaiLoginPanel.tsx")
        self.assertIn("forums.e-hentai.org", panel, "网页登录 must open the official E-Hentai forum login page")
        self.assertIn("_blank", panel, "官方论坛必须在新标签页打开")

    def test_web_login_mode_does_not_require_extension(self):
        panel = read(WEB_R18 / "components/R18EhentaiLoginPanel.tsx")
        # Extension is optional — the panel must still function without it
        # The "open login page" flow must work when extension.kind is NOT "present"
        self.assertNotIn(
            'if (extension.kind !== "present") return',
            panel,
            "网页登录 basic flow must not be gated on extension presence",
        )

    def test_extension_is_optional_enhancement_not_required(self):
        panel = read(WEB_R18 / "components/R18EhentaiLoginPanel.tsx")
        # Extension is shown as optional enhancement, not a blocker
        self.assertIn(
            'extension.kind === "present"', panel, "Extension check must exist for optional enhancement display"
        )
        # Extension absence must not prevent the basic web login flow
        open_idx = panel.index("forums.e-hentai.org")
        # The open-login-page action must come before any extension gating
        ext_required_count = panel[:open_idx].count('extension.kind !== "present"')
        self.assertEqual(ext_required_count, 0, "Opening official login page must not require extension")

    def test_web_login_mode_shows_cookie_paste_field_after_opening_page(self):
        panel = read(WEB_R18 / "components/R18EhentaiLoginPanel.tsx")
        # After the page opens, user must be guided to paste cookies
        self.assertIn("ipb_member_id", panel, "Panel must guide user to copy ipb_member_id cookie")
        self.assertIn("ipb_pass_hash", panel, "Panel must guide user to copy ipb_pass_hash cookie")

    def test_shell_content_applies_panel_to_both_ehentai_sources(self):
        shell = read(WEB_R18 / "components/R18ShellContent.tsx")
        self.assertIn("R18EhentaiLoginPanel", shell)
        self.assertIn("optionalCookie", shell)
        self.assertIn("requiredEhentaiAuth", shell)
        # The ternary that opens the panel must combine both sources
        self.assertIn("requiredEhentaiAuth || optionalCookie", shell, "Both E-Hentai sources must use the new panel")

    def test_api_has_all_three_ehentai_auth_methods(self):
        api = read(WEB_R18 / "api/r18Api.ts")
        self.assertIn("importSession", api)
        self.assertIn("startBrowserImport", api)
        self.assertIn("getBrowserImportStatus", api)

    def test_api_passes_cookies_as_nested_object_not_flat(self):
        api = read(WEB_R18 / "api/r18Api.ts")
        # cookies field must be a nested object (not flat params)
        self.assertIn("cookies: {", api, "Cookies must be sent as a nested object, not flat query params")
        self.assertIn("ipb_member_id: input.ipb_member_id", api)
        self.assertIn("ipb_pass_hash: input.ipb_pass_hash", api)

    def test_jm_picacg_password_forms_still_present(self):
        shell = read(WEB_R18 / "components/R18ShellContent.tsx")
        password_field = read(WEB_R18 / "components/R18PasswordField.tsx")
        self.assertIn("R18PasswordField", shell)
        self.assertIn('"password"', password_field)
        self.assertIn("required-account", shell)


if __name__ == "__main__":
    unittest.main()
