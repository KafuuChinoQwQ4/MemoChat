import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps/server/core"
ENV_EXAMPLE = REPO_ROOT / ".env.example"
GITIGNORE = REPO_ROOT / ".gitignore"
CI_WORKFLOW = REPO_ROOT / ".github/workflows/ci.yml"
AUTH_SERVICE = SERVER_CORE / "AccountShared/domain/services/auth/AuthService.cpp"
AUTH_CACHE = SERVER_CORE / "AccountShared/core/cache/AuthCache.cpp"
AUTH_CACHE_HEADER = SERVER_CORE / "AccountShared/core/cache/AuthCache.hpp"
AUTH_RATE_LIMITER = SERVER_CORE / "AccountShared/core/support/AuthLoginRateLimiter.cpp"
AUTH_RATE_LIMITER_HEADER = SERVER_CORE / "AccountShared/core/support/AuthLoginRateLimiter.hpp"
AUTH_LOGIN_SUPPORT = SERVER_CORE / "AccountShared/core/support/AuthLoginSupport.cpp"
CHAT_SESSION_CONFIG = SERVER_CORE / "ChatServer/config/ChatSessionConfig.cpp"
INI_CONFIG = SERVER_CORE / "common/runtime/IniConfig.cpp"
SECRET_DOC = SERVER_CORE / "docs/secret-management.md"
AUTH_SECRET = SERVER_CORE / "common/auth/AuthSecret.hpp"
REFRESH_TOKEN = SERVER_CORE / "common/auth/RefreshToken.cpp"
AUTH_PUBLIC_DTOS = SERVER_CORE / "AccountShared/core/support/AuthPublicDtos.cpp"
AUTH_PUBLIC_DTOS_HEADER = SERVER_CORE / "AccountShared/core/support/AuthPublicDtos.hpp"
AUTH_ROUTE_MODULE = SERVER_CORE / "AccountShared/domain/modules/auth/AuthRouteModule.cpp"
AUTH_ROUTE_SCHEMAS = SERVER_CORE / "AccountShared/domain/modules/auth/AuthRouteSchemas.cpp"
AUTH_ROUTE_REGISTRATION_MODULE = (
    SERVER_CORE / "AccountShared/domain/modules/auth/cxx_modules/AuthRouteRegistration.cppm"
)
AUTH_ROUTE_SCHEMA_MODULE = SERVER_CORE / "AccountShared/domain/modules/auth/cxx_modules/AuthRouteSchema.cppm"
ACCOUNT_PERSISTENCE = SERVER_CORE / "AccountShared/domain/services/account/AccountPersistence.cpp"
ACCOUNT_PERSISTENCE_HEADER = SERVER_CORE / "AccountShared/domain/services/account/AccountPersistence.hpp"
POSTGRES_DAO_ACCOUNT = SERVER_CORE / "GateShared/core/persistence/PostgresDaoAccount.cpp"
POSTGRES_DAO_HEADER = SERVER_CORE / "GateShared/core/persistence/PostgresDao.hpp"
GATE_DOMAIN_SERVER = SERVER_CORE / "GateShared/app/GateDomainServer.cpp"
CHAT_SERVER = SERVER_CORE / "ChatServer/app/ChatServer.cpp"
CHAT_LOGIC_SYSTEM = SERVER_CORE / "ChatServer/domain/orchestration/LogicSystem.cpp"
CHAT_PRIVATE_MESSAGE = SERVER_CORE / "ChatServer/domain/message/PrivateMessageService.cpp"
CHAT_GROUP_MESSAGE = SERVER_CORE / "ChatServer/domain/message/GroupMessageService.cpp"
CHAT_SESSION_SERVICE = SERVER_CORE / "ChatServer/domain/session/ChatSessionService.cpp"
CHAT_SESSION_REPOSITORY = SERVER_CORE / "ChatServer/persistence/ChatSessionRepository.cpp"
CHAT_REDIS = SERVER_CORE / "ChatServer/persistence/RedisMgr.cpp"
CHAT_REDIS_HEADER = SERVER_CORE / "ChatServer/persistence/RedisMgr.hpp"
CHAT_LOGIN_TICKET = SERVER_CORE / "common/auth/ChatLoginTicket.hpp"
USER_TOKEN_VALIDATOR = SERVER_CORE / "GateShared/core/support/UserTokenValidator.cpp"
PROFILE_ROUTE_MODULE = SERVER_CORE / "AccountShared/domain/modules/profile/ProfileRouteModule.cpp"
HTTP2_PROFILE_SUPPORT = SERVER_CORE / "AccountShared/core/support/Http2ProfileSupport.cpp"
MOMENTS_SERVICE = SERVER_CORE / "MomentsService/domain/services/moments/MomentsService.cpp"
MOMENTS_PERSISTENCE = SERVER_CORE / "MomentsService/domain/services/moments/MomentsPersistence.cpp"
MOMENTS_PERSISTENCE_HEADER = SERVER_CORE / "MomentsService/domain/services/moments/MomentsPersistence.hpp"
POSTGRES_DAO = SERVER_CORE / "GateShared/core/persistence/PostgresDao.cpp"
POSTGRES_MGR = SERVER_CORE / "GateShared/core/persistence/PostgresMgr.cpp"
VARIFY_SERVICE_IMPL = SERVER_CORE / "VarifyServer/VarifyServiceImpl.cpp"
VARIFY_RATE_LIMITER = SERVER_CORE / "VarifyServer/RateLimiter.cpp"
AIGATEWAY_AI_SERVICE = SERVER_CORE / "AIGatewayService/domain/services/ai/AIService.cpp"
AIGATEWAY_AI_CLIENT = SERVER_CORE / "AIGatewayService/domain/AIServiceClient.cpp"
AIGATEWAY_ROUTE_MODULE = SERVER_CORE / "AIGatewayService/domain/AIRouteModules.cpp"
AIGATEWAY_CONFIG = SERVER_CORE / "AIGatewayService/aigateway.ini"
AISERVER_IMPL = SERVER_CORE / "AIServer/AIServiceImpl.cpp"
AISERVER_CLIENT = SERVER_CORE / "AIServer/AIServiceClient.cpp"
AISERVER_CORE = SERVER_CORE / "AIServer/AIServiceCore.cpp"
AISERVER_SESSION_REPO = SERVER_CORE / "AIServer/db/AISessionRepo.cpp"
AISERVER_SESSION_REPO_HEADER = SERVER_CORE / "AIServer/db/AISessionRepo.hpp"
AISERVER_CONFIG = SERVER_CORE / "AIServer/config.ini"
AIORCH_CONFIG = SERVER_CORE / "AIOrchestrator/config.py"
AIORCH_CONFIG_YAML = SERVER_CORE / "AIOrchestrator/config.yaml"
AIORCH_MODEL_ROUTER = SERVER_CORE / "AIOrchestrator/api/model_router.py"
AIORCH_AGENT_ROUTER = SERVER_CORE / "AIOrchestrator/api/agent_router.py"
AIORCH_LLM_SERVICE = SERVER_CORE / "AIOrchestrator/harness/llm/service.py"
AIORCH_TASK_SERVICE = SERVER_CORE / "AIOrchestrator/harness/runtime/task_service.py"
AIORCH_MEMORY_SERVICE = SERVER_CORE / "AIOrchestrator/harness/memory/service.py"
AIORCH_PET_ROUTER = SERVER_CORE / "AIOrchestrator/api/pet_router.py"
AIORCH_PET_RUNTIME = SERVER_CORE / "AIOrchestrator/harness/pet/runtime.py"
AIORCH_PET_VOICE_TRAINING = SERVER_CORE / "AIOrchestrator/harness/pet/voice_training.py"
REDACTION_MODULE = SERVER_CORE / "common/logging/cxx_modules/Redaction.cppm"
H1_HTTP_CONNECTION = SERVER_CORE / "GateShared/HttpConnection.cpp"
H3_HTTP_LISTENER = SERVER_CORE / "GateShared/transports/h3/listener/GateHttp3Listener.cpp"
START_ALL_SERVICES = REPO_ROOT / "tools/scripts/status/start-all-services.sh"
BASELINE_MIGRATION = REPO_ROOT / "apps/server/migrations/postgresql/business/001_baseline.sql"
ACCOUNT_SCHEMA_MIGRATION = REPO_ROOT / "apps/server/migrations/postgresql/business/009_memo_account_schema.sql"
REFRESH_TOKEN_MIGRATION = REPO_ROOT / "apps/server/migrations/postgresql/business/011_auth_refresh_tokens.sql"
BUSINESS_INIT = REPO_ROOT / "infra/deploy/local/init/postgresql/001-business.sql"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : index]
    raise AssertionError(f"Could not parse function body for {signature}")


