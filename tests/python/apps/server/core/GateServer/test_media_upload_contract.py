import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_H3_LEGACY_ROUTES = SERVER_CORE / "GateShared" / "transports" / "h3" / "legacy_routes"
GATE_H2_HANDLERS = SERVER_CORE / "GateShared" / "transports" / "h2" / "handlers"


def read_text(path):
    return path.read_text(encoding="utf-8-sig")


def strip_comments(source: str) -> str:
    source = re.sub(r"//.*", "", source)
    return re.sub(r"/\*.*?\*/", "", source, flags=re.S)


def normalize_space(source: str) -> str:
    return re.sub(r"\s+", " ", source)


def extract_logic_route_block(source: str, method: str, route: str) -> str:
    source = strip_comments(source)
    match = re.search(rf'\blogic\.{method}\s*\(\s*"{re.escape(route)}"', source)
    if match is None:
        raise AssertionError(f"missing {method} route block for {route}")

    paren_start = source.find("(", match.start())
    depth = 0
    in_string = False
    escaped = False
    for index in range(paren_start, len(source)):
        char = source[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return source[match.start() : index + 1]

    raise AssertionError(f"unterminated {method} route block for {route}")


def extract_function_region(source: str, start_pattern: str, next_pattern: str | None = None) -> str:
    source = strip_comments(source)
    start = re.search(start_pattern, source, re.S)
    if start is None:
        raise AssertionError(f"missing function matching {start_pattern}")
    if next_pattern is None:
        return source[start.start() :]
    end = re.search(next_pattern, source[start.end() :], re.S)
    if end is None:
        raise AssertionError(f"missing next function matching {next_pattern}")
    return source[start.start() : start.end() + end.start()]


class MediaUploadContractTest(unittest.TestCase):
    def test_direct_gate_and_h1_raise_beast_request_body_limit_for_media_chunks(self):
        direct = read_text(SERVER_CORE / "GateShared" / "HttpConnection.cpp")
        h1 = read_text(SERVER_CORE / "GateShared" / "transports" / "h1" / "legacy_standalone" / "H1Connection.cpp")

        for source in (direct, h1):
            with self.subTest(source=source[:32]):
                self.assertIn("http::request_parser<http::dynamic_body>", source)
                self.assertIn("kMediaRequestBodyLimitBytes", source)
                self.assertIn("parser->body_limit(kMediaRequestBodyLimitBytes);", source)
                self.assertIn("self->_request = parser->release();", source)
                self.assertRegex(source, r"64ULL\s*\*\s*1024ULL\s*\*\s*1024ULL")

    def test_h1_media_routes_are_migrated_to_shared_support(self):
        source = read_text(
            SERVER_CORE / "GateShared" / "transports" / "h1" / "legacy_standalone" / "H1MediaService.cpp"
        )

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
        source = read_text(
            SERVER_CORE / "GateShared" / "transports" / "h1" / "legacy_standalone" / "H1MediaService.cpp"
        )
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
        header = read_text(SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.h")
        source = read_text(SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.cpp")

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
        source = read_text(SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.cpp")
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
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
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
        source = read_text(GATE_H2_HANDLERS / "Http2MediaHandlers.cpp")
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

    def test_h3_media_routes_dispatch_to_shared_registry_adapter(self):
        source = read_text(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")
        compact = normalize_space(strip_comments(source))

        self.assertIn("RegisterMedia(registry)", source)
        self.assertIn("H3RouteAdapter::Dispatch", source)
        for route in (
            "/upload_media_init",
            "/upload_media_chunk",
            "/upload_media_complete",
            "/upload_media",
            "/upload_media_status",
            "/media/download",
        ):
            with self.subTest(route=route):
                self.assertRegex(
                    compact,
                    rf'logic\.Reg(?:Post|Get)\s*\(\s*"{re.escape(route)}"\s*,\s*DispatchSharedRoute',
                )

        for forbidden in (
            "Http2MediaSupport",
            "MediaResponseJson",
            "HandleUploadMediaChunkBytes",
            "HeaderValue",
            "X-Upload-Id",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_h2_h3_media_download_file_responses_are_owned_by_adapters(self):
        source = read_text(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")
        h2_adapter = read_text(
            SERVER_CORE / "GateShared" / "transports" / "h2" / "adapters" / "h2" / "H2RouteAdapter.cpp"
        )
        h3_adapter = read_text(
            SERVER_CORE / "GateShared" / "transports" / "h3" / "adapters" / "h3" / "H3RouteAdapter.cpp"
        )

        self.assertNotIn("MediaResponseJson", source)
        self.assertNotIn("Http2MediaSupport", source)
        for adapter in (h2_adapter, h3_adapter):
            with self.subTest(adapter=adapter[:80]):
                self.assertIn("GateResponseBodyKind::File", adapter)
                self.assertIn("ReadFileBody", adapter)
                self.assertIn("route_response.file_path", adapter)
                self.assertIn("application/octet-stream", adapter)
                self.assertIn("404", adapter)
                self.assertNotIn("file response limitation", adapter)
                self.assertNotIn("not supported", adapter)

    def test_current_h1_media_upload_json_responses_remain_text_json(self):
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        handlers = (
            "HandleUploadMediaInit",
            "HandleUploadMediaChunk",
            "HandleUploadMediaStatus",
            "HandleUploadMediaComplete",
            "HandleUploadMediaSimple",
        )

        for handler in handlers:
            with self.subTest(handler=handler):
                block = extract_function_region(
                    source,
                    rf"bool\s+MediaService::{handler}\s*\(",
                    r"\nbool\s+MediaService::Handle",
                )
                self.assertIn("WriteJson(response, root)", block)
                self.assertIn("root", block)

        write_json = extract_function_region(
            source,
            r"void\s+WriteJson\s*\([^)]*GateResponse& response[^)]*JsonValue& root[^)]*\)",
            r"\nstd::string\s+LowercaseAscii",
        )
        self.assertIn('response.content_type = "text/json"', write_json)
        self.assertIn("response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline", write_json)
        self.assertIn("response.body = root.toStyledString()", write_json)

        init_block = extract_function_region(
            source,
            r"bool\s+MediaService::HandleUploadMediaInit\s*\(",
            r"\nbool\s+MediaService::HandleUploadMediaChunk",
        )
        self.assertIn("ErrorCodes::Error_Json", init_block)
        self.assertIn('"invalid json"', init_block)

        for handler, next_handler in (
            ("HandleUploadMediaComplete", "HandleUploadMediaSimple"),
            ("HandleUploadMediaSimple", "HandleMediaDownload"),
        ):
            with self.subTest(success_payload=handler):
                block = extract_function_region(
                    source,
                    rf"bool\s+MediaService::{handler}\s*\(",
                    rf"\nbool\s+MediaService::{next_handler}",
                )
                for token in (
                    'root["media_key"]',
                    'root["media_type"]',
                    'root["file_name"]',
                    'root["mime"]',
                    'root["size"]',
                    'root["url"]',
                ):
                    self.assertIn(token, block)

    def test_current_h1_media_download_documents_json_bytes_redirect_and_file_shapes(self):
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        block = extract_function_region(source, r"bool\s+MediaService::HandleMediaDownload\s*\(")
        compact = normalize_space(block)

        self.assertIn("WriteJson(response, root)", block)
        self.assertIn('root["message"] = "missing media key or auth params"', block)
        self.assertIn('root["message"] = "token invalid"', block)
        self.assertIn('root["message"] = "asset not found"', block)
        self.assertIn('root["message"] = "legacy file not found"', block)
        self.assertIn("root.toStyledString()", source)

        self.assertIn("storage.ReadObject", block)
        self.assertIn('content_type = ct_from_storage.empty() ? "application/octet-stream" : ct_from_storage', block)
        self.assertIn("response.body.assign(data.data(), data.size())", compact)

        self.assertIn("storage.ResolvePublicUrl", block)
        self.assertIn("response.status = 307", block)
        self.assertRegex(block, r'response\.headers\s*\[\s*"Location"\s*\]')
        self.assertIn('"text/plain"', block)
        self.assertIn('"redirecting to object storage"', block)

        self.assertIn("storage.ResolveReadPath", block)
        self.assertIn("ResolveLegacyMediaPathLocal", block)
        self.assertIn("GateResponseBodyKind::File", block)
        self.assertIn("response.file_path = full_path.string()", compact)

    def test_e4_raw_binary_chunk_headers_are_case_insensitive_in_media_service(self):
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        header_helper = extract_function_region(
            source,
            r"std::string\s+HeaderValue\s*\([^)]*GateRequest& request[^)]*const std::string& name[^)]*\)",
            r"\nstd::string\s+QueryValue",
        )
        chunk_block = extract_function_region(
            source,
            r"bool\s+MediaService::HandleUploadMediaChunk\s*\(",
            r"\nbool\s+MediaService::HandleUploadMediaStatus",
        )

        self.assertIn("request.headers", header_helper)
        self.assertIn("LowercaseAscii(key) == LowercaseAscii(name)", normalize_space(header_helper))
        self.assertRegex(source, r"\b(ToLower|EqualsIgnoreCase|HeaderValue|FindHeaderCaseInsensitive)\b")
        for header in ("Content-Type", "X-Uid", "X-Token", "X-Upload-Id", "X-Chunk-Index"):
            with self.subTest(header=header):
                self.assertIn(f'"{header}"', chunk_block)
        self.assertNotRegex(
            chunk_block,
            r'request\.headers\.(?:find|at)\s*\(\s*"Content-Type"\s*\)',
            "raw chunk content type must not depend on exact-case map lookup",
        )

    def test_e4_upload_media_status_appends_uploaded_chunks_to_response_member(self):
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        block = extract_function_region(
            source,
            r"bool\s+MediaService::HandleUploadMediaStatus\s*\(",
            r"\nbool\s+MediaService::HandleUploadMediaComplete",
        )

        self.assertNotIn('auto uploaded_chunks_arr = root["uploaded_chunks"]', block)
        self.assertRegex(
            normalize_space(block),
            r'auto&\s+uploaded_chunks_arr\s*=\s*root\["uploaded_chunks"\]\s*=\s*memochat::json::array_t\{\}',
        )
        self.assertIn("memochat::json::glaze_array_append(uploaded_chunks_arr, idx)", block)

    def test_e4_local_file_download_contract_uses_gate_response_file_body_kind(self):
        response_header = strip_comments(read_text(SERVER_CORE / "GateShared" / "routing" / "GateResponse.h"))
        h1_adapter_source = strip_comments(
            read_text(SERVER_CORE / "GateShared" / "adapters" / "h1" / "H1RouteAdapter.cpp")
        )
        compact_header = normalize_space(response_header)
        compact_h1_adapter = normalize_space(h1_adapter_source)

        self.assertIn("enum class GateResponseBodyKind", response_header)
        self.assertIn("Inline", response_header)
        self.assertIn("File", response_header)
        self.assertIn("GateResponseBodyKind body_kind = GateResponseBodyKind::Inline", compact_header)
        self.assertIn("std::string file_path", response_header)
        self.assertIn(
            "route_response.body_kind == memochat::gate::routing::GateResponseBodyKind::File",
            compact_h1_adapter,
        )
        self.assertIn(
            "connection->SetFileResponse(route_response.file_path, route_response.content_type)",
            compact_h1_adapter,
        )

        media_source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        download_block = extract_function_region(media_source, r"bool\s+MediaService::HandleMediaDownload\s*\(")
        self.assertIn(
            "response.body_kind = memochat::gate::routing::GateResponseBodyKind::File", normalize_space(download_block)
        )
        self.assertIn("response.file_path = full_path.string()", normalize_space(download_block))


if __name__ == "__main__":
    unittest.main()
