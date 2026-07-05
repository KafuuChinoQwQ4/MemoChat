import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
WEB_APP = REPO_ROOT / "apps/web"


def read(path):
    return path.read_text(encoding="utf-8")


class WebSecurityContractTests(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
