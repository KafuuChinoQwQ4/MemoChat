import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"


def read_text(path):
    return path.read_text(encoding="utf-8-sig")


class MediaUploadContractTest(unittest.TestCase):
    def test_direct_gate_and_h1_raise_beast_request_body_limit_for_media_chunks(self):
        direct = read_text(SERVER_CORE / "GateServer" / "HttpConnection.cpp")
        h1 = read_text(SERVER_CORE / "GateServerHttp1.1" / "H1Connection.cpp")

        for source in (direct, h1):
            with self.subTest(source=source[:32]):
                self.assertIn("http::request_parser<http::dynamic_body>", source)
                self.assertIn("kMediaRequestBodyLimitBytes", source)
                self.assertIn("parser->body_limit(kMediaRequestBodyLimitBytes);", source)
                self.assertIn("self->_request = parser->release();", source)
                self.assertRegex(source, r"64ULL\s*\*\s*1024ULL\s*\*\s*1024ULL")

    def test_h1_media_routes_are_migrated_to_shared_support(self):
        source = read_text(SERVER_CORE / "GateServerHttp1.1" / "H1MediaService.cpp")

        self.assertNotIn("MakeMediaNotImplementedResponse", source)
        self.assertNotIn("not migrated", source)
        for route in (
            "/upload_media_init",
            "/upload_media_chunk",
            "/upload_media_status",
            "/upload_media_complete",
            "/upload_media",
            "/media/download",
        ):
            self.assertIn(f'"{route}"', source)

        self.assertIn("Http2MediaSupport::HandleUploadMediaInit", source)
        self.assertIn("Http2MediaSupport::HandleUploadMediaChunkBytes", source)
        self.assertIn("Http2MediaSupport::HandleUploadMediaComplete", source)
        self.assertIn("Http2MediaSupport::HandleMediaDownloadInfo", source)
        self.assertIn("connection->SetFileResponse", source)

    def test_h1_media_response_starts_from_object_not_result_data_variant(self):
        source = read_text(SERVER_CORE / "GateServerHttp1.1" / "H1MediaService.cpp")
        response_func = re.search(
            r"memochat::json::JsonValue MakeMediaResponse\(const Http2MediaSupport::MediaResult& result\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(response_func)
        body = response_func.group("body")

        self.assertIn("memochat::json::glaze_empty_object()", body)
        self.assertIn("memochat::json::glaze_is_object(result.data)", body)
        self.assertIn("memochat::json::getMemberNames(result.data)", body)
        self.assertNotIn("JsonValue root = result.data", body)

    def test_shared_media_support_has_raw_binary_chunk_helper(self):
        header = read_text(SERVER_CORE / "GateServerCore" / "Http2MediaSupport.h")
        source = read_text(SERVER_CORE / "GateServerCore" / "Http2MediaSupport.cpp")

        self.assertIn('const std::string provider = media["StorageProvider"];', source)
        self.assertIn("cfg.storage_provider = provider;", source)
        self.assertIn("HandleUploadMediaChunkBytes", header)
        self.assertIn("std::string_view chunk_data", header)
        self.assertRegex(source, r"HandleUploadMediaChunkBytes\s*\([^)]*std::string_view chunk_data")
        self.assertIn("return HandleUploadMediaChunkBytes(uid, token, upload_id, index, binary);", source)
        bytes_body = re.search(
            r"MediaResult HandleUploadMediaChunkBytes\s*\([^)]*std::string_view chunk_data\)(?P<body>.*?)"
            r"\nMediaResult HandleUploadMediaStatus",
            source,
            re.S,
        )
        self.assertIsNotNone(bytes_body)
        self.assertIn("ofs.write(chunk_data.data()", bytes_body.group("body"))
        self.assertNotIn("DecodeBase64Local", bytes_body.group("body"))

    def test_shared_media_support_persists_upload_session_as_object(self):
        source = read_text(SERVER_CORE / "GateServerCore" / "Http2MediaSupport.cpp")
        save_func = re.search(
            r"bool SaveJsonFileLocal\(const std::filesystem::path& path, const memochat::json::JsonValue& root\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        load_func = re.search(
            r"bool LoadJsonFileLocal\(const std::filesystem::path& path, memochat::json::JsonValue& root\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(save_func)
        self.assertIsNotNone(load_func)
        self.assertIn("memochat::json::writeString(root)", save_func.group("body"))
        self.assertNotIn("glaze_stringify(root)", save_func.group("body"))
        self.assertIn("root.isString()", load_func.group("body"))
        self.assertIn("root.asString()", load_func.group("body"))
        self.assertIn("root = decoded", load_func.group("body"))

    def test_direct_gate_media_service_persists_upload_session_as_object(self):
        source = read_text(SERVER_CORE / "GateServer" / "MediaHttpService.cpp")
        save_func = re.search(
            r"bool SaveJsonFileLocal\(const std::filesystem::path& path, const memochat::json::JsonValue& root\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        load_func = re.search(
            r"bool LoadJsonFileLocal\(const std::filesystem::path& path, memochat::json::JsonValue& root\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(save_func)
        self.assertIsNotNone(load_func)
        self.assertIn("memochat::json::writeString(root)", save_func.group("body"))
        self.assertNotIn("glz::write_json", save_func.group("body"))
        self.assertIn("root.isString()", load_func.group("body"))
        self.assertIn("root.asString()", load_func.group("body"))
        self.assertIn("root = decoded", load_func.group("body"))

    def test_h2_raw_binary_chunk_route_uses_bytes_helper(self):
        source = read_text(SERVER_CORE / "GateServerHttp2" / "Http2MediaHandlers.cpp")
        func = re.search(
            r"void Http2MediaHandlers::HandleUploadMediaChunk\(.*?\n\}(?=\n\nvoid Http2MediaHandlers::HandleUploadMediaStatus)",
            source,
            re.S,
        )
        self.assertIsNotNone(func)
        body = func.group(0)

        self.assertIn("json_chunk", body)
        self.assertIn("HandleUploadMediaChunk(", body)
        self.assertIn("HandleUploadMediaChunkBytes", body)
        self.assertNotIn("chunk_data_base64 = body_str", body)

    def test_h3_raw_binary_chunk_route_uses_bytes_helper(self):
        source = read_text(SERVER_CORE / "GateServerHttp3" / "GateHttp3ServiceRoutes.cpp")
        chunk_route = re.search(
            r'logic\.RegPost\(\s*"/upload_media_chunk".*?logic\.RegPost\(\s*"/upload_media_complete"',
            source,
            re.S,
        )
        self.assertIsNotNone(chunk_route)
        body = chunk_route.group(0)

        self.assertIn("GetRequestBody()", body)
        self.assertIn("GetRequestHeaders()", body)
        self.assertIn('HeaderValue(headers, "X-Upload-Id")', body)
        self.assertIn("HandleUploadMediaChunkBytes", body)
        self.assertIn("HandleUploadMediaChunk(uid, token, upload_id, index, chunk_data_base64)", body)

    def test_h3_media_response_starts_from_object_not_result_data_variant(self):
        source = read_text(SERVER_CORE / "GateServerHttp3" / "GateHttp3ServiceRoutes.cpp")
        response_func = re.search(
            r"memochat::json::JsonValue MediaResponseJson\(const Http2MediaSupport::MediaResult& result\)"
            r"(?P<body>.*?)\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(response_func)
        body = response_func.group("body")

        self.assertIn("memochat::json::glaze_empty_object()", body)
        self.assertIn("memochat::json::glaze_is_object(result.data)", body)
        self.assertIn("memochat::json::getMemberNames(result.data)", body)
        self.assertNotIn("JsonValue root = result.data", body)

        media_block = re.search(
            r"// Media upload routes(?P<body>.*?)// User profile routes",
            source,
            re.S,
        )
        self.assertIsNotNone(media_block)
        self.assertNotIn("root = result.data", media_block.group("body"))
        self.assertNotIn("JsonValue root = result.data", media_block.group("body"))


if __name__ == "__main__":
    unittest.main()
