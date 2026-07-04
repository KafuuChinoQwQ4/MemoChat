import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_H3_LEGACY_ROUTES = SERVER_CORE / "GateShared" / "transports" / "h3" / "legacy_routes"
CLIENT_QML = REPO_ROOT / "apps" / "client" / "desktop" / "MemoChat-qml"


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
    def test_direct_gate_raises_beast_request_body_limit_for_media_chunks(self):
        source = read_text(SERVER_CORE / "GateShared" / "HttpConnection.cpp")

        self.assertIn("http::request_parser<http::dynamic_body>", source)
        self.assertIn("kMediaRequestBodyLimitBytes", source)
        self.assertIn("parser->body_limit(kMediaRequestBodyLimitBytes);", source)
        self.assertIn("self->_request = parser->release();", source)
        self.assertRegex(source, r"64ULL\s*\*\s*1024ULL\s*\*\s*1024ULL")

    def test_shared_media_support_has_raw_binary_chunk_helper(self):
        header = read_text(SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.hpp")
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

    def test_h3_media_routes_dispatch_to_shared_registry_adapter(self):
        source = read_text(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")
        legacy_routes = read_text(GATE_H3_LEGACY_ROUTES / "cxx_modules" / "H3LegacyRoute.cppm")
        compact = normalize_space(strip_comments(source))

        self.assertIn("RegisterMedia(registry)", source)
        self.assertIn("H3RouteAdapter::Dispatch", source)
        self.assertIn("logic.RegGet(h3_legacy_modules::GetRoutePathAt(index), DispatchSharedRoute)", source)
        self.assertIn("logic.RegPost(h3_legacy_modules::PostRoutePathAt(index), DispatchSharedRoute)", source)
        self.assertIn("h3_legacy_modules::GetRouteCount()", source)
        self.assertIn("h3_legacy_modules::PostRouteCount()", source)
        for route in (
            "/upload_media_init",
            "/upload_media_chunk",
            "/upload_media_complete",
            "/upload_media",
            "/upload_media_status",
            "/media/download",
        ):
            with self.subTest(route=route):
                self.assertIn(f'return "{route}"', legacy_routes)

        for forbidden in (
            "Http2MediaSupport",
            "MediaResponseJson",
            "HandleUploadMediaChunkBytes",
            "HeaderValue",
            "X-Upload-Id",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_h3_media_download_file_responses_are_owned_by_adapter(self):
        source = read_text(GATE_H3_LEGACY_ROUTES / "GateHttp3ServiceRoutes.cpp")
        h3_adapter = read_text(
            SERVER_CORE / "GateShared" / "transports" / "h3" / "adapters" / "h3" / "H3RouteAdapter.cpp"
        )

        self.assertNotIn("MediaResponseJson", source)
        self.assertNotIn("Http2MediaSupport", source)
        self.assertIn("GateResponseBodyKind::File", h3_adapter)
        self.assertIn("ReadFileBody", h3_adapter)
        self.assertIn("route_response.file_path", h3_adapter)
        self.assertIn("adapter_modules::DefaultFileContentType()", h3_adapter)
        self.assertIn("adapter_modules::FileBodyNotFoundStatus()", h3_adapter)
        self.assertNotIn("file response limitation", h3_adapter)
        self.assertNotIn("not supported", h3_adapter)

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
        self.assertIn("response.content_type = service_modules::JsonContentType()", write_json)
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
                    "MediaUploadAssetResponseDto response_dto",
                    "response_dto.media_key",
                    "response_dto.media_type",
                    "response_dto.file_name",
                    "response_dto.mime",
                    "response_dto.size",
                    "response_dto.url",
                    "MediaUploadAssetResponseToJsonValue(response_dto)",
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
        self.assertIn("LegacyFileDownloadDisabledMessage()", block)
        self.assertIn("root.toStyledString()", source)
        self.assertIn("MediaPersistence::Instance().CanReadAsset(asset, uid)", block)
        self.assertIn('root["message"] = "media access denied"', block)
        self.assertIn("media.download.access_denied", block)
        self.assertNotIn("media.download.cross_owner", block)

        self.assertIn("storage.ReadObject", block)
        self.assertIn("ResolveDownloadContentTypeLocal(ct_from_storage", block)
        self.assertIn("response.body.assign(data.data(), data.size())", compact)

        self.assertIn("storage.ResolvePublicUrl", block)
        self.assertIn("response.status = service_modules::RedirectHttpStatus()", block)
        self.assertRegex(block, r'response\.headers\s*\[\s*"Location"\s*\]')
        self.assertIn("service_modules::PlainTextContentType()", block)
        self.assertIn("service_modules::RedirectBody()", block)

        self.assertIn("storage.ResolveReadPath", block)
        self.assertIn("FileNotFoundMessage()", block)
        self.assertNotIn('root["message"] = storage_err', block)
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
        self.assertIn("MediaUploadStatusResponseDto response_dto", block)
        self.assertIn("response_dto.uploaded_chunks.push_back(idx)", block)
        self.assertIn("MediaUploadStatusResponseToJsonValue(response_dto)", block)

    def test_s3_tls_is_configurable_and_defaults_to_https(self):
        source = read_text(SERVER_CORE / "MediaService" / "core" / "storage" / "S3MediaStorage.cpp")
        ini = read_text(SERVER_CORE / "MediaService" / "mediagateway.ini")

        self.assertIn('minio["Scheme"]', source)
        self.assertIn('minio["VerifySSL"]', source)
        self.assertIn('const bool use_https = scheme != "http"', source)
        self.assertIn("Aws::Http::Scheme::HTTPS", source)
        self.assertIn("config.verifySSL = use_https && !IsFalseConfigValue", source)
        self.assertNotIn("config.scheme = Aws::Http::Scheme::HTTP;", source)
        self.assertNotIn("config.verifySSL = false;", source)
        self.assertIn("Scheme=http", ini)
        self.assertIn("VerifySSL=false", ini)

    def test_s3_storage_accepts_minio_env_aliases_used_by_local_runtime(self):
        source = read_text(SERVER_CORE / "MediaService" / "core" / "storage" / "S3MediaStorage.cpp")
        startup = read_text(REPO_ROOT / "tools/scripts/status/start-all-services.sh")

        for token in (
            '"MEMOCHAT_MINIO_ACCESSKEY"',
            '"MEMOCHAT_MINIO_ACCESS_KEY"',
            '"MEMOCHAT_MINIO_ROOT_USER"',
            '"MINIO_ROOT_USER"',
            '"MINIO_ACCESS_KEY"',
            '"MEMOCHAT_MINIO_SECRETKEY"',
            '"MEMOCHAT_MINIO_SECRET_KEY"',
            '"MEMOCHAT_MINIO_ROOT_PASSWORD"',
            '"MINIO_ROOT_PASSWORD"',
            '"MINIO_SECRET_KEY"',
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        self.assertIn("export_minio_runtime_credentials", startup)
        self.assertIn("first_env_value", startup)
        self.assertIn('access_key="${access_key:-memochat_admin}"', startup)
        self.assertIn('secret_key="${secret_key:-MinioPass2026!}"', startup)
        self.assertIn('MINIO_ACCESS_KEY="${MINIO_ACCESS_KEY:-}"', startup)
        self.assertIn('MINIO_SECRET_KEY="${MINIO_SECRET_KEY:-}"', startup)

    def test_e4_local_file_download_contract_uses_gate_response_file_body_kind(self):
        response_header = strip_comments(read_text(SERVER_CORE / "GateShared" / "routing" / "GateResponse.hpp"))
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

    def test_media_upload_requests_carry_explicit_grant_audience(self):
        dto_header = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaPublicDtos.hpp")
        dto_source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaPublicDtos.cpp")
        session_header = read_text(
            SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaUploadSessionDto.hpp"
        )

        for token in (
            "std::vector<int> grant_uids",
            "int64_t grant_group_id",
            "bool grant_public",
            "bool grant_friends",
        ):
            with self.subTest(token=token):
                self.assertIn(token, dto_header)
                self.assertIn(token, session_header)

        self.assertIn("ParseGrantUids(root)", dto_source)
        self.assertIn('glaze_safe_get<int64_t>(root, "grant_group_id", 0LL)', dto_source)
        self.assertIn('glaze_safe_get<bool>(root, "grant_public", false)', dto_source)
        self.assertIn('glaze_safe_get<bool>(root, "grant_friends", false)', dto_source)
        self.assertIn("NormalizeGrantUids", dto_source)

    def test_media_upload_completion_persists_grants_before_success(self):
        source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")

        init_block = extract_function_region(
            source,
            r"bool\s+MediaService::HandleUploadMediaInit\s*\(",
            r"\nbool\s+MediaService::HandleUploadMediaChunk",
        )
        self.assertIn("GrantSpecFromRequestLocal(upload_request)", init_block)
        self.assertIn("session.grant_uids = grants.grant_uids", init_block)
        self.assertIn("session.grant_group_id = grants.grant_group_id", init_block)
        self.assertIn("session.grant_public = grants.grant_public", init_block)
        self.assertIn("session.grant_friends = grants.grant_friends", init_block)

        for handler, next_handler, grant_source in (
            ("HandleUploadMediaComplete", "HandleUploadMediaSimple", "GrantSpecFromSessionLocal(session)"),
            ("HandleUploadMediaSimple", "HandleMediaDownload", "GrantSpecFromRequestLocal(upload_request)"),
        ):
            with self.subTest(handler=handler):
                block = extract_function_region(
                    source,
                    rf"bool\s+MediaService::{handler}\s*\(",
                    rf"\nbool\s+MediaService::{next_handler}",
                )
                save_index = block.index("MediaPersistence::Instance().SaveAsset(asset)")
                grant_index = block.index(f"ApplyMediaUploadGrantsLocal(media_key, uid, {grant_source}")
                success_index = block.index('root["error"] = ErrorCodes::Success')
                self.assertLess(save_index, grant_index)
                self.assertLess(grant_index, success_index)
                self.assertIn('root["message"] = grant_error', block)

    def test_media_grant_dao_supports_direct_public_friends_and_group_audiences(self):
        source = read_text(SERVER_CORE / "GateShared" / "core" / "persistence" / "PostgresDao.cpp")
        grant_block = extract_function_region(
            source,
            r"bool\s+PostgresDao::GrantMediaAccess\s*\(",
            r"\nbool\s+PostgresDao::GrantMediaGroupAccess",
        )
        group_block = extract_function_region(
            source,
            r"bool\s+PostgresDao::GrantMediaGroupAccess\s*\(",
            r"\nbool\s+PostgresDao::HasMediaAccess",
        )
        access_block = extract_function_region(
            source,
            r"bool\s+PostgresDao::HasMediaAccess\s*\(",
            r"\nbool\s+PostgresDao::AddMoment",
        )

        self.assertIn('grantee_uid == 0 && scope != "public" && scope != "friends"', grant_block)
        self.assertIn('scope + ":" + std::to_string(group_id)', group_block)
        self.assertIn("SELECT $1, 0, $3, $4 WHERE EXISTS", group_block)
        self.assertIn("owner_uid = $2", group_block)
        self.assertIn("grantee_uid = $2 OR (grantee_uid = 0 AND grant_scope = 'public')", access_block)
        self.assertIn("media_type = 'avatar'", access_block)
        self.assertLess(access_block.index("media_type = 'avatar'"), access_block.index("chat_media_access_grant"))
        self.assertIn("to_regclass('chat_group_member')", access_block)
        self.assertIn("grant_row.grant_scope = ('group:' || member.group_id::text)", access_block)
        self.assertIn("to_regclass('friend')", access_block)
        self.assertIn("grant_scope = 'friends'", access_block)

    def test_avatar_uploads_are_public_profile_media_without_broadening_other_media(self):
        h1_source = read_text(SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaService.cpp")
        h2_source = read_text(SERVER_CORE / "MediaService" / "core" / "support" / "Http2MediaSupport.cpp")

        for label, source in (("h1", h1_source), ("h2", h2_source)):
            with self.subTest(entrypoint=label):
                apply_block = extract_function_region(
                    source,
                    r"bool\s+ApplyMediaUploadGrantsLocal\s*\(",
                    r"\n(?:MediaService&\s+MediaService::Instance|std::string\s+DecodeBase64Local)",
                )
                self.assertIn('return media_type == "avatar"', source)
                self.assertIn("IsAvatarMediaTypeLocal(persisted.media_type)", apply_block)
                self.assertIn("grants.grant_public = true", apply_block)
                self.assertLess(
                    apply_block.index("IsAvatarMediaTypeLocal(persisted.media_type)"),
                    apply_block.index("HasAnyGrantLocal(grants)"),
                )

    def test_split_media_schema_lacks_relation_projection_and_access_fails_closed(self):
        schema = read_text(
            REPO_ROOT / "apps" / "server" / "migrations" / "postgresql" / "business" / "008_memo_media_schema.sql"
        )
        source = read_text(SERVER_CORE / "GateShared" / "core" / "persistence" / "PostgresDao.cpp")
        access_block = extract_function_region(
            source,
            r"bool\s+PostgresDao::HasMediaAccess\s*\(",
            r"\nbool\s+PostgresDao::AddMoment",
        )
        compact = normalize_space(access_block)

        self.assertNotIn("CREATE TABLE IF NOT EXISTS memo.friend", schema)
        self.assertNotIn("CREATE TABLE IF NOT EXISTS memo.chat_group_member", schema)
        self.assertIn("SELECT to_regclass('chat_group_member') IS NOT NULL", access_block)
        self.assertIn("SELECT to_regclass('friend') IS NOT NULL", access_block)
        self.assertIn(
            "if (friend_table_rows.empty() || friend_table_rows[0][0].is_null() || !friend_table_rows[0][0].as<bool>()) { return false; }",
            compact,
        )

    def test_client_media_uploads_send_grant_fields_for_chat_and_moments(self):
        request_header = read_text(CLIENT_QML / "shared" / "media" / "MediaUploadRequest.h")
        upload_source = read_text(CLIENT_QML / "shared" / "media" / "MediaUploadServiceUploads.cpp")
        runner_source = read_text(CLIENT_QML / "app" / "coordinators" / "MediaPendingAttachmentRunner.cpp")
        moments_source = read_text(CLIENT_QML / "features" / "moments" / "parsing" / "MomentsPublishPayload.cpp")
        controller_source = read_text(
            CLIENT_QML / "features" / "moments" / "controller" / "MomentsControllerPublish.cpp"
        )

        for token in ("QList<int> grantUids", "qint64 grantGroupId", "bool grantPublic", "bool grantFriends"):
            with self.subTest(request_field=token):
                self.assertIn(token, request_header)

        self.assertIn('payload["grant_uids"] = grantees', upload_source)
        self.assertIn('payload["grant_group_id"] = grantGroupId', upload_source)
        self.assertIn('payload["grant_public"] = true', upload_source)
        self.assertIn('payload["grant_friends"] = true', upload_source)

        self.assertIn("request.grantGroupId = queueSnapshot.groupId", runner_source)
        self.assertIn("request.grantUids.append(queueSnapshot.chatUid)", runner_source)
        self.assertIn("request.grantPublic = visibility == 0", moments_source)
        self.assertIn("request.grantFriends = visibility == 1", moments_source)
        self.assertIn("uid, token, visibility", controller_source)


if __name__ == "__main__":
    unittest.main()