def compact(source: str) -> str:
    return re.sub(r"\s+", " ", source)


class SecurityHardeningContractTests(unittest.TestCase):
    def test_chatserver_rejects_unauthenticated_business_messages_and_binds_payload_uid(self):
        logic = read(CHAT_LOGIC_SYSTEM)
        private = read(CHAT_PRIVATE_MESSAGE)
        group = read(CHAT_GROUP_MESSAGE)

        self.assertIn("RejectUnauthenticatedBusinessMessage", logic)
        self.assertIn("msg_id == MSG_CHAT_LOGIN || msg_id == ID_HEART_BEAT_REQ", logic)
        self.assertIn("session->userId() > 0", logic)
        self.assertIn("session->close();", logic)
        self.assertIn("NormalizeAuthenticatedCallerFields", logic)
        for field in ('"uid"', '"fromuid"', '"owner_uid"', '"from_uid"', '"reviewer_uid"', '"viewer_uid"'):
            self.assertIn(field, logic)
        self.assertIn("root[field] = session_uid;", logic)

        self.assertIn("AuthenticatedRequestUidLocal", private)
        self.assertIn("request.session_uid > 0 ? request.session_uid : payload_uid", private)
        self.assertNotRegex(private, r'const\s+(?:auto|int)\s+uid\s*=\s*root\["fromuid"\]')

        self.assertIn("BuildGroupMessageCommandRequestLocal", group)
        self.assertIn("request.session_uid = session->userId();", group)
        self.assertNotRegex(group, r"const\s+int\s+(?:uid|from_uid|owner_uid|reviewer_uid)\s*=\s*command\.")

    def test_profile_and_moments_routes_resolve_bearer_uid_before_mutation(self):
        profile = read(PROFILE_ROUTE_MODULE)
        http2_profile = read(HTTP2_PROFILE_SUPPORT)
        moments = read(MOMENTS_SERVICE)

        self.assertIn('#include "support/BearerAccessAuth.hpp"', profile)
        self.assertIn("gateauthsupport::ValidateProfileUpdateRequest(profile_request)", profile)
        self.assertLess(
            profile.index("ValidateProfileUpdateRequest(profile_request)"),
            profile.index("ResolveBearerAccessUserId(request, uid)"),
        )
        self.assertIn("memochat::auth::ResolveBearerAccessUserId(request, uid)", profile)
        self.assertLess(profile.index("ResolveBearerAccessUserId(request, uid)"), profile.index("UpdateUserProfile"))

        self.assertIn('#include "support/UserTokenValidator.hpp"', http2_profile)
        self.assertIn("gateauthsupport::ValidateProfileUpdateRequest(profile_request)", http2_profile)
        self.assertLess(
            http2_profile.index("ValidateProfileUpdateRequest(profile_request)"),
            http2_profile.index("ResolveUserIdFromToken(access_token, uid)"),
        )
        self.assertIn("memochat::auth::ResolveUserIdFromToken(access_token, uid)", http2_profile)
        self.assertLess(
            http2_profile.index("ResolveUserIdFromToken(access_token, uid)"),
            http2_profile.index("UpdateUserProfile"),
        )

        self.assertIn('#include "support/BearerAccessAuth.hpp"', moments)
        self.assertIn("memochat::auth::ResolveBearerAccessUserId(request, uid)", moments)
        self.assertNotIn("ValidateUserToken(uid, token)", moments)
        self.assertNotIn("LoginTicketField()", moments)

    def test_moments_interactions_enforce_visibility_before_read_or_mutation(self):
        moments = read(MOMENTS_SERVICE)
        persistence = read(MOMENTS_PERSISTENCE)
        persistence_header = read(MOMENTS_PERSISTENCE_HEADER)
        postgres_dao = read(POSTGRES_DAO)
        postgres_mgr = read(POSTGRES_MGR)

        self.assertIn("bool LoadVisibleMoment(", moments)
        self.assertIn("bool LoadVisibleMomentForComment(", moments)
        self.assertIn("CanViewMomentRemote(viewer_uid, moment)", moments)

        like_body = function_body(moments, "bool MomentsService::HandleLike")
        self.assertLess(
            like_body.index("LoadVisibleMoment(uid, request_dto.moment_id"), like_body.index("SetMomentLike")
        )

        comment_body = function_body(moments, "bool MomentsService::HandleComment")
        self.assertLess(comment_body.index("LoadVisibleMomentForComment"), comment_body.index("DeleteComment"))
        self.assertLess(
            comment_body.index("LoadVisibleMoment(uid, request_dto.moment_id"), comment_body.index("AddComment")
        )
        self.assertIn("request_dto.moment_id != deleted_comment.moment_id", comment_body)

        comment_list_body = function_body(moments, "bool MomentsService::HandleCommentList")
        self.assertLess(
            comment_list_body.index("LoadVisibleMoment(uid, request_dto.moment_id"),
            comment_list_body.index("LoadComments"),
        )

        comment_like_body = function_body(moments, "bool MomentsService::HandleCommentLike")
        self.assertLess(
            comment_like_body.index("LoadVisibleMomentForComment"), comment_like_body.index("SetCommentLike")
        )

        self.assertIn("bool LoadComment", persistence_header)
        self.assertIn("GetMomentCommentById(comment_id, comment)", persistence)
        self.assertIn("bool PostgresDao::GetMomentCommentById", postgres_dao)
        self.assertIn("WHERE mc.id = $1", postgres_dao)
        self.assertIn("bool PostgresMgr::GetMomentCommentById", postgres_mgr)

    def test_moments_soft_deleted_rows_are_not_visible_or_mutable_by_id(self):
        postgres_dao = read(POSTGRES_DAO)

        get_moment = function_body(postgres_dao, "bool PostgresDao::GetMomentById")
        self.assertIn("FROM moments WHERE moment_id = $1 AND deleted_at = 0 LIMIT 1", get_moment)

        add_like = function_body(postgres_dao, "bool PostgresDao::AddMomentLike")
        self.assertIn("SELECT 1 FROM moments WHERE moment_id = $1 AND deleted_at = 0", add_like)
        self.assertIn("WHERE moment_id = $1 AND deleted_at = 0", add_like)

        remove_like = function_body(postgres_dao, "bool PostgresDao::RemoveMomentLike")
        self.assertIn("DELETE FROM moments_like ml USING moments m", remove_like)
        self.assertIn("m.moment_id = ml.moment_id AND m.deleted_at = 0", remove_like)
        self.assertIn("WHERE moment_id = $1 AND deleted_at = 0", remove_like)

        has_like = function_body(postgres_dao, "bool PostgresDao::HasLikedMoment")
        self.assertIn("JOIN moments m ON m.moment_id = ml.moment_id", has_like)
        self.assertIn("AND m.deleted_at = 0 LIMIT 1", has_like)

        list_likes = function_body(postgres_dao, "bool PostgresDao::GetMomentLikes")
        self.assertIn("JOIN moments m ON m.moment_id = ml.moment_id", list_likes)
        self.assertIn("WHERE ml.moment_id = $1 AND m.deleted_at = 0", list_likes)

        add_comment = function_body(postgres_dao, "bool PostgresDao::AddMomentComment")
        self.assertIn("SELECT 1 FROM moments WHERE moment_id = $1 AND deleted_at = 0", add_comment)
        self.assertIn("return false;", add_comment)
        self.assertIn("WHERE moment_id = $1 AND deleted_at = 0", add_comment)

        load_comment = function_body(postgres_dao, "bool PostgresDao::GetMomentCommentById")
        self.assertIn("JOIN moments m ON m.moment_id = mc.moment_id", load_comment)
        self.assertIn("AND m.deleted_at = 0 LIMIT 1", load_comment)

        list_comments = function_body(postgres_dao, "bool PostgresDao::GetMomentComments")
        self.assertIn("JOIN moments m ON m.moment_id = mc.moment_id", list_comments)
        self.assertIn("AND m.deleted_at = 0", list_comments)

        comment_like = function_body(postgres_dao, "bool PostgresDao::AddMomentCommentLike")
        self.assertIn("JOIN moments m ON m.moment_id = mc.moment_id", comment_like)
        self.assertIn("mc.deleted_at = 0 AND m.deleted_at = 0", comment_like)

        remove_comment_like = function_body(postgres_dao, "bool PostgresDao::RemoveMomentCommentLike")
        self.assertIn("DELETE FROM moments_comment_like mcl USING moments_comment mc, moments m", remove_comment_like)
        self.assertIn("mc.id = mcl.comment_id AND mc.deleted_at = 0", remove_comment_like)
        self.assertIn("m.moment_id = mc.moment_id AND m.deleted_at = 0", remove_comment_like)

        has_comment_like = function_body(postgres_dao, "bool PostgresDao::HasLikedMomentComment")
        self.assertIn("JOIN moments_comment mc ON mc.id = mcl.comment_id", has_comment_like)
        self.assertIn("JOIN moments m ON m.moment_id = mc.moment_id", has_comment_like)
        self.assertIn("AND mc.deleted_at = 0 AND m.deleted_at = 0 LIMIT 1", has_comment_like)

        list_comment_likes = function_body(postgres_dao, "bool PostgresDao::GetMomentCommentLikes")
        self.assertIn("JOIN moments_comment mc ON mc.id = mcl.comment_id", list_comment_likes)
        self.assertIn("JOIN moments m ON m.moment_id = mc.moment_id", list_comment_likes)
        self.assertIn("AND mc.deleted_at = 0 AND m.deleted_at = 0", list_comment_likes)

    def test_ai_provider_management_requires_admin_secret_at_all_entry_points(self):
        gateway_service = read(AIGATEWAY_AI_SERVICE)
        gateway_client = read(AIGATEWAY_AI_CLIENT)
        gateway_config = read(AIGATEWAY_CONFIG)
        aiserver_impl = read(AISERVER_IMPL)
        aiserver_client = read(AISERVER_CLIENT)
        aiserver_config = read(AISERVER_CONFIG)
        orchestrator_config = read(AIORCH_CONFIG)
        orchestrator_yaml = read(AIORCH_CONFIG_YAML)
        model_router = read(AIORCH_MODEL_ROUTER)

        for source in (gateway_config, aiserver_config):
            with self.subTest(source_hash=hash(source)):
                self.assertIn("AIProviderAdmin", source)
                self.assertIn("X-MemoChat-AI-Provider-Admin-Key", source)
                self.assertIn("MEMOCHAT_AI_PROVIDER_ADMIN_KEY", source)
        self.assertIn("provider_admin_auth_header", orchestrator_yaml)
        self.assertIn("X-MemoChat-AI-Provider-Admin-Key", orchestrator_yaml)
        self.assertIn("MEMOCHAT_AI_PROVIDER_ADMIN_KEY", orchestrator_yaml)

        register_body = function_body(gateway_service, "bool AIService::HandleRegisterApiProvider")
        delete_body = function_body(gateway_service, "bool AIService::HandleDeleteApiProvider")
        for body, call in (
            (register_body, "Client().RegisterApiProvider"),
            (delete_body, "Client().DeleteApiProvider"),
        ):
            with self.subTest(call=call):
                self.assertLess(body.index("RequireUserAuth"), body.index("RequireProviderAdmin"))
                self.assertLess(body.index("RequireProviderAdmin"), body.index(call))

        self.assertIn("ResolveProviderAdminKey", gateway_service)
        self.assertIn("ConstantTimeEquals", gateway_service)
        self.assertIn("ShouldRejectProviderAdminAuth", gateway_service)
        self.assertIn("ProviderAdminAuthRequiredMessage", gateway_service)

        self.assertIn("AttachProviderAdminMetadata(ctx)", gateway_client)
        self.assertIn("ctx.AddMetadata(ProviderAdminMetadataKey(), key)", gateway_client)

        self.assertIn("RequireProviderAdminMetadata", aiserver_impl)
        self.assertIn("context->client_metadata()", aiserver_impl)
        self.assertIn("grpc::StatusCode::PERMISSION_DENIED", aiserver_impl)
        aiserver_register = function_body(aiserver_impl, "grpc::Status AIServiceImpl::RegisterApiProvider")
        aiserver_delete = function_body(aiserver_impl, "grpc::Status AIServiceImpl::DeleteApiProvider")
        self.assertLess(aiserver_register.index("RequireProviderAdminMetadata"), aiserver_register.index("_core->"))
        self.assertLess(aiserver_delete.index("RequireProviderAdminMetadata"), aiserver_delete.index("_core->"))

        self.assertIn("ResolveAiOrchestratorInternalKey", aiserver_client)
        self.assertIn("ResolveProviderAdminKey", aiserver_client)
        self.assertIn("req.set(AiOrchestratorInternalHeader(), internal_key)", aiserver_client)
        self.assertIn("req.set(ProviderAdminAuthHeader(), provider_admin_key)", aiserver_client)
        self.assertIn("IsProviderAdminPath(path)", aiserver_client)

        self.assertIn("provider_admin_auth_header", orchestrator_config)
        self.assertIn("provider_admin_key_env", orchestrator_config)
        self.assertIn("def resolve_provider_admin_key", orchestrator_config)
        self.assertIn("def _require_provider_admin", model_router)
        self.assertIn("hmac.compare_digest", model_router)
        self.assertIn("HTTPException(status_code=403", model_router)
        register_section = model_router[
            model_router.index("async def register_api_provider") : model_router.index(
                '@router.post("/api-provider/delete"'
            )
        ]
        delete_section = model_router[model_router.index("async def delete_api_provider") :]
        self.assertLess(
            register_section.index("_require_provider_admin(request)"), register_section.index("HarnessContainer")
        )
        self.assertLess(
            delete_section.index("_require_provider_admin(request)"),
            delete_section.index("delete_api_provider(req.provider_id)"),
        )

    def test_ai_gateway_binds_all_user_scoped_operations_to_authenticated_uid(self):
        gateway_service = read(AIGATEWAY_AI_SERVICE)
        route_module = read(AIGATEWAY_ROUTE_MODULE)

        self.assertIn("RequireUserAuth", gateway_service)
        self.assertIn("ResolveBearerAccessUserId(request, uid)", gateway_service)
        self.assertIn("RequireConnectionUserAuth", route_module)
        self.assertIn('ExtractBearerAccessToken(HeaderValue(connection, "authorization"))', route_module)
        self.assertIn("PrefixProxyBodyWithTrustedUid(IncomingBodyString(connection), auth_uid)", route_module)
        self.assertIn("PrefixProxyTargetWithTrustedUid(std::move(target), auth_uid)", route_module)

        user_scoped_calls = (
            ("bool AIService::HandleChat", "Client().Chat(auth_uid"),
            ("bool AIService::HandleSmart", "Client().Smart(auth_uid"),
            ("bool AIService::HandleHistory", "Client().GetHistory(auth_uid"),
            ("bool AIService::HandleCreateSession", "Client().CreateSession(auth_uid"),
            ("bool AIService::HandleListSessions", "Client().ListSessions(auth_uid"),
            ("bool AIService::HandleDeleteSession", "Client().DeleteSession(auth_uid"),
            ("bool AIService::HandleUpdateSession", "Client().UpdateSession(auth_uid"),
            ("bool AIService::HandleKbUpload", "Client().KbUpload(auth_uid"),
            ("bool AIService::HandleKbSearch", "Client().KbSearch(auth_uid"),
            ("bool AIService::HandleListKb", "Client().ListKb(auth_uid"),
            ("bool AIService::HandleDeleteKb", "Client().DeleteKb(auth_uid"),
            ("bool AIService::HandleMemoryList", "Client().MemoryList(auth_uid"),
            ("bool AIService::HandleMemoryCreate", "Client().MemoryCreate(auth_uid"),
            ("bool AIService::HandleMemoryDelete", "Client().MemoryDelete(auth_uid"),
            ("bool AIService::HandleTaskCreate", "Client().AgentTaskCreate(auth_uid"),
            ("bool AIService::HandleTaskList", "Client().AgentTaskList(auth_uid"),
        )
        for signature, call in user_scoped_calls:
            with self.subTest(signature=signature):
                body = function_body(gateway_service, signature)
                self.assertIn("int32_t auth_uid = 0", body)
                self.assertIn("&auth_uid", body)
                self.assertIn(call, body)

        stream_block = route_module[route_module.index('"/ai/chat/stream"') :]
        self.assertIn("RequireConnectionUserAuth(connection, &auth_uid)", stream_block)
        self.assertRegex(stream_block, r"g_ai_stream_client->ChatStream\(\s*auth_uid")
        self.assertNotRegex(stream_block, r"g_ai_stream_client->ChatStream\(\s*request_uid")

    def test_ai_agent_task_operations_are_bound_to_authenticated_uid(self):
        gateway_service = read(AIGATEWAY_AI_SERVICE)
        gateway_client = read(AIGATEWAY_AI_CLIENT)
        aiserver_client = read(AISERVER_CLIENT)
        aiserver_core = read(AISERVER_CORE)
        agent_router = read(AIORCH_AGENT_ROUTER)
        task_service = read(AIORCH_TASK_SERVICE)

        detail_body = function_body(gateway_service, "bool AIService::HandleTaskDetail")
        cancel_body = function_body(gateway_service, "bool AIService::HandleTaskCancel")
        resume_body = function_body(gateway_service, "bool AIService::HandleTaskResume")
        for body, call in (
            (detail_body, "Client().AgentTaskGet(auth_uid, task_id)"),
            (cancel_body, "Client().AgentTaskCancel(auth_uid, task_request.task_id)"),
            (resume_body, "Client().AgentTaskResume(auth_uid, task_request.task_id)"),
        ):
            with self.subTest(call=call):
                self.assertIn("int32_t auth_uid = 0", body)
                self.assertIn("&auth_uid", body)
                self.assertIn(call, body)

        for token in (
            "memochat::json::JsonValue AIServiceClient::AgentTaskGet(int32_t uid",
            "memochat::json::JsonValue AIServiceClient::AgentTaskCancel(int32_t uid",
            "memochat::json::JsonValue AIServiceClient::AgentTaskResume(int32_t uid",
            "req.set_uid(uid);",
        ):
            with self.subTest(token=token):
                self.assertIn(token, gateway_client)

        for token in (
            "AgentTaskGet(req.uid(), req.task_id(), &result)",
            "AgentTaskCancel(req.uid(), req.task_id(), &result)",
            "AgentTaskResume(req.uid(), req.task_id(), &result)",
        ):
            with self.subTest(token=token):
                self.assertIn(token, aiserver_core)

        for signature in (
            "AIServiceClient::AgentTaskGet(int32_t uid",
            "AIServiceClient::AgentTaskCancel(int32_t uid",
            "AIServiceClient::AgentTaskResume(int32_t uid",
        ):
            with self.subTest(signature=signature):
                self.assertIn(signature, aiserver_client)
        self.assertGreaterEqual(aiserver_client.count("client_modules::UidQueryPrefix() + std::to_string(uid)"), 3)

        for token in (
            "async def get_task(task_id: str, uid: int)",
            "async def cancel_task(task_id: str, uid: int)",
            "async def resume_task(task_id: str, uid: int)",
            "get_task_for_uid(task_id, uid)",
            "cancel_task(task_id, uid)",
            "resume_task(task_id, uid)",
            'HTTPException(status_code=400, detail="uid is required")',
        ):
            with self.subTest(token=token):
                self.assertIn(token, agent_router)

        self.assertIn("async def get_task_for_uid", task_service)
        self.assertIn("task_uid == int(uid)", task_service)
        self.assertIn("task_uid != int(uid)", task_service)

    def test_ai_sessions_are_bound_to_owner_uid_before_history_or_mutation(self):
        repo_header = read(AISERVER_SESSION_REPO_HEADER)
        repo = read(AISERVER_SESSION_REPO)
        core = read(AISERVER_CORE)

        for token in (
            "bool SoftDelete(int32_t uid, const std::string& session_id)",
            "std::unique_ptr<ai::AISessionInfo> GetSession(int32_t uid, const std::string& session_id)",
            "std::vector<ai::AIMessage> GetMessages(int32_t uid, const std::string& session_id",
        ):
            with self.subTest(token=token):
                self.assertIn(token, repo_header)

        self.assertIn("WHERE uid = $2 AND session_id = $3 AND deleted_at IS NULL", repo)
        self.assertIn("WHERE uid = $1 AND session_id = $2 AND deleted_at IS NULL", repo)
        self.assertIn("SELECT $1, s.session_id, $3, $4, $5, $6, $7", repo)
        self.assertIn("WHERE s.session_id = $2 AND s.uid = $8 AND s.deleted_at IS NULL", repo)
        self.assertIn("JOIN ai_session s ON s.session_id = m.session_id", repo)
        self.assertIn("WHERE s.uid = $1 AND m.session_id = $2", repo)

        chat_body = function_body(core, "grpc::Status AIServiceCore::HandleChat")
        self.assertLess(
            chat_body.index("_session_repo->GetSession(req.from_uid(), session_id)"), chat_body.index("SaveUserMessage")
        )
        self.assertIn("if (!SaveUserMessage(session_id, req.from_uid()", chat_body)
        self.assertLess(chat_body.index("if (!SaveUserMessage"), chat_body.index("_ai_client->Chat("))

        stream_body = function_body(core, "grpc::Status AIServiceCore::HandleChatStream")
        self.assertLess(
            stream_body.index("_session_repo->GetSession(req.from_uid(), session_id)"),
            stream_body.index("SaveUserMessage"),
        )
        self.assertIn("if (!SaveUserMessage(session_id, req.from_uid()", stream_body)
        self.assertLess(stream_body.index("if (!SaveUserMessage"), stream_body.index("_ai_client->ChatStream("))

        history_body = function_body(core, "grpc::Status AIServiceCore::GetHistory")
        self.assertIn("_session_repo->GetSession(req.from_uid(), req.session_id())", history_body)
        self.assertIn("_session_repo->GetMessages(req.from_uid()", history_body)
        self.assertLess(
            history_body.index("GetSession(req.from_uid()"), history_body.index("GetMessages(req.from_uid()")
        )

        delete_body = function_body(core, "grpc::Status AIServiceCore::DeleteSession")
        self.assertIn("_session_repo->SoftDelete(req.uid(), req.session_id())", delete_body)

        create_body = function_body(core, "grpc::Status AIServiceCore::CreateSession")
        update_body = function_body(core, "grpc::Status AIServiceCore::UpdateSession")
        self.assertIn("_session_repo->GetSession(req.uid(), session_id)", create_body)
        self.assertIn("_session_repo->GetSession(req.uid(), req.session_id())", update_body)

    def test_ai_pet_proxy_sse_and_orchestrator_memory_are_owner_scoped(self):
        route_module = read(AIGATEWAY_ROUTE_MODULE)
        aiserver_client = read(AISERVER_CLIENT)
        memory_service = read(AIORCH_MEMORY_SERVICE)
        pet_router = read(AIORCH_PET_ROUTER)
        pet_runtime = read(AIORCH_PET_RUNTIME)
        voice_training = read(AIORCH_PET_VOICE_TRAINING)

        proxy_body = function_body(route_module, "static void ProxyAiOrchestratorPrefix")
        self.assertLess(proxy_body.index("RequireConnectionUserAuth"), proxy_body.index("BuildPrefixProxyTarget"))
        self.assertIn('ProxyAiOrchestratorPrefix(connection, verb, "/ai/pet", "/pet"', route_module)
        self.assertIn("RequireConnectionUserAuth(connection, &auth_uid)", route_module)
        self.assertIn("ProxyAiOrchestratorPetStream(connection, auth_uid)", route_module)
        self.assertIn(
            'PrefixProxyTargetWithTrustedUid(BuildPrefixProxyTarget(connection, "/ai/pet", "/pet"), auth_uid)',
            route_module,
        )
        self.assertLess(
            route_module.index("RequireConnectionUserAuth(connection, &auth_uid)"),
            route_module.index("ProxyAiOrchestratorPetStream(connection, auth_uid)"),
        )

        sse_body = function_body(aiserver_client, "grpc::Status PostJsonSSE")
        self.assertLess(sse_body.index("ResolveAiOrchestratorInternalKey"), sse_body.index("req.prepare_payload()"))
        self.assertIn("req.set(AiOrchestratorInternalHeader(), internal_key)", sse_body)

        self.assertIn("chat_history, short_term_summary = await self._load_short_term(uid, session_id)", memory_service)
        self.assertIn("async def _load_short_term(self, uid: int, session_id: str)", memory_service)
        self.assertIn("JOIN ai_session s ON s.session_id = m.session_id", memory_service)
        self.assertIn("s.uid = $1", memory_service)
        self.assertIn("m.session_id = $2", memory_service)

        self.assertIn('raise HTTPException(status_code=400, detail="uid is required")', pet_router)
        self.assertIn("async def get_voice_training_job(job_id: str, uid: int = 0)", pet_router)
        self.assertIn("_runtime.get_voice_training_job(job_id, uid)", pet_router)
        self.assertIn("def get_voice_training_job(self, job_id: str, uid: int)", pet_runtime)
        self.assertIn("def list_voice_training_jobs(self, uid: int)", pet_runtime)
        self.assertIn("self._voice_runtime_metadata(uid=session.uid)", pet_runtime)
        self.assertIn("self._configured_or_trained_reference_audio(uid=uid)", pet_runtime)
        self.assertIn("latest_ready_reference_audio(uid=uid)", pet_runtime)
        self.assertIn("def get_job(self, job_id: str, uid: int)", voice_training)
        self.assertIn("if uid <= 0 or job.uid != uid:", voice_training)
        self.assertIn("def list_jobs(self, uid: int)", voice_training)
        self.assertIn("if uid <= 0:", voice_training)
        self.assertIn("return []", voice_training)
        self.assertIn("def latest_ready_reference_audio(self, uid: int = 0)", voice_training)

    def test_runtime_ai_provider_api_key_is_not_persisted_plaintext(self):
        service = read(AIORCH_LLM_SERVICE)

        self.assertIn("self._runtime_provider_api_keys", service)
        self.assertIn("_runtime_provider_api_key_env_name", service)
        self.assertIn("_provider_api_fingerprint", service)
        self.assertIn('"api_key_env": api_key_env', service)
        self.assertIn('"api_key_fingerprint": _provider_api_fingerprint(api_key)', service)
        self.assertIn('provider.pop("api_key"', service)
        self.assertIn("self._runtime_provider_api_key(runtime_cfg)", service)
        self.assertNotIn('"api_key": api_key', service)

    def test_jwt_access_tokens_rotate_with_ttl_and_reset_consumes_verification_code(self):
        auth_service = read(AUTH_SERVICE)
        auth_cache = read(AUTH_CACHE)
        auth_cache_header = read(AUTH_CACHE_HEADER)
        auth_login_support = read(AUTH_LOGIN_SUPPORT)
        user_token_validator = read(USER_TOKEN_VALIDATOR)
        http2_media = read(SERVER_CORE / "MediaService/core/support/Http2MediaSupport.cpp")
        issue_body = function_body(auth_service, "bool IssueLoginSessionForUser")
        reset_body = function_body(auth_service, "bool AuthService::HandleResetPwd")

        self.assertIn("int GetAccessTokenTtlSec()", auth_login_support)
        self.assertIn('GetValue("AuthToken", "AccessTokenTtlSec")', auth_login_support)
        self.assertIn("std::string GetJwtAccessSecret()", auth_login_support)
        self.assertIn("std::string GetJwtAccessIssuer()", auth_login_support)
        self.assertIn("std::string GetJwtAccessAudience()", auth_login_support)
        self.assertIn("bool SetHttpToken", auth_cache_header)
        self.assertIn("memochat::auth::StoreUserToken(uid, token, ttl_seconds)", auth_cache)
        self.assertIn('constexpr std::string_view kUserTokenLookupPrefix = "utoken_lookup_";', user_token_validator)
        self.assertIn("DecodeAndVerifyAccessToken(token, JwtAccessSecret(), options, claims", user_token_validator)
        self.assertIn("ShouldAcceptJwtAccessClaims(jwt_valid, uid, claims.uid)", user_token_validator)
        self.assertIn("RedisMgr::GetInstance()->SetEx(token_key, token, ttl_seconds)", user_token_validator)
        self.assertIn(
            "RedisMgr::GetInstance()->SetEx(lookup_key, std::to_string(uid), ttl_seconds)", user_token_validator
        )
        self.assertIn("boost::uuids::random_generator()()", issue_body)
        self.assertIn("JwtAccessTokenClaims access_claims", issue_body)
        self.assertIn("access_claims.uid = userInfo.uid", issue_body)
        self.assertIn("access_claims.sub = std::to_string(userInfo.uid)", issue_body)
        self.assertIn("EncodeAccessToken(access_claims, gateauthsupport::GetJwtAccessSecret())", issue_body)
        self.assertIn("SetHttpToken(userInfo.uid, access_token, access_token_ttl_sec)", issue_body)
        self.assertIn('root["access_token"] = access_token', issue_body)
        self.assertNotIn("GetHttpToken(userInfo.uid, access_token)", issue_body)
        self.assertIn("ResolveAccessTokenUidLocal(const std::string& access_token, int& uid)", http2_media)
        self.assertIn("memochat::auth::ResolveUserIdFromToken(access_token, uid)", http2_media)
        self.assertNotIn("ValidateUserTokenLocal(uid, token)", http2_media)

        self.assertIn("void DeleteVerificationCode", auth_cache_header)
        self.assertIn("RedisMgr::GetInstance()->Del(BuildVerificationCodeKey(email));", auth_cache)
        self.assertLess(
            reset_body.index("UpdatePassword(email, pwd)"), reset_body.index("DeleteVerificationCode(email)")
        )

        login_body = function_body(auth_service, "bool AuthService::HandleUserLogin")
        self.assertIn("rate-limit counter check failed closed", login_body)
        self.assertIn('root["error"] = ErrorCodes::RateLimited;', login_body)

    def test_chat_route_cluster_cache_expires_instead_of_process_lifetime_static(self):
        auth_login_support = read(AUTH_LOGIN_SUPPORT)

        self.assertIn("constexpr auto kChatRouteClusterCacheTtl = std::chrono::seconds(60);", auth_login_support)
        self.assertIn("LoadChatClusterConfigSnapshot", auth_login_support)
        self.assertIn("std::chrono::steady_clock::now()", auth_login_support)
        self.assertIn("cached_at", auth_login_support)
        self.assertIn("now - cached_at >= kChatRouteClusterCacheTtl", auth_login_support)
        self.assertNotIn("static const auto kCachedCluster", auth_login_support)

    def test_chat_login_ticket_has_one_time_jti_consumption(self):
        ticket = read(CHAT_LOGIN_TICKET)
        auth_service = read(AUTH_SERVICE)
        session = read(CHAT_SESSION_SERVICE)
        session_repository = read(CHAT_SESSION_REPOSITORY)
        redis_header = read(CHAT_REDIS_HEADER)
        redis_source = read(CHAT_REDIS)

        self.assertIn("std::string jti;", ticket)
        self.assertIn('"jti"', ticket)
        self.assertIn("claims.jti = boost::uuids::to_string(boost::uuids::random_generator()());", auth_service)
        self.assertIn("ConsumeLoginTicketJti", session)
        self.assertIn("chat_login_ticket_jti:", session_repository)
        self.assertIn("SetNxEx", session_repository)
        self.assertLess(
            session.index("DecodeAndVerifyTicket"), session.index("ConsumeLoginTicketJti(ticket_claims.jti")
        )

        self.assertIn("bool SetNxEx", redis_header)
        self.assertIn('"NX"', redis_source)
        self.assertIn('"EX"', redis_source)

    def test_login_rate_limit_runs_before_password_hash_check_and_records_failures(self):
        source = read(AUTH_SERVICE)
        body = function_body(source, "bool AuthService::HandleUserLogin")
        compact_body = compact(body)

        self.assertIn('#include "AuthLoginRateLimiter.hpp"', source)
        self.assertLess(
            body.index("gateauthsupport::CheckLoginRateLimit(email, request)"),
            body.index("AccountPersistence::Instance().CheckPassword(email, pwd, dbUser)"),
        )
        self.assertIn("WriteRateLimited(response, root, rate_limit);", body)
        self.assertIn('root["error"] = ErrorCodes::RateLimited;', source)
        self.assertIn('root["rate_limited"] = true;', source)
        self.assertIn('root["retry_after_sec"] = rate_limit.retry_after_sec;', source)

        invalid_block = body[body.index("if (!pwd_valid)") : body.index("gateauthsupport::ClearLoginFailureCounters")]
        self.assertLess(
            invalid_block.index("gateauthsupport::RecordLoginFailure(email, request)"),
            invalid_block.index('root["error"] = ErrorCodes::PasswdInvalid'),
        )
        self.assertIn("WriteRateLimited(response, root, failure_limit);", invalid_block)
        self.assertIn("gateauthsupport::ClearLoginFailureCounters(email, request);", compact_body)

    def test_login_rate_limiter_uses_redis_fixed_window_counters(self):
        source = read(AUTH_RATE_LIMITER)
        header = read(AUTH_RATE_LIMITER_HEADER)

        for token in (
            "auth_login_fail_email:",
            "auth_login_fail_ip:",
            "AuthLoginRateLimit",
            "WindowSec",
            "EmailMaxFailures",
            "IpMaxFailures",
            'redisCommand(ctx, "GET %s", key.c_str())',
            "kincr_with_expire",
            'redisCommand(ctx, "EVAL %s 1 %s %d", kLuaIncrExpire.c_str(), key.c_str(), window_sec)',
            'redisCommand(ctx, "TTL %s", key.c_str())',
        ):
            self.assertIn(token, source)

        self.assertIn("struct AuthLoginRateLimitResult", header)
        self.assertIn("bool rate_limited = false;", header)
        self.assertIn("int retry_after_sec = 0;", header)
        self.assertIn("ResolveLoginRateLimitClientIp", header)
        self.assertIn("x-forwarded-for", source)
        self.assertIn("x-real-ip", source)

    def test_auth_public_dtos_define_input_validation_contract(self):
        dtos = read(AUTH_PUBLIC_DTOS)
        header = read(AUTH_PUBLIC_DTOS_HEADER)

        for token in (
            "enum class AuthInputField",
            "Email",
            "User",
            "Passwd",
            "Confirm",
            "Icon",
            "Nick",
            "Desc",
            "VarifyCode",
            "RefreshToken",
            "ClientVer",
            "AuthInputMaxLength",
            "IsValidAuthEmail",
            "IsValidAuthRefreshTokenShape",
            "ValidateAuthEmailRequest",
            "ValidateAuthRegisterRequest",
            "ValidateAuthResetPasswordRequest",
            "ValidateAuthLoginRequest",
            "ValidateAuthRefreshRequest",
            "ValidateAuthLogoutRequest",
            "ValidateProfileUpdateRequest",
        ):
            with self.subTest(token=token):
                self.assertIn(token, header)

        for token in (
            "constexpr std::size_t kMaxEmailLength = 254;",
            "constexpr std::size_t kMaxPasswordLength = 128;",
            "constexpr std::size_t kMaxIconLength = 512;",
            "constexpr std::size_t kMaxNickLength = 32;",
            "constexpr std::size_t kMaxDescLength = 255;",
            "constexpr std::size_t kMaxVerifyCodeLength = 16;",
            "constexpr std::size_t kMaxRefreshTokenLength = 128;",
            "constexpr std::size_t kMaxClientVersionLength = 32;",
            "return InvalidEmail();",
            "return InvalidRefreshToken();",
        ):
            with self.subTest(token=token):
                self.assertIn(token, dtos)

    def test_auth_handlers_validate_and_rate_limit_before_side_effects(self):
        source = read(AUTH_SERVICE)

        get_code = function_body(source, "bool AuthService::HandleGetVarifyCode")
        self.assertLess(get_code.index("ValidateAuthEmailRequest"), get_code.index("EnforceAuthRequestRateLimit"))
        self.assertLess(get_code.index("EnforceAuthRequestRateLimit"), get_code.index("RequestVerifyCode(email)"))

        register = function_body(source, "bool AuthService::HandleUserRegister")
        self.assertLess(register.index("ValidateAuthRegisterRequest"), register.index("EnforceAuthRequestRateLimit"))
        self.assertLess(register.index("EnforceAuthRequestRateLimit"), register.index("GetVerificationCode(email"))
        self.assertLess(
            register.index("ValidateAuthRegisterRequest"), register.index("RegisterUser(name, email, pwd, icon)")
        )

        reset = function_body(source, "bool AuthService::HandleResetPwd")
        self.assertLess(reset.index("ValidateAuthResetPasswordRequest"), reset.index("EnforceAuthRequestRateLimit"))
        self.assertLess(reset.index("EnforceAuthRequestRateLimit"), reset.index("GetVerificationCode(email"))
        self.assertLess(reset.index("ValidateAuthResetPasswordRequest"), reset.index("UpdatePassword(email, pwd)"))

        login = function_body(source, "bool AuthService::HandleUserLogin")
        self.assertLess(login.index("ValidateAuthLoginRequest"), login.index("CheckLoginRateLimit(email, request)"))
        self.assertLess(login.index("ValidateAuthLoginRequest"), login.index("CheckPassword(email, pwd, dbUser)"))

        refresh = function_body(source, "bool AuthService::HandleAuthRefresh")
        self.assertLess(refresh.index("ValidateAuthRefreshRequest"), refresh.index("EnforceAuthRequestRateLimit"))
        self.assertLess(refresh.index("EnforceAuthRequestRateLimit"), refresh.index("RotateRefreshToken"))
        self.assertIn("AuthRefreshTokenRateLimitSubject(refresh_request.refresh_token)", refresh)

        logout = function_body(source, "bool AuthService::HandleAuthLogout")
        self.assertLess(logout.index("ValidateAuthLogoutRequest"), logout.index("ResolveBearerAccessUserId"))
        self.assertLess(logout.index("ValidateAuthLogoutRequest"), logout.index("RevokeRefreshToken"))

    def test_auth_request_rate_limiter_adds_missing_endpoint_buckets_and_safe_failure_mode(self):
        source = read(AUTH_RATE_LIMITER)
        header = read(AUTH_RATE_LIMITER_HEADER)

        for token in (
            "enum class AuthRateLimitAction",
            "GetVarifyCode",
            "Register",
            "ResetPassword",
            "AuthRefresh",
            "CheckAuthRequestRateLimit",
            "BuildAuthRateLimitKey",
            "ParseAuthRateLimitRedisFailureBehavior",
            "BoundedFailOpen",
        ):
            with self.subTest(token=token):
                self.assertIn(token, header)

        for token in (
            'constexpr const char* kRequestSection = "AuthRequestRateLimit";',
            'constexpr const char* kRequestPrefix = "auth_req:";',
            "kDefaultVerifyCodeEmailMaxRequests",
            "kDefaultVerifyCodeIpMaxRequests",
            "kDefaultRegisterEmailMaxRequests",
            "kDefaultRegisterIpMaxRequests",
            "kDefaultResetPasswordEmailMaxRequests",
            "kDefaultResetPasswordIpMaxRequests",
            "kDefaultAuthRefreshSubjectMaxRequests",
            "kDefaultAuthRefreshIpMaxRequests",
            "BuildAuthRateLimitKey(action, subject_bucket",
            "BuildAuthRateLimitKey(action, AuthRateLimitBucket::Ip",
            "ConfiguredRedisFailureBehavior() != AuthRateLimitRedisFailureBehavior::BoundedFailOpen",
            "result.redis_error = true;",
            "IncrementLocalFallbackCounter",
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

    def test_success_clears_email_bucket_but_not_shared_ip_bucket(self):
        source = read(AUTH_RATE_LIMITER)
        clear_body = function_body(source, "void ClearLoginFailureCounters")

        self.assertIn("FailureKeysFor(email, request, false)", clear_body)
        self.assertIn("Do not clear the shared IP bucket on success", clear_body)
        self.assertNotIn("FailureKeysFor(email, request, true)", clear_body)

    def test_verify_code_rate_limit_counter_errors_fail_closed_before_email(self):
        source = read(VARIFY_SERVICE_IMPL)
        limiter = read(VARIFY_RATE_LIMITER)
        body = function_body(source, "grpc::Status VarifyServiceImpl::GetVarifyCode")

        self.assertIn("return Result::Error;", limiter)
        self.assertIn("IsRedisCounterError(count)", limiter)

        resolve_pos = body.index("ResolveVerifyCode(email, &code)")
        send_pos = body.index("SendEmail(email, code)")

        email_error_pos = body.index("email_result == RateLimiter::Result::Error")
        email_error_block = body[email_error_pos : body.index("if (email_result == RateLimiter::Result::RateLimited)")]
        self.assertLess(email_error_pos, resolve_pos)
        self.assertIn("varify.get_code.rate_limit_counter_error_email", email_error_block)
        self.assertIn("requests_failed.fetch_add", email_error_block)
        self.assertIn("reply->set_error(static_cast<int>(VarifyError::RedisErr));", email_error_block)
        self.assertIn("return grpc::Status::OK;", email_error_block)

        ip_error_pos = body.index("ip_result == RateLimiter::Result::Error")
        ip_error_block = body[ip_error_pos : body.index("if (ip_result == RateLimiter::Result::RateLimited)")]
        self.assertLess(ip_error_pos, resolve_pos)
        self.assertLess(ip_error_pos, send_pos)
        self.assertIn("varify.get_code.rate_limit_counter_error_ip", ip_error_block)
        self.assertIn("requests_failed.fetch_add", ip_error_block)
        self.assertIn("reply->set_error(static_cast<int>(VarifyError::RedisErr));", ip_error_block)
        self.assertIn("return grpc::Status::OK;", ip_error_block)

    def test_chat_auth_secret_is_env_injected_and_documented_fail_closed(self):
        ini_config = read(INI_CONFIG)
        auth_login = read(AUTH_LOGIN_SUPPORT)
        chat_config = read(CHAT_SESSION_CONFIG)
        docs = read(SECRET_DOC)

        self.assertIn("IniConfig::GetValue(const std::string& section, const std::string& key)", ini_config)
        self.assertIn("EnvKeyFor(section, key)", ini_config)
        self.assertIn("std::getenv(env_key.c_str())", ini_config)
        self.assertIn("MEMOCHAT_CHATAUTH_HMACSECRET", docs)

        for source in (auth_login, chat_config):
            with self.subTest(source_hash=hash(source)):
                self.assertIn('GetValue("ChatAuth", "HmacSecret")', source)
                self.assertIn("IsWellKnownDevHmacSecret", source)
                self.assertIn("MEMOCHAT_CHATAUTH_HMACSECRET", source)

    def test_env_example_documents_fail_closed_secret_model(self):
        env_example = read(ENV_EXAMPLE)

        self.assertIn("MEMOCHAT_CHATAUTH_HMACSECRET=", env_example)
        self.assertIn("MEMOCHAT_AUTH_REFRESH_PEPPER=", env_example)
        self.assertIn("MEMOCHAT_ALLOW_DEV_SECRETS=0", env_example)
        self.assertNotIn("MEMOCHAT_REQUIRE_PROD_SECRETS", env_example)
        self.assertNotIn("MEMOCHAT_ENV=dev", env_example)

    def test_production_secret_guard_fails_closed_before_accepting_traffic(self):
        auth_secret = read(AUTH_SECRET)
        gate_domain = read(GATE_DOMAIN_SERVER)
        chat_server = read(CHAT_SERVER)

        self.assertIn("IsProductionSecretEnforcementEnabled", auth_secret)
        self.assertIn("MEMOCHAT_ALLOW_DEV_SECRETS", auth_secret)
        self.assertIn("return !IsDevSecretsAllowed();", auth_secret)
        self.assertIn("RequireNonDefaultChatAuthSecretInProduction", auth_secret)
        self.assertIn("ChatAuth.HmacSecret must be non-default; set", auth_secret)
        self.assertIn("ChatAuth.HmacSecret must be at least 32 bytes", auth_secret)

        self.assertIn('#include "auth/AuthSecret.hpp"', gate_domain)
        self.assertIn('GetValue("ChatAuth", "HmacSecret")', gate_domain)
        self.assertIn("RequireNonDefaultChatAuthSecretInProduction(service_name, chat_auth_secret)", gate_domain)
        self.assertLess(
            gate_domain.index("RequireNonDefaultChatAuthSecretInProduction"),
            gate_domain.index("std::make_shared<CServer>(ioc, port)->Start();"),
        )

        self.assertIn('#include "auth/AuthSecret.hpp"', chat_server)
        self.assertIn('GetValue("ChatAuth", "HmacSecret")', chat_server)
        self.assertIn('RequireNonDefaultChatAuthSecretInProduction("ChatServer", chat_auth_secret)', chat_server)
        self.assertLess(
            chat_server.index("RequireNonDefaultChatAuthSecretInProduction"),
            chat_server.index("builder.AddListeningPort"),
        )

    def test_service_ini_files_do_not_commit_weak_dependency_credentials(self):
        for path in SERVER_CORE.rglob("*.ini"):
            text = read(path)
            with self.subTest(path=path.relative_to(REPO_ROOT).as_posix()):
                self.assertIsNone(re.search(r"(?m)^(?:Passwd|Password)\s*=\s*(?:123456|password)\s*$", text))
                self.assertNotIn("mongodb://memochat_app:123456@", text)

    def test_ci_scans_for_secrets_and_runtime_configs_remain_ignored(self):
        ci = read(CI_WORKFLOW)
        gitignore = read(GITIGNORE)

        self.assertIn("secret-scan:", ci)
        self.assertIn("Reject known weak dependency credentials", ci)
        self.assertIn("git grep -nE", ci)
        for token in (
            "weak_password=",
            "weak_mongodb=",
            "disabled_tls=",
            "mutable_prod_tag=",
            "Passwd|Password",
            "mongodb://",
            "newTag:[[:space:]]*latest",
            ":!.github/workflows/ci.yml",
        ):
            with self.subTest(token=token):
                self.assertIn(token, ci)

        self.assertIn("Memo_ops/runtime/", gitignore)
        self.assertIn("infra/Memo_ops/runtime/", gitignore)
        self.assertIn("/.claude/worktrees/", gitignore)
        self.assertNotIn("!Memo_ops/runtime/services/**/config.ini", gitignore)
        self.assertNotIn("!infra/Memo_ops/runtime/services/**/config.ini", gitignore)

    def test_refresh_tokens_are_opaque_hashed_and_never_stored_plaintext(self):
        refresh_source = read(REFRESH_TOKEN)
        for token in (
            "randombytes_buf",
            "selector",
            "verifier",
            "sodium-generichash-keyed-v2$",
            "MEMOCHAT_AUTH_REFRESH_PEPPER",
            "crypto_generichash",
            "sodium_memcmp",
            "HashRefreshTokenMetadata",
        ):
            with self.subTest(token=token):
                self.assertIn(token, refresh_source)

        for path in (BASELINE_MIGRATION, ACCOUNT_SCHEMA_MIGRATION, REFRESH_TOKEN_MIGRATION, BUSINESS_INIT):
            with self.subTest(path=path):
                sql = read(path)
                self.assertIn("auth_refresh_token", sql)
                self.assertIn("selector text NOT NULL", sql)
                self.assertIn("verifier_hash text NOT NULL", sql)
                self.assertIn("rotated_at timestamptz", sql)
                self.assertIn("revoked_at timestamptz", sql)
                self.assertIn("replaced_by_selector text", sql)
                self.assertIn("idx_auth_refresh_token_uid_active", sql)
                self.assertNotRegex(sql, r"\brefresh_token\s+text\b")
                self.assertNotRegex(sql, r"\bverifier\s+text\b")

    def test_refresh_token_lifecycle_rotates_transactionally_and_revokes_on_password_change(self):
        dao = read(POSTGRES_DAO_ACCOUNT)
        dao_header = read(POSTGRES_DAO_HEADER)
        account = read(ACCOUNT_PERSISTENCE)
        account_header = read(ACCOUNT_PERSISTENCE_HEADER)
        auth_service = read(AUTH_SERVICE)

        for token in (
            "IssueRefreshToken",
            "RotateRefreshToken",
            "RevokeRefreshToken",
            "RevokeAllRefreshTokensForUid",
        ):
            with self.subTest(token=token):
                self.assertIn(token, dao_header)
                self.assertIn(token, account_header)
                self.assertIn(token, dao)
                self.assertIn(token, account)

        self.assertIn("SELECT uid, verifier_hash, expires_at <= now() AS expired", dao)
        self.assertIn("WHERE selector = $1 FOR UPDATE", dao)
        self.assertIn("VerifyRefreshTokenVerifier(verifier, stored_hash)", dao)
        self.assertIn("rotated_at = now()", dao)
        self.assertIn("replaced_by_selector", dao)
        self.assertIn("revoked_at = COALESCE(revoked_at, now())", dao)
        self.assertIn("return RefreshTokenRotationStatus::Reused", dao)
        self.assertIn('"UPDATE " + PgTable("auth_refresh_token")', dao)

        update_pwd_body = function_body(dao, "bool PostgresDao::UpdatePwd")
        self.assertIn("RETURNING uid", update_pwd_body)
        self.assertIn('PgTable("auth_refresh_token")', update_pwd_body)
        self.assertIn("SET revoked_at = now()", update_pwd_body)

        self.assertIn("RotateRefreshToken(", auth_service)
        self.assertIn("IssueLoginSessionForUser", auth_service)
        self.assertIn('root["refresh_token"]', auth_service)
        self.assertRegex(
            auth_service,
            r"const auto rotated\s*=\s*account::AccountPersistence::Instance\(\)\.RotateRefreshToken\s*\(",
        )

    def test_password_reset_deletes_redis_http_token_after_successful_update(self):
        auth_service = read(AUTH_SERVICE)
        account_header = read(ACCOUNT_PERSISTENCE_HEADER)
        account = read(ACCOUNT_PERSISTENCE)
        reset_body = function_body(auth_service, "bool AuthService::HandleResetPwd")

        self.assertIn("FindUserByEmail", account_header)
        self.assertIn("PostgresMgr::GetInstance()->TestProcedure(email, uid, name)", account)
        self.assertLess(reset_body.index("account_persistence.UpdatePassword"), reset_body.index("FindUserByEmail"))
        self.assertIn("AuthCache::Instance().DeleteHttpToken(reset_uid);", reset_body)
        self.assertIn("gateauthsupport::InvalidateLoginCacheByUid(reset_uid);", reset_body)

    def test_auth_refresh_route_contract_is_registered_and_uses_login_response(self):
        route_module = read(AUTH_ROUTE_MODULE)
        route_registration = read(AUTH_ROUTE_REGISTRATION_MODULE)
        route_schema_module = read(AUTH_ROUTE_SCHEMA_MODULE)
        route_schemas = read(AUTH_ROUTE_SCHEMAS)
        dtos = read(AUTH_PUBLIC_DTOS)
        dto_header = read(AUTH_PUBLIC_DTOS_HEADER)
        auth_service_header = read(SERVER_CORE / "AccountShared/domain/services/auth/AuthService.hpp")

        self.assertIn('return "/auth/refresh";', route_registration)
        self.assertIn("AuthRefreshPath()", route_module)
        self.assertIn("HandleAuthRefresh(request, response)", route_module)
        self.assertIn("AuthRefreshRequestDto", route_schema_module)
        self.assertIn("AuthLoginResponseDto", route_schema_module)
        self.assertIn("auth.refresh", route_schema_module)
        self.assertIn("/auth/refresh", route_schema_module)

        self.assertIn("struct AuthRefreshRequestDto", dto_header)
        self.assertIn("struct AuthLoginResponseDto", dto_header)
        self.assertIn("refresh_token", dto_header)
        self.assertIn("DecodeAuthRefreshRequest", dto_header)
        self.assertIn("AuthRefreshRequestFromJsonValue", dtos)
        self.assertIn('glaze_safe_get<std::string>(root, "refresh_token"', dtos)
        self.assertIn("bool HandleAuthRefresh", auth_service_header)

    def test_auth_logout_route_revokes_refresh_and_http_tokens(self):
        route_module = read(AUTH_ROUTE_MODULE)
        route_registration = read(AUTH_ROUTE_REGISTRATION_MODULE)
        route_schema_module = read(AUTH_ROUTE_SCHEMA_MODULE)
        route_schemas = read(AUTH_ROUTE_SCHEMAS)
        dtos = read(AUTH_PUBLIC_DTOS)
        dto_header = read(AUTH_PUBLIC_DTOS_HEADER)
        auth_service = read(AUTH_SERVICE)
        auth_service_header = read(SERVER_CORE / "AccountShared/domain/services/auth/AuthService.hpp")

        self.assertIn('return "/auth/logout";', route_registration)
        self.assertIn("AuthLogoutPath()", route_module)
        self.assertIn("HandleAuthLogout(request, response)", route_module)
        self.assertIn("AuthLogoutRequestDto", route_schema_module)
        self.assertIn("AuthLogoutResponseDto", route_schema_module)
        self.assertIn("auth.logout", route_schema_module)
        self.assertIn("/auth/logout", route_schema_module)
        self.assertIn("AuthLogoutRequestDto, gateauthsupport::AuthLogoutResponseDto", route_schemas)

        self.assertIn("struct AuthLogoutRequestDto", dto_header)
        self.assertIn("struct AuthLogoutResponseDto", dto_header)
        self.assertIn("DecodeAuthLogoutRequest", dto_header)
        self.assertIn("AuthLogoutRequestFromJsonValue", dtos)
        for token in (
            'glaze_safe_get<std::string>(root, "refresh_token"',
            'glaze_safe_get<bool>(root, "all_devices"',
        ):
            with self.subTest(token=token):
                self.assertIn(token, dtos)

        self.assertIn("bool HandleAuthLogout", auth_service_header)
        logout_body = function_body(auth_service, "bool AuthService::HandleAuthLogout")
        self.assertIn("memochat::auth::ResolveBearerAccessUserId(request, authenticated_uid)", logout_body)
        self.assertIn("refresh_uid == authenticated_uid", logout_body)
        self.assertNotIn("logout_request.uid", logout_body)
        self.assertNotIn("logout_request.access_token", logout_body)
        self.assertNotIn("AuthCache::Instance().GetHttpToken", logout_body)
        self.assertNotIn("provided_token != logout_request.token", logout_body)
        self.assertIn("RevokeAllRefreshTokensForUid", logout_body)
        self.assertIn("RevokeRefreshToken", logout_body)
        self.assertIn("AuthCache::Instance().DeleteHttpToken", logout_body)
        self.assertIn("AuthLogoutResponseDto logout_response", logout_body)
        self.assertNotIn('root["refresh_token"]', logout_body)

    def test_logging_redaction_covers_current_credential_keys_and_query_tokens(self):
        redaction = read(REDACTION_MODULE)
        h1 = read(H1_HTTP_CONNECTION)
        h3 = read(H3_HTTP_LISTENER)

        for token in (
            '"login_ticket"',
            '"pwd"',
            '"password_hash"',
            '"api_key"',
            '"client_secret"',
            '"x-token"',
            '"set-cookie"',
            'EndsWithAsciiLiteral(key, size, "_token")',
            'ContainsAsciiLiteral(key, size, "secret")',
        ):
            with self.subTest(token=token):
                self.assertIn(token, redaction)

        self.assertIn("RequestPathForLog", h1)
        self.assertIn("target.find('?')", h1)
        self.assertIn('fields["route"] = RequestPathForLog(target_sv);', h1)
        self.assertIn("RequestPathForLog", h3)
        self.assertIn("path.find('?')", h3)
        self.assertIn('fields["path"] = RequestPathForLog(stream_ctx->Path);', h3)

    def test_start_all_services_rejects_stale_pid_and_non_runtime_port_owner(self):
        script = read(START_ALL_SERVICES)

        self.assertIn("service_pid_matches_runtime", script)
        self.assertIn('readlink "/proc/${pid}/cwd"', script)
        self.assertIn('readlink "/proc/${pid}/exe"', script)
        self.assertIn('rm -f -- "$pid_file"', script)
        self.assertIn('pid_running "$pid_file" "$svc_dir" "$exe_path"', script)
        self.assertIn('service_pid_matches_runtime "$port_owner" "$svc_dir" "$exe_path"', script)
        self.assertIn("port ${port} is already listening by non-matching pid", script)
        self.assertIn('echo "  [X] ${svc_name}: port ${port} is already listening', script)
