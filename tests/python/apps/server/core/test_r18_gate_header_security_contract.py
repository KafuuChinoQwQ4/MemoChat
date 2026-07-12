import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
R18_SERVICE = REPO_ROOT / "apps/server/core/R18Service/domain/services/r18"
R18_GATEWAY_INI = REPO_ROOT / "apps/server/core/R18Service/r18gateway.ini"
GATE_HTTP_CONNECTION = REPO_ROOT / "apps/server/core/GateShared/HttpConnection.cpp"
R18_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/r18"


def read(path):
    return path.read_text(encoding="utf-8")


class R18GateHeaderSecurityContractTests(unittest.TestCase):
    def test_r18_global_source_mutations_require_fail_closed_admin_auth(self):
        service = read(R18_SERVICE / "R18Service.cpp")
        config = read(R18_GATEWAY_INI)

        self.assertIn("RequireSourceAdmin", service)
        self.assertEqual(service.count("HandleAdminJsonRequest("), 5)
        self.assertIn("[R18SourceAdmin]", config)
        self.assertIn("AuthHeader=X-MemoChat-R18-Source-Admin-Key", config)
        self.assertIn("AdminKey=", config)

    def test_r18_public_source_payload_uses_redacted_record_codec(self):
        service = read(R18_SERVICE / "R18Service.cpp")
        source_service = read(R18_SERVICE / "R18SourceService.cpp")

        self.assertIn("R18SourceRecordToPublicJsonValue(rec)", service)
        self.assertIn("R18SourceRecordToPublicJsonValue(it->second)", source_service)
        self.assertIn("R18SourceRecordToPublicJsonValue(source)", source_service)

    def test_picacg_image_fetch_validates_origin_addresses_and_payload_before_network_or_cache(self):
        picacg = read(R18_SERVICE / "R18PicacgAdapter.cpp")

        self.assertIn("ValidatePicacgImageUrl", picacg)
        self.assertIn("ResolvePublicImageEndpoints", picacg)
        self.assertIn("IsPublicIpv4Address", picacg)
        self.assertIn("IsPublicIpv6Address", picacg)
        self.assertIn("parser.body_limit", picacg)
        self.assertIn("IsAllowedImageContentType", picacg)
        self.assertLess(picacg.index("ValidatePicacgImageUrl"), picacg.index("ReadCachedImage"))
        fetch = picacg.split("R18ImagePayload PicacgFetchImage", 1)[1]
        self.assertLess(fetch.index("ResolvePublicImageEndpoints"), fetch.index("HttpGetPinned"))

    def test_standard_qml_client_contains_no_global_source_mutation_route(self):
        source = "\n".join(
            read(path) for path in R18_QML.rglob("*") if path.is_file() and path.suffix in {".cpp", ".h", ".qml"}
        )
        for route in (
            "/api/r18/source/import",
            "/api/r18/source/enable",
            "/api/r18/source/disable",
            "/api/r18/source/delete",
        ):
            self.assertNotIn(route, source)

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
