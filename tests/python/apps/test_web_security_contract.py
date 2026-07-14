import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
WEB_APP = REPO_ROOT / "apps/web"


def read(path):
    return path.read_text(encoding="utf-8")


class WebSecurityContractTests(unittest.TestCase):
    def test_typescript_sources_are_not_shadowed_by_emitted_javascript(self):
        emitted_javascript = sorted(path.relative_to(WEB_APP).as_posix() for path in (WEB_APP / "src").rglob("*.js"))

        self.assertEqual([], emitted_javascript)

        package_json = read(WEB_APP / "package.json")
        gitignore = read(WEB_APP / ".gitignore")
        self.assertIn('"build": "npm run type-check && vite build"', package_json)
        self.assertIn('"type-check": "tsc --noEmit && tsc --noEmit -p tsconfig.node.json"', package_json)
        self.assertIn("src/**/*.js", gitignore)

    def test_web_restores_via_http_only_cookie_without_javascript_token_storage(self):
        session_store = read(WEB_APP / "src/core/session/sessionStore.ts")
        session_types = read(WEB_APP / "src/core/session/sessionTypes.ts")
        bootstrap = read(WEB_APP / "src/app/bootstrap/postLoginBootstrap.ts")
        restore = read(WEB_APP / "src/app/bootstrap/sessionRestore.ts")
        http_client = read(WEB_APP / "src/core/network/http/HttpClient.ts")
        runtime_config = read(WEB_APP / "src/core/config/runtimeConfig.ts")

        self.assertFalse((WEB_APP / "src/core/session/tokenStorage.ts").exists())
        self.assertFalse((WEB_APP / "src/core/session/sessionPersistence.ts").exists())
        self.assertNotIn("refreshToken", session_store)
        self.assertNotIn("refreshToken", session_types)
        self.assertNotIn("localStorage", session_store + session_types + bootstrap + restore)
        self.assertNotIn("sessionStorage", session_store + session_types + bootstrap + restore)
        self.assertNotIn("res.refresh_token", bootstrap)
        self.assertIn("restoreSessionFromRefresh()", restore)
        self.assertIn('credentials: "include"', http_client)
        self.assertIn('gateBaseUrl: envString("VITE_GATE_BASE_URL")', runtime_config)
        self.assertIn('"X-MemoChat-Client": "web"', bootstrap)

    def test_websocket_endpoint_builder_forces_wss_for_non_local_hosts(self):
        source = read(WEB_APP / "src/app/bootstrap/postLoginBootstrap.ts")

        self.assertIn("export function buildEndpointUrl", source)
        self.assertIn("function resolveWsProtocol", source)
        self.assertIn("if (!isLocalhost) return 'wss'", source)
        self.assertIn("return serverTls ? 'wss' : 'ws'", source)
        self.assertIn("host === 'localhost'", source)
        self.assertIn("host === '127.0.0.1'", source)
        self.assertNotIn('endpoint.tls ? "wss" : "ws"', source)

    def test_csp_is_defined_in_index_and_vite_dev_preview_headers(self):
        index = read(WEB_APP / "index.html")
        vite = read(WEB_APP / "vite.config.ts")

        for token in (
            "Content-Security-Policy",
            "default-src 'self'",
            "frame-ancestors 'none'",
            "connect-src 'self' ws: wss: http: https:",
        ):
            with self.subTest(token=token):
                self.assertIn(token, index)
                self.assertIn(token, vite)

        self.assertIn("const securityHeaders", vite)
        self.assertIn("headers: securityHeaders", vite)
        self.assertIn("preview:", vite)
        self.assertNotIn("unsafe-eval", index + "\n" + vite)

    def test_r18_dialogs_and_narrow_layout_are_keyboard_accessible(self):
        component = read(WEB_APP / "src/features/r18/components/R18ShellContent.tsx")
        responsive_styles = WEB_APP / "src/features/r18/components/R18ShellContent.module.css"

        self.assertTrue(responsive_styles.exists())
        styles = read(responsive_styles)
        for token in (
            "function useDialogAccessibility",
            'event.key === "Escape"',
            'event.key !== "Tab"',
            "previousFocus",
            'aria-labelledby="r18-chapter-dialog-title"',
            'aria-labelledby="r18-reader-dialog-title"',
            "tabIndex={-1}",
            "styles.shell",
            "styles.sourceSidebar",
            "styles.sourceList",
            "styles.searchBar",
        ):
            with self.subTest(token=token):
                self.assertIn(token, component)

        self.assertIn("@media (max-width: 720px)", styles)
        self.assertIn("grid-template-columns: minmax(0, 1fr)", styles)
        self.assertIn("overflow-x: auto", styles)

    def test_application_shell_is_responsive_and_auth_brand_is_not_duplicated(self):
        shell = read(WEB_APP / "src/shared/ui/layout/ThreeColumnShell.tsx")
        shell_styles = read(WEB_APP / "src/shared/ui/layout/ThreeColumnShell.module.css")
        brand = WEB_APP / "src/shared/ui/primitives/BrandMark.tsx"

        self.assertTrue(brand.exists())
        self.assertIn("styles.shell", shell)
        self.assertIn("minmax(0, 1fr)", shell_styles)
        self.assertIn("@media (max-width: 900px)", shell_styles)
        self.assertIn("@media (max-width: 520px)", shell_styles)

        for page_name in ("LoginPage.tsx", "RegisterPage.tsx", "ResetPage.tsx"):
            page = read(WEB_APP / "src/features/auth/components" / page_name)
            with self.subTest(page=page_name):
                self.assertIn('import { BrandMark } from "@/shared/ui/primitives/BrandMark"', page)
                self.assertNotIn("function BrandMark()", page)


if __name__ == "__main__":
    unittest.main()
