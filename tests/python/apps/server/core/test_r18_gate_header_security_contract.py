import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
R18_SERVICE = REPO_ROOT / "apps/server/core/R18Service/domain/services/r18"
GATE_HTTP_CONNECTION = REPO_ROOT / "apps/server/core/GateShared/HttpConnection.cpp"


def read(path):
    return path.read_text(encoding="utf-8")


class R18GateHeaderSecurityContractTests(unittest.TestCase):
    def test_r18_https_clients_verify_peer_certificates_and_hostnames(self):
        for path in (
            R18_SERVICE / "R18AdapterUtils.cpp",
            R18_SERVICE / "R18PicacgAdapter.cpp",
        ):
            with self.subTest(path=path):
                source = read(path)
                self.assertNotIn("ssl::verify_none", source)
                self.assertIn("ssl::verify_peer", source)
                self.assertIn("set_default_verify_paths", source)
                self.assertIn("ssl::host_name_verification", source)

    def test_r18_ecb_decrypt_is_limited_to_third_party_jm_compatibility(self):
        references = {
            path.relative_to(REPO_ROOT).as_posix()
            for path in R18_SERVICE.rglob("*")
            if path.is_file()
            and path.suffix in {".cpp", ".hpp", ".h", ".cppm"}
            and ("Aes256EcbDecrypt" in read(path) or "EVP_aes_256_ecb" in read(path))
        }

        self.assertEqual(
            references,
            {
                "apps/server/core/R18Service/domain/services/r18/R18AdapterUtils.cpp",
                "apps/server/core/R18Service/domain/services/r18/R18AdapterUtils.hpp",
                "apps/server/core/R18Service/domain/services/r18/R18JmAdapter.cpp",
            },
        )
        utils = read(R18_SERVICE / "R18AdapterUtils.cpp")
        self.assertIn("upstream JM/Picacg API mandates", utils)
        self.assertIn("key.size() != 32", utils)

    def test_gate_reflected_trace_headers_are_sanitized_before_response_use(self):
        source = read(GATE_HTTP_CONNECTION)

        self.assertIn("SanitizeHttpHeaderValueLocal", source)
        self.assertIn("ch == '\\r' || ch == '\\n'", source)
        self.assertIn("_trace_id = SanitizeHttpHeaderValueLocal", source)
        self.assertIn("_request_id = SanitizeHttpHeaderValueLocal", source)
        self.assertIn("const std::string cors_origin = AllowedCorsOriginForRequestLocal(_request);", source)
        self.assertIn('header << "X-Trace-Id: " << _trace_id << "\\r\\n";', source)
        self.assertIn('header << "X-Request-Id: " << _request_id << "\\r\\n";', source)


if __name__ == "__main__":
    unittest.main()
