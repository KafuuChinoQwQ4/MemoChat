import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_SHARED = SERVER_CORE / "GateShared"
ACCOUNT_SHARED = SERVER_CORE / "AccountShared"
AI_GATEWAY_SERVICE = SERVER_CORE / "AIGatewayService"
AI_SERVER = SERVER_CORE / "AIServer"
CALL_SERVICE = SERVER_CORE / "CallService"
CHAT_SERVER = SERVER_CORE / "ChatServer"
MEDIA_SERVICE = SERVER_CORE / "MediaService"
MOMENTS_SERVICE = SERVER_CORE / "MomentsService"
R18_SERVICE = SERVER_CORE / "R18Service"
VARIFY_SERVER = SERVER_CORE / "VarifyServer"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class CxxModulesContractTest(unittest.TestCase):
    def test_linux_gcc16_preset_uses_project_owned_cmi_management(self):
        presets = json.loads(read(REPO_ROOT / "CMakePresets.json"))
        linux_full = next(preset for preset in presets["configurePresets"] if preset["name"] == "linux-full-gcc16")
        cache = linux_full["cacheVariables"]

        self.assertEqual(cache["CMAKE_CXX_COMPILER"], "/usr/bin/g++-16")
        self.assertEqual(cache["CMAKE_CXX_STANDARD"], "26")
        self.assertEqual(cache["CMAKE_CXX_SCAN_FOR_MODULES"], "OFF")
        self.assertEqual(cache["MEMOCHAT_ENABLE_GNU_MODULES"], "ON")

    def test_cmake_helper_builds_named_cmis_with_gnu_mapper(self):
        helper = read(REPO_ROOT / "cmake" / "MemoChatModules.cmake")

        self.assertIn("function(memochat_enable_gnu_modules target_name)", helper)
        self.assertIn("-fmodules", helper)
        self.assertIn("-fmodule-mapper=${_mapper_file}", helper)
        self.assertIn("gcm.cache", helper)
        self.assertIn("OBJECT_DEPENDS", helper)
        self.assertIn("set_property(SOURCE", helper)
        self.assertIn("MODULE_INCLUDE_DIRS", helper)
        self.assertIn("MODULE_DEFINITIONS", helper)
        self.assertIn("MODULE_OPTIONS", helper)
        self.assertIn("MODULE_DEPENDS", helper)
        self.assertIn("CONSUMER_SOURCES", helper)
        self.assertIn("add_custom_target(${target_name}_cxx_modules", helper)

    def test_cmake_helper_uses_stamp_deps_and_depfile_filter_for_incremental_modules(self):
        helper = read(REPO_ROOT / "cmake" / "MemoChatModules.cmake")

        self.assertIn(
            "function(memochat_prepare_gnu_std_modules out_cmis out_stamps out_mapper_lines out_target)", helper
        )
        self.assertIn("set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_STAMPS", helper)
        self.assertIn("UpdateModuleStamp.cmake", helper)
        self.assertIn("CXX_COMPILER_LAUNCHER", helper)
        self.assertIn("gcc-modules-depfile-filter.sh", helper)
        self.assertIn('OBJECT_DEPENDS "${_module_stamps};${_mapper_file}"', helper)
        self.assertIn('DEPENDS "${_module_source}" "${_mapper_file}" ${_std_module_stamps}', helper)
        self.assertNotIn('OBJECT_DEPENDS "${_module_cmis};${_mapper_file}"', helper)

    @unittest.skipUnless(os.name == "posix", "GNU modules depfile filter is a POSIX launcher")
    def test_gnu_module_depfile_filter_removes_ninja_poison_module_deps(self):
        depfile_filter = REPO_ROOT / "cmake" / "gcc-modules-depfile-filter.sh"

        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            fake_compiler = tmpdir / "fake-gxx"
            depfile = tmpdir / "consumer.o.d"
            depfile_joined = tmpdir / "consumer-joined.o.d"

            fake_compiler.write_text(
                """#!/usr/bin/env bash
set -euo pipefail
depfile=""
args=("$@")
for ((i = 0; i < ${#args[@]}; ++i)); do
    arg="${args[$i]}"
    case "$arg" in
        -MF)
            depfile="${args[$((i + 1))]}"
            ;;
        -MF*)
            depfile="${arg#-MF}"
            ;;
    esac
done
cat > "$depfile" <<'DEPFILE'
consumer.o: consumer.cpp \\
  /tmp/build/gcm.cache/memochat/gate/domain_runtime_algorithms.gcm \\
  memochat.gate.domain_runtime_algorithms.c++-module \\
  /tmp/include/header.hpp
CXX_IMPORTS += memochat.gate.domain_runtime_algorithms.c++-module \\
  /tmp/build/gcm.cache/memochat/gate/domain_runtime_algorithms.gcm
DEPFILE
""",
                encoding="utf-8",
            )
            fake_compiler.chmod(0o755)

            subprocess.run(
                [str(depfile_filter), str(fake_compiler), "-c", "consumer.cpp", "-MF", str(depfile)],
                check=True,
            )
            subprocess.run(
                [str(depfile_filter), str(fake_compiler), "-c", "consumer.cpp", f"-MF{depfile_joined}"],
                check=True,
            )

            for sanitized in (read(depfile), read(depfile_joined)):
                self.assertIn("consumer.cpp", sanitized)
                self.assertIn("/tmp/include/header.hpp", sanitized)
                self.assertNotIn(".c++-module", sanitized)
                self.assertNotIn(".gcm", sanitized)
                self.assertNotIn("CXX_IMPORTS", sanitized)
                self.assertIsNone(re.search(r"^[ \t]*\\$", sanitized, flags=re.M))

    def test_lua_embed_helper_preserves_cpp_semicolons_in_generated_headers(self):
        helper = read(REPO_ROOT / "cmake" / "EmbedLuaScripts.cmake")

        self.assertIn('set(CONTENT "")', helper)
        self.assertIn("string(APPEND CONTENT", helper)
        self.assertIn('")${DELIM}\\";\\n\\n"', helper)
        self.assertIn('file(WRITE "${OUT_FILE}" "${CONTENT}")', helper)
        self.assertNotIn('file(WRITE "${OUT_FILE}" ${CONTENT})', helper)

    def test_gate_runtime_core_declares_module_interfaces_and_import_consumers(self):
        cmake = read(GATE_SHARED / "CMakeLists.txt")

        self.assertIn("if(MEMOCHAT_ENABLE_GNU_MODULES)", cmake)
        self.assertIn("memochat_enable_gnu_modules(GateRuntimeCore", cmake)
        self.assertIn("memochat.gate.domain_runtime_algorithms=modules/cxx_modules/GateDomainRuntime.cppm", cmake)
        self.assertIn("memochat.gate.module_probe=modules/cxx_modules/GateModuleProbe.cppm", cmake)
        self.assertIn("memochat.gate.routing_algorithms=modules/cxx_modules/GateRouting.cppm", cmake)
        self.assertIn("app/GateDomainRuntime.cpp", cmake)
        self.assertIn("LogicSystem.cpp", cmake)
        self.assertIn("modules/cxx_modules/GateModuleProbeConsumer.cpp", cmake)
        self.assertIn("routing/RouteRegistry.cpp", cmake)

    def test_module_interface_and_consumer_keep_import_boundary_explicit(self):
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateModuleProbe.cppm")
        consumer = read(GATE_SHARED / "modules" / "cxx_modules" / "GateModuleProbeConsumer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.gate\.module_probe\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::gate::modules::cxx", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.gate\.module_probe\s*;", consumer, flags=re.M))
        self.assertIn("gateshared_module_probe_gtest", test_cmake)
        self.assertIn("if(MEMOCHAT_ENABLE_GNU_MODULES)", test_cmake)
        self.assertIn("GateRuntimeCore", test_cmake)

    def test_route_registry_uses_project_module_for_routing_algorithms(self):
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateRouting.cppm")
        consumer = read(GATE_SHARED / "routing" / "RouteRegistry.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.gate\.routing_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::gate::routing::modules", module_interface)
        self.assertIn("NormalizeMethodAscii", module_interface)
        self.assertIn("MatchesRoutePrefix", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.gate\.routing_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::NormalizeMethodAscii", consumer)
        self.assertIn("modules::MatchesRoutePrefix", consumer)
        self.assertIn("gateshared_route_registry_gtest", test_cmake)

    def test_logic_system_reuses_project_module_for_legacy_prefix_routes(self):
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateRouting.cppm")
        consumer = read(GATE_SHARED / "LogicSystem.cpp")

        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.gate\.routing_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.gate\.routing_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("memochat::gate::routing::modules::MatchesRoutePrefix", consumer)
        self.assertNotIn("path.rfind(prefix, 0)", consumer)

    def test_gate_domain_runtime_uses_project_module_for_startup_algorithms(self):
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateDomainRuntime.cppm")
        consumer = read(GATE_SHARED / "app" / "GateDomainRuntime.cpp")
        domain_server = read(GATE_SHARED / "app" / "GateDomainServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.domain_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::domain::modules", module_interface)
        self.assertIn("SelectListenPort", module_interface)
        self.assertIn("NormalizeIoThreadCount", module_interface)
        self.assertIn("SelectWorkerThreadCount", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.domain_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SelectListenPort", consumer)
        self.assertIn("modules::NormalizeIoThreadCount", consumer)
        self.assertIn("modules::SelectWorkerThreadCount", consumer)
        self.assertIn("SelectGateDomainListenPort", domain_server)
        self.assertIn("NormalizeGateDomainIoThreads", domain_server)
        self.assertIn("SelectGateDomainWorkerThreads", domain_server)
        self.assertNotRegex(domain_server, r"^\s*import\s+memochat\.gate\.domain_runtime_algorithms\s*;")
        self.assertIn("gateshared_domain_runtime_gtest", test_cmake)

    def test_health_route_module_uses_project_module_for_probe_metadata(self):
        cmake = read(GATE_SHARED / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "HealthRoute.cppm")
        consumer = read(GATE_SHARED / "modules" / "health" / "HealthRouteModule.cpp")
        route_registry = read(GATE_SHARED / "routing" / "RouteRegistry.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "HealthRouteAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat.gate.health_route_algorithms=modules/cxx_modules/HealthRoute.cppm", cmake)
        self.assertIn("modules/health/HealthRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.health_route_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HealthPath", module_interface)
        self.assertIn("ReadinessPath", module_interface)
        self.assertIn("SuccessfulProbeStatus", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.health_route_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("health_modules::HealthPath", consumer)
        self.assertIn("health_modules::ReadinessPath", consumer)
        self.assertIn("health_modules::JsonContentType", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.gate\.health_route_algorithms\s*;", route_registry, flags=re.M)
        )
        self.assertIn("HealthRouteAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.health_route_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.health_route_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_http3_legacy_routes_use_project_module_for_route_table(self):
        cmake = read(GATE_SHARED / "transports" / "h3" / "CMakeLists.txt")
        module_interface = read(
            GATE_SHARED / "transports" / "h3" / "legacy_routes" / "cxx_modules" / "H3LegacyRoute.cppm"
        )
        consumer = read(GATE_SHARED / "transports" / "h3" / "legacy_routes" / "GateHttp3ServiceRoutes.cpp")
        listener = read(GATE_SHARED / "transports" / "h3" / "listener" / "GateHttp3Listener.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "H3LegacyRouteAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateServerHttp3", cmake)
        self.assertIn(
            "memochat.gate.h3_legacy_route_algorithms=legacy_routes/cxx_modules/H3LegacyRoute.cppm",
            cmake,
        )
        self.assertIn("legacy_routes/GateHttp3ServiceRoutes.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.h3_legacy_route_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("GetRouteCount", module_interface)
        self.assertIn("PostRouteCount", module_interface)
        self.assertIn("RouteNotFoundStatus", module_interface)
        self.assertIn('return "/user_login"', module_interface)
        self.assertIn('return "/api/moments/comment/like"', module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.h3_legacy_route_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("h3_legacy_modules::GetRoutePathAt", consumer)
        self.assertIn("h3_legacy_modules::PostRoutePathAt", consumer)
        self.assertIn("h3_legacy_modules::RouteNotFoundStatus", consumer)
        self.assertNotIn('logic.RegPost("/user_login"', consumer)
        self.assertNotIn('logic.RegPost("/api/moments/comment/like"', consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.gate\.h3_legacy_route_algorithms\s*;", listener, flags=re.M)
        )
        self.assertIn("gateshared_h3_legacy_route_algorithms_gtest", test_cmake)
        self.assertIn("H3LegacyRouteAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.h3_legacy_route_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.h3_legacy_route_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_runtime_core_imports_worker_pool_algorithms_module(self):
        cmake = read(GATE_SHARED / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateWorkerPool.cppm")
        consumer = read(GATE_SHARED / "GateWorkerPool.cpp")
        cserver = read(GATE_SHARED / "CServer.cpp")
        http_connection = read(GATE_SHARED / "HttpConnection.cpp")
        domain_server = read(GATE_SHARED / "app" / "GateDomainServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "GateWorkerPoolAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat.gate.worker_pool_algorithms=modules/cxx_modules/GateWorkerPool.cppm", cmake)
        self.assertIn("GateWorkerPool.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.worker_pool_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("SelectWorkerThreadCount", module_interface)
        self.assertIn("ShouldJoinWorkerPool", module_interface)
        self.assertIn("ShouldStopWorkerPool", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.worker_pool_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("worker_pool_modules::SelectWorkerThreadCount", consumer)
        self.assertIn("worker_pool_modules::ShouldJoinWorkerPool", consumer)
        self.assertIn("worker_pool_modules::ShouldStopWorkerPool", consumer)
        for non_consumer in (cserver, http_connection, domain_server):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.worker_pool_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_worker_pool_algorithms_gtest", test_cmake)
        self.assertIn("GateWorkerPoolAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.worker_pool_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.worker_pool_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_runtime_core_imports_h1_route_adapter_algorithms_module(self):
        cmake = read(GATE_SHARED / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "adapters" / "h1" / "cxx_modules" / "H1RouteAdapter.cppm")
        consumer = read(GATE_SHARED / "adapters" / "h1" / "H1RouteAdapter.cpp")
        http_connection = read(GATE_SHARED / "HttpConnection.cpp")
        cserver = read(GATE_SHARED / "CServer.cpp")
        domain_server = read(GATE_SHARED / "app" / "GateDomainServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "H1RouteAdapterAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn(
            "memochat.gate.h1_route_adapter_algorithms=adapters/h1/cxx_modules/H1RouteAdapter.cppm",
            cmake,
        )
        self.assertIn("adapters/h1/H1RouteAdapter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.h1_route_adapter_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldReadConnectionFields", module_interface)
        self.assertIn("ShouldSetContentType", module_interface)
        self.assertIn("ShouldApplyFileResponse", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.h1_route_adapter_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("h1_modules::ShouldReadConnectionFields", consumer)
        self.assertIn("h1_modules::ShouldSetContentType", consumer)
        self.assertIn("h1_modules::ShouldApplyFileResponse", consumer)
        for non_consumer in (http_connection, cserver, domain_server):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.h1_route_adapter_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_h1_route_adapter_algorithms_gtest", test_cmake)
        self.assertIn("H1RouteAdapterAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.h1_route_adapter_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.h1_route_adapter_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_runtime_core_imports_http_json_support_algorithms_module(self):
        cmake = read(GATE_SHARED / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "modules" / "cxx_modules" / "GateHttpJsonSupport.cppm")
        consumer = read(GATE_SHARED / "GateHttpJsonSupport.cpp")
        http_connection = read(GATE_SHARED / "HttpConnection.cpp")
        cserver = read(GATE_SHARED / "CServer.cpp")
        domain_server = read(GATE_SHARED / "app" / "GateDomainServer.cpp")
        h1_adapter = read(GATE_SHARED / "adapters" / "h1" / "H1RouteAdapter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "GateHttpJsonSupportAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn(
            "memochat.gate.http_json_support_algorithms=modules/cxx_modules/GateHttpJsonSupport.cppm",
            cmake,
        )
        self.assertIn("GateHttpJsonSupport.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.http_json_support_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("JsonContentType", module_interface)
        self.assertIn("EmptyTraceId", module_interface)
        self.assertIn("ShouldWriteJsonParseError", module_interface)
        self.assertIn("ShouldAttachTraceId", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.http_json_support_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("http_json_modules::JsonContentType", consumer)
        self.assertIn("http_json_modules::EmptyTraceId", consumer)
        self.assertIn("http_json_modules::ShouldWriteJsonParseError", consumer)
        self.assertIn("http_json_modules::ShouldAttachTraceId(parsed_json)", consumer)
        for non_consumer in (http_connection, cserver, domain_server, h1_adapter):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.gate\.http_json_support_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("gateshared_http_json_support_algorithms_gtest", test_cmake)
        self.assertIn("GateHttpJsonSupportAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.http_json_support_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.gate\.http_json_support_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_gate_infra_core_imports_user_token_validator_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "support" / "cxx_modules" / "UserTokenValidator.cppm")
        consumer = read(GATE_SHARED / "core" / "support" / "UserTokenValidator.cpp")
        redis_mgr = read(GATE_SHARED / "core" / "cache" / "RedisMgr.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "UserTokenValidatorAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.user_token_validator_algorithms=support/cxx_modules/UserTokenValidator.cppm",
            cmake,
        )
        self.assertIn("support/UserTokenValidator.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.user_token_validator_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldValidateUserToken", module_interface)
        self.assertIn("ShouldAcceptStoredUserToken", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.user_token_validator_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::ShouldValidateUserToken", consumer)
        self.assertIn("modules::ShouldAcceptStoredUserToken", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.gate\.user_token_validator_algorithms\s*;", redis_mgr, flags=re.M)
        )
        self.assertIn("gateshared_user_token_validator_algorithms_gtest", test_cmake)
        self.assertIn("UserTokenValidatorAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.user_token_validator_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.gate\.user_token_validator_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_gate_infra_core_imports_redis_pipeline_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "cache" / "cxx_modules" / "RedisPipeline.cppm")
        consumer = read(GATE_SHARED / "core" / "cache" / "RedisPipelines.cpp")
        redis_mgr = read(GATE_SHARED / "core" / "cache" / "RedisMgr.cpp")
        token_validator = read(GATE_SHARED / "core" / "support" / "UserTokenValidator.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "RedisPipelineAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.redis_pipeline_algorithms=cache/cxx_modules/RedisPipeline.cppm",
            cmake,
        )
        self.assertIn("cache/RedisPipelines.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.redis_pipeline_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::redis_pipeline::modules", module_interface)
        self.assertIn("LoginProfilePrefix", module_interface)
        self.assertIn("LoginProfileUidPrefix", module_interface)
        self.assertIn("UserTokenPrefix", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.redis_pipeline_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("redis_pipeline_modules::LoginProfilePrefix", consumer)
        self.assertIn("redis_pipeline_modules::LoginProfileUidPrefix", consumer)
        self.assertIn("redis_pipeline_modules::UserTokenPrefix", consumer)
        self.assertNotIn('#include "const.hpp"', consumer)
        for non_consumer in (redis_mgr, token_validator):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.redis_pipeline_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_redis_pipeline_algorithms_gtest", test_cmake)
        self.assertIn("RedisPipelineAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.redis_pipeline_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.redis_pipeline_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_infra_core_imports_redis_mgr_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "cache" / "cxx_modules" / "RedisMgr.cppm")
        consumer = read(GATE_SHARED / "core" / "cache" / "RedisMgr.cpp")
        pipelines = read(GATE_SHARED / "core" / "cache" / "RedisPipelines.cpp")
        token_validator = read(GATE_SHARED / "core" / "support" / "UserTokenValidator.cpp")
        test_consumer = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "RedisMgrAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.redis_mgr_algorithms=cache/cxx_modules/RedisMgr.cppm",
            cmake,
        )
        self.assertIn("cache/RedisMgr.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.redis_mgr_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::redis_mgr::modules", module_interface)
        self.assertIn("NormalizePoolSize", module_interface)
        self.assertIn("IsStatusOk", module_interface)
        self.assertIn("IsPositiveIntegerReply", module_interface)
        self.assertIn("IsNonEmptyStringReply", module_interface)
        self.assertIn("ShouldCollectPipelineReply", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.gate\.redis_mgr_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("redis_mgr_modules::NormalizePoolSize", consumer)
        self.assertIn("redis_mgr_modules::IsStatusOk", consumer)
        self.assertIn("redis_mgr_modules::IsPositiveIntegerReply", consumer)
        self.assertIn("redis_mgr_modules::ShouldCollectPipelineReply", consumer)
        for non_consumer in (pipelines, token_validator):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.redis_mgr_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_redis_mgr_algorithms_gtest", test_cmake)
        self.assertIn("RedisMgrAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.redis_mgr_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.redis_mgr_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_infra_core_imports_postgres_dao_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "persistence" / "cxx_modules" / "PostgresDao.cppm")
        consumer = read(GATE_SHARED / "core" / "persistence" / "PostgresDao.cpp")
        account_dao = read(GATE_SHARED / "core" / "persistence" / "PostgresDaoAccount.cpp")
        postgres_mgr = read(GATE_SHARED / "core" / "persistence" / "PostgresMgr.cpp")
        mongo_dao = read(GATE_SHARED / "core" / "persistence" / "MongoDao.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "PostgresDaoAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.postgres_dao_algorithms=persistence/cxx_modules/PostgresDao.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDao.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.postgres_dao_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::postgres_dao::modules", module_interface)
        self.assertIn("DefaultSchema", module_interface)
        self.assertIn("DefaultSslMode", module_interface)
        self.assertIn("NormalizePoolSize", module_interface)
        self.assertIn("ShouldFetchUserProfiles", module_interface)
        self.assertIn("ShouldUseAccountPostgresSection", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.postgres_dao_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_dao_modules::DefaultSchema", consumer)
        self.assertIn("postgres_dao_modules::DefaultSslMode", consumer)
        self.assertIn("postgres_dao_modules::NormalizePoolSize", consumer)
        self.assertIn("postgres_dao_modules::ShouldUseAccountPostgresSection", consumer)
        for non_consumer in (account_dao, postgres_mgr, mongo_dao):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.postgres_dao_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_postgres_dao_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.postgres_dao_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.postgres_dao_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_infra_core_imports_postgres_dao_account_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "persistence" / "cxx_modules" / "PostgresDaoAccount.cppm")
        consumer = read(GATE_SHARED / "core" / "persistence" / "PostgresDaoAccount.cpp")
        postgres_dao = read(GATE_SHARED / "core" / "persistence" / "PostgresDao.cpp")
        postgres_mgr = read(GATE_SHARED / "core" / "persistence" / "PostgresMgr.cpp")
        mongo_dao = read(GATE_SHARED / "core" / "persistence" / "MongoDao.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "PostgresDaoAccountAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.postgres_dao_account_algorithms=persistence/cxx_modules/PostgresDaoAccount.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoAccount.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.postgres_dao_account_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::postgres_dao_account::modules", module_interface)
        self.assertIn("DefaultUserIcon", module_interface)
        self.assertIn("UserPublicIdMaxAttempts", module_interface)
        for removed_helper in ("LegacyXorModulo", "LegacyXorCode"):
            self.assertNotIn(removed_helper, module_interface)
        self.assertIn("ShouldAcceptGeneratedPublicId", module_interface)
        self.assertIn("ShouldUseAccountBridge", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.postgres_dao_account_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_dao_account_modules::DefaultSchema", consumer)
        self.assertIn("postgres_dao_account_modules::DefaultUserIcon", consumer)
        self.assertIn("postgres_dao_account_modules::UserPublicIdMaxAttempts", consumer)
        self.assertIn("postgres_dao_account_modules::ShouldUseAccountBridge", consumer)
        self.assertNotIn("postgres_dao_account_modules::LegacyXorCode", consumer)
        self.assertNotIn("DecodeLegacyXorPwd", consumer)
        for non_consumer in (postgres_dao, postgres_mgr, mongo_dao):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.gate\.postgres_dao_account_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("gateshared_postgres_dao_account_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoAccountAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.postgres_dao_account_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.gate\.postgres_dao_account_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_gate_infra_core_imports_mongo_dao_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "persistence" / "cxx_modules" / "MongoDao.cppm")
        consumer = read(GATE_SHARED / "core" / "persistence" / "MongoDao.cpp")
        mongo_mgr = read(GATE_SHARED / "core" / "persistence" / "MongoMgr.cpp")
        postgres_dao = read(GATE_SHARED / "core" / "persistence" / "PostgresDao.cpp")
        postgres_dao_account = read(GATE_SHARED / "core" / "persistence" / "PostgresDaoAccount.cpp")
        postgres_mgr = read(GATE_SHARED / "core" / "persistence" / "PostgresMgr.cpp")
        test_consumer = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "MongoDaoAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.mongo_dao_algorithms=persistence/cxx_modules/MongoDao.cppm",
            cmake,
        )
        self.assertIn("persistence/MongoDao.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.mongo_dao_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::mongo_dao::modules", module_interface)
        self.assertIn("ParseBoolText", module_interface)
        self.assertIn("DefaultMomentsCollection", module_interface)
        self.assertIn("IsEnabled", module_interface)
        self.assertIn("HasRequiredConfig", module_interface)
        self.assertIn("CanEnsureIndexes", module_interface)
        self.assertIn("CanAccessMomentContent", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.gate\.mongo_dao_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("mongo_dao_modules::ParseBoolText", consumer)
        self.assertIn("mongo_dao_modules::DefaultMomentsCollection", consumer)
        self.assertIn("mongo_dao_modules::CanEnsureIndexes", consumer)
        self.assertIn("mongo_dao_modules::CanAccessMomentContent", consumer)
        for non_consumer in (mongo_mgr, postgres_dao, postgres_dao_account, postgres_mgr):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.gate\.mongo_dao_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("gateshared_mongo_dao_algorithms_gtest", test_cmake)
        self.assertIn("MongoDaoAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.mongo_dao_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.mongo_dao_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_gate_infra_core_imports_chat_grpc_client_algorithms_module(self):
        cmake = read(GATE_SHARED / "core" / "CMakeLists.txt")
        module_interface = read(GATE_SHARED / "core" / "clients" / "cxx_modules" / "ChatGrpcClient.cppm")
        consumer = read(GATE_SHARED / "core" / "clients" / "ChatGrpcClient.cpp")
        header = read(GATE_SHARED / "core" / "clients" / "ChatGrpcClient.hpp")
        auth_verify_client = read(GATE_SHARED / "core" / "clients" / "AuthVerifyClient.cpp")
        verify_client = read(GATE_SHARED / "core" / "clients" / "VerifyGrpcClient.cpp")
        call_service = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "GateShared"
            / "ChatGrpcClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "GateShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateInfraCore", cmake)
        self.assertIn(
            "memochat.gate.chat_grpc_client_algorithms=clients/cxx_modules/ChatGrpcClient.cppm",
            cmake,
        )
        self.assertIn("clients/ChatGrpcClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.gate\.chat_grpc_client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::chat_grpc_client::modules", module_interface)
        self.assertIn("DefaultConnectionPoolSize", module_interface)
        self.assertIn("NotifyCallEventSpanName", module_interface)
        self.assertIn("ShouldWakeConnectionWait", module_interface)
        self.assertIn("ShouldReportMissingServer", module_interface)
        self.assertIn("ShouldReportUnavailableStub", module_interface)
        self.assertIn("ShouldReportFailedStatus", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.gate\.chat_grpc_client_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("chat_grpc_modules::DefaultConnectionPoolSize", consumer)
        self.assertIn("chat_grpc_modules::NotifyCallEventSpanName", consumer)
        self.assertIn("chat_grpc_modules::ShouldWakeConnectionWait", consumer)
        self.assertIn("chat_grpc_modules::ShouldReportMissingServer", consumer)
        self.assertIn("chat_grpc_modules::ShouldReportFailedStatus", consumer)
        self.assertIn("ChatConPool::ChatConPool", consumer)
        for non_consumer in (header, auth_verify_client, verify_client, call_service):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.gate\.chat_grpc_client_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("gateshared_chat_grpc_client_algorithms_gtest", test_cmake)
        self.assertIn("ChatGrpcClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.gate.chat_grpc_client_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.gate\.chat_grpc_client_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_memochat_runtime_imports_ini_config_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "runtime" / "cxx_modules" / "IniConfig.cppm")
        consumer = read(SERVER_CORE / "common" / "runtime" / "IniConfig.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "runtime" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_runtime", cmake)
        self.assertIn("memochat.runtime.ini_config_algorithms=common/runtime/cxx_modules/IniConfig.cppm", cmake)
        self.assertIn("common/runtime/IniConfig.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.runtime\.ini_config_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("SanitizeEnvTokenChar", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.runtime\.ini_config_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SanitizeEnvTokenChar", consumer)
        self.assertIn("runtime_ini_config_gtest", test_cmake)

    def test_memochat_runtime_imports_io_context_pool_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "runtime" / "cxx_modules" / "IoContextPool.cppm")
        consumer = read(SERVER_CORE / "common" / "runtime" / "IoContextPool.cpp")
        etcd_source = read(SERVER_CORE / "common" / "runtime" / "EtcdConfig.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "runtime"
            / "IoContextPoolAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "runtime" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_runtime", cmake)
        self.assertIn(
            "memochat.runtime.io_context_pool_algorithms=common/runtime/cxx_modules/IoContextPool.cppm",
            cmake,
        )
        self.assertIn("common/runtime/IoContextPool.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.runtime\.io_context_pool_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("NormalizeIoContextPoolSize", module_interface)
        self.assertIn("ShouldWrapNextIoContextIndex", module_interface)
        self.assertIn("ShouldStopIoContextPool", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.runtime\.io_context_pool_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::NormalizeIoContextPoolSize", consumer)
        self.assertIn("modules::ShouldWrapNextIoContextIndex", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.runtime\.io_context_pool_algorithms\s*;", etcd_source, flags=re.M)
        )
        self.assertIn("IoContextPoolAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.runtime.io_context_pool_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.runtime\.io_context_pool_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_infrastructure_core_reuses_runtime_io_context_pool_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        consumer = read(CHAT_SERVER / "infrastructure" / "AsioIOServicePool.cpp")
        config_mgr = read(CHAT_SERVER / "config" / "ConfigMgr.cpp")

        self.assertIn("memochat_enable_gnu_modules(ChatInfrastructureCore", cmake)
        self.assertIn(
            "memochat.runtime.io_context_pool_algorithms=../common/runtime/cxx_modules/IoContextPool.cppm",
            cmake,
        )
        self.assertIn("infrastructure/AsioIOServicePool.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.runtime\.io_context_pool_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("runtime_modules::NormalizeIoContextPoolSize", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.runtime\.io_context_pool_algorithms\s*;", config_mgr, flags=re.M)
        )

    def test_chat_transport_core_imports_msg_node_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "transport" / "cxx_modules" / "MsgNode.cppm")
        consumer = read(CHAT_SERVER / "transport" / "MsgNode.cpp")
        csession = read(CHAT_SERVER / "transport" / "CSession.cpp")
        quic_server = read(CHAT_SERVER / "transport" / "QuicChatServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatTransportCore", cmake)
        self.assertIn(
            "memochat.chat.msg_node_algorithms=transport/cxx_modules/MsgNode.cppm",
            cmake,
        )
        self.assertIn("transport/MsgNode.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.chat\.msg_node_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::chat::transport::msg_node::modules", module_interface)
        self.assertIn("HeaderTotalLength", module_interface)
        self.assertIn("SendBufferTotalLength", module_interface)
        self.assertIn("PayloadOffset", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.msg_node_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("msg_node_modules::SendBufferTotalLength", consumer)
        self.assertIn("msg_node_modules::PayloadOffset", consumer)
        for non_consumer in (csession, quic_server):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.msg_node_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_msg_node_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_msg_node_algorithms_gtest", test_cmake)

    def test_memochat_runtime_imports_etcd_config_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "runtime" / "cxx_modules" / "EtcdConfig.cppm")
        consumer = read(SERVER_CORE / "common" / "runtime" / "EtcdConfig.cpp")
        ini_source = read(SERVER_CORE / "common" / "runtime" / "IniConfig.cpp")
        io_pool_source = read(SERVER_CORE / "common" / "runtime" / "IoContextPool.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "runtime"
            / "EtcdConfigAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "runtime" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_runtime", cmake)
        self.assertIn(
            "memochat.runtime.etcd_config_algorithms=common/runtime/cxx_modules/EtcdConfig.cppm",
            cmake,
        )
        self.assertIn("common/runtime/EtcdConfig.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.runtime\.etcd_config_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HasMultipleEndpoints", module_interface)
        self.assertIn("IsResponseWithHeader", module_interface)
        self.assertIn("ShouldCreateEtcdConfig", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.runtime\.etcd_config_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::HasMultipleEndpoints", consumer)
        self.assertIn("modules::ShouldCreateEtcdConfig", consumer)
        for non_consumer in (ini_source, io_pool_source):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.runtime\.etcd_config_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("EtcdConfigAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.runtime.etcd_config_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.runtime\.etcd_config_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_memochat_logging_imports_config_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "logging" / "cxx_modules" / "LoggingConfig.cppm")
        log_consumer = read(SERVER_CORE / "common" / "logging" / "LogConfig.cpp")
        telemetry_consumer = read(SERVER_CORE / "common" / "logging" / "TelemetryConfig.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "logging" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_logging", cmake)
        self.assertIn("memochat.logging.config_algorithms=common/logging/cxx_modules/LoggingConfig.cppm", cmake)
        self.assertIn("common/logging/LogConfig.cpp", cmake)
        self.assertIn("common/logging/TelemetryConfig.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.logging\.config_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("TrimAsciiBegin", module_interface)
        self.assertIn("LowerAsciiInPlace", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.config_algorithms\s*;", log_consumer, flags=re.M)
        )
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.config_algorithms\s*;", telemetry_consumer, flags=re.M)
        )
        self.assertIn("logging_config_gtest", test_cmake)

    def test_memochat_logging_imports_redaction_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "logging" / "cxx_modules" / "Redaction.cppm")
        consumer = read(SERVER_CORE / "common" / "logging" / "Redaction.cpp")
        logger_source = read(SERVER_CORE / "common" / "logging" / "Logger.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "logging"
            / "RedactionAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "logging" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_logging", cmake)
        self.assertIn("memochat.logging.redaction_algorithms=common/logging/cxx_modules/Redaction.cppm", cmake)
        self.assertIn("common/logging/Redaction.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.logging\.redaction_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ClassifyLowerSensitiveKey", module_interface)
        self.assertIn("ShouldRedactAsToken", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.redaction_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("redaction_modules::ClassifyLowerSensitiveKey", consumer)
        self.assertNotRegex(logger_source, r"^\s*import\s+memochat\.logging\.redaction_algorithms\s*;")
        self.assertIn("memochat_enable_gnu_modules(logging_gtest_json_logger_test", test_cmake)
        self.assertIn("RedactionAlgorithmsConsumer.cpp", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.redaction_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_memochat_logging_imports_logger_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "logging" / "cxx_modules" / "Logger.cppm")
        consumer = read(SERVER_CORE / "common" / "logging" / "Logger.cpp")
        telemetry_source = read(SERVER_CORE / "common" / "logging" / "Telemetry.cpp")
        grpc_trace_source = read(SERVER_CORE / "common" / "logging" / "GrpcTrace.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "logging"
            / "LoggerAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "logging" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_logging", cmake)
        self.assertIn("memochat.logging.logger_algorithms=common/logging/cxx_modules/Logger.cppm", cmake)
        self.assertIn("common/logging/Logger.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.logging\.logger_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldUseDefaultLogDir", module_interface)
        self.assertIn("IsTopLevelFieldName", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.logging\.logger_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("logger_modules::ShouldUseDefaultLogDir", consumer)
        self.assertIn("logger_modules::IsTopLevelFieldName", consumer)
        for non_consumer in (telemetry_source, grpc_trace_source):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.logging\.logger_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("LoggerAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.logging.logger_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.logger_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_memochat_logging_imports_telemetry_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "logging" / "cxx_modules" / "Telemetry.cppm")
        consumer = read(SERVER_CORE / "common" / "logging" / "Telemetry.cpp")
        logger_source = read(SERVER_CORE / "common" / "logging" / "Logger.cpp")
        grpc_trace_source = read(SERVER_CORE / "common" / "logging" / "GrpcTrace.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "logging"
            / "TelemetryAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "logging" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_logging", cmake)
        self.assertIn("memochat.logging.telemetry_algorithms=common/logging/cxx_modules/Telemetry.cppm", cmake)
        self.assertIn("common/logging/Telemetry.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.logging\.telemetry_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HasHttpSchemePrefix", module_interface)
        self.assertIn("IsTelemetryEnabled", module_interface)
        self.assertIn("ShouldKeepSpanAttribute", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.telemetry_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("telemetry_modules::HasHttpSchemePrefix", consumer)
        self.assertIn("telemetry_modules::IsTelemetryEnabled", consumer)
        self.assertIn("telemetry_modules::ShouldKeepSpanAttribute", consumer)
        for non_consumer in (logger_source, grpc_trace_source):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.logging\.telemetry_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("TelemetryAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.logging.telemetry_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.telemetry_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_memochat_logging_imports_grpc_trace_algorithms_module(self):
        cmake = read(SERVER_CORE / "CMakeLists.txt")
        module_interface = read(SERVER_CORE / "common" / "logging" / "cxx_modules" / "GrpcTrace.cppm")
        consumer = read(SERVER_CORE / "common" / "logging" / "GrpcTrace.cpp")
        logger_source = read(SERVER_CORE / "common" / "logging" / "Logger.cpp")
        telemetry_source = read(SERVER_CORE / "common" / "logging" / "Telemetry.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "common"
            / "logging"
            / "GrpcTraceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "logging" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(memochat_logging", cmake)
        self.assertIn("memochat.logging.grpc_trace_algorithms=common/logging/cxx_modules/GrpcTrace.cppm", cmake)
        self.assertIn("common/logging/GrpcTrace.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.logging\.grpc_trace_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldInjectTraceId", module_interface)
        self.assertIn("ShouldGenerateRequestId", module_interface)
        self.assertIn("ShouldGenerateBoundTraceId", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.grpc_trace_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("grpc_trace_modules::ShouldInjectTraceId", consumer)
        self.assertIn("grpc_trace_modules::ShouldGenerateBoundRequestId", consumer)
        for non_consumer in (logger_source, telemetry_source):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.logging\.grpc_trace_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("GrpcTraceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.logging.grpc_trace_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.logging\.grpc_trace_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_chat_config_core_imports_chat_config_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "config" / "cxx_modules" / "ChatConfig.cppm")
        session_consumer = read(CHAT_SERVER / "config" / "ChatSessionConfig.cpp")
        message_consumer = read(CHAT_SERVER / "config" / "MessageServiceConfig.cpp")
        relation_consumer = read(CHAT_SERVER / "config" / "RelationServiceConfig.cpp")
        relation_query_consumer = read(CHAT_SERVER / "config" / "RelationQueryServiceConfig.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatConfigCore", cmake)
        self.assertIn("memochat.chat.config_algorithms=config/cxx_modules/ChatConfig.cppm", cmake)
        self.assertIn("config/ChatSessionConfig.cpp", cmake)
        self.assertIn("config/MessageServiceConfig.cpp", cmake)
        self.assertIn("config/RelationServiceConfig.cpp", cmake)
        self.assertIn("config/RelationQueryServiceConfig.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.chat\.config_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("LowerAsciiInPlace", module_interface)
        self.assertIn("FeatureFlagDefaultTrue", module_interface)
        self.assertIn("ClampInt", module_interface)
        for consumer in (session_consumer, message_consumer, relation_consumer, relation_query_consumer):
            self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.config_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("chatserver_config_gtest", test_cmake)

    def test_chat_app_core_imports_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "app" / "cxx_modules" / "ChatRuntime.cppm")
        consumer = read(CHAT_SERVER / "app" / "ChatRuntime.cpp")
        entrypoint = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        delivery_worker = read(CHAT_SERVER / "app" / "ChatDeliveryWorker.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatAppCore", cmake)
        self.assertIn("memochat.chat.runtime_algorithms=app/cxx_modules/ChatRuntime.cppm", cmake)
        self.assertIn("app/ChatRuntime.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.chat\.runtime_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("ToLowerAscii", module_interface)
        self.assertIn("ParseBoolFlagOr", module_interface)
        self.assertIn("AtLeast", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.runtime_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("runtime::modules::ToLowerAscii", consumer)
        self.assertIn("runtime::modules::ParseBoolFlagOr", consumer)
        self.assertIn("runtime::modules::AtLeast", consumer)
        self.assertIsNone(re.search(r"^\s*import\s+memochat\.chat\.runtime_algorithms\s*;", entrypoint, flags=re.M))
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.runtime_algorithms\s*;", delivery_worker, flags=re.M)
        )
        self.assertIn("ChatRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_envelope_codec_gtest", test_cmake)

    def test_chat_orchestration_core_imports_runtime_composition_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(
            CHAT_SERVER / "domain" / "orchestration" / "cxx_modules" / "ChatRuntimeComposition.cppm"
        )
        consumer = read(CHAT_SERVER / "domain" / "orchestration" / "ChatRuntimeComposition.cpp")
        logic_system = read(CHAT_SERVER / "domain" / "orchestration" / "LogicSystem.cpp")
        registrars = read(CHAT_SERVER / "domain" / "orchestration" / "ChatHandlerRegistrars.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRuntimeCompositionAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatOrchestrationCore", cmake)
        self.assertIn(
            "memochat.chat.runtime_composition_algorithms=domain/orchestration/cxx_modules/ChatRuntimeComposition.cppm",
            cmake,
        )
        self.assertIn("domain/orchestration/ChatRuntimeComposition.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.runtime_composition_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldUseRabbitMqTaskBus", module_interface)
        self.assertIn("ShouldWarnUnsupportedTaskBusBackend", module_interface)
        self.assertIn("ShouldUseKafkaAsyncEventBus", module_interface)
        self.assertIn("ShouldWarnUnsupportedAsyncEventBusBackend", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.runtime_composition_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("runtime_composition_modules::ShouldUseRabbitMqTaskBus", consumer)
        self.assertIn("runtime_composition_modules::ShouldWarnUnsupportedTaskBusBackend", consumer)
        self.assertIn("runtime_composition_modules::ShouldUseKafkaAsyncEventBus", consumer)
        self.assertIn("runtime_composition_modules::ShouldWarnUnsupportedAsyncEventBusBackend", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.runtime_composition_algorithms\s*;", logic_system, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.runtime_composition_algorithms\s*;", registrars, flags=re.M)
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.runtime_composition_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )
        self.assertIn("chatserver_runtime_composition_algorithms_gtest", test_cmake)
        self.assertIn("ChatRuntimeCompositionAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.runtime_composition_algorithms", test_cmake)

    def test_chat_orchestration_core_imports_handler_registrar_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "orchestration" / "cxx_modules" / "ChatHandlerRegistrar.cppm")
        consumer = read(CHAT_SERVER / "domain" / "orchestration" / "ChatHandlerRegistrars.cpp")
        runtime_composition = read(CHAT_SERVER / "domain" / "orchestration" / "ChatRuntimeComposition.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatHandlerRegistrarAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn(
            "memochat.chat.handler_registrar_algorithms=domain/orchestration/cxx_modules/ChatHandlerRegistrar.cppm",
            cmake,
        )
        self.assertIn("domain/orchestration/ChatHandlerRegistrars.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.handler_registrar_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ChatSessionHandlerCount", module_interface)
        self.assertIn("ExpectedTotalHandlerCount", module_interface)
        self.assertIn("ShouldRegisterAsyncEventDispatcherHandlers", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.handler_registrar_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("handler_registrar_modules::ChatSessionHandlerCount", consumer)
        self.assertIn("handler_registrar_modules::GroupMessageHandlerCount", consumer)
        self.assertIn("handler_registrar_modules::ShouldRegisterAsyncEventDispatcherHandlers", consumer)
        self.assertIsNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.handler_registrar_algorithms\s*;",
                runtime_composition,
                flags=re.M,
            )
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.handler_registrar_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )
        self.assertIn("chatserver_handler_registrar_algorithms_gtest", test_cmake)
        self.assertIn("ChatHandlerRegistrarAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.handler_registrar_algorithms", test_cmake)

    def test_chat_message_service_imports_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "app" / "cxx_modules" / "ChatMessageServiceRuntime.cppm")
        consumer = read(CHAT_SERVER / "app" / "ChatMessageService.cpp")
        chat_server = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        delivery_worker = read(CHAT_SERVER / "app" / "ChatDeliveryWorker.cpp")
        relation_service_worker = read(CHAT_SERVER / "app" / "ChatRelationServiceWorker.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatMessageServiceRuntimeAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessageService", cmake)
        self.assertIn(
            "memochat.chat.message_service_runtime_algorithms=app/cxx_modules/ChatMessageServiceRuntime.cppm",
            cmake,
        )
        self.assertIn("app/ChatMessageService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.message_service_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsConfigFlag", module_interface)
        self.assertIn("DefaultServiceName", module_interface)
        self.assertIn("DefaultSnowflakeWorkerId", module_interface)
        self.assertIn("DisabledEventPublishError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.message_service_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("message_service_modules::IsConfigFlag", consumer)
        self.assertIn("message_service_modules::DefaultServiceName", consumer)
        self.assertIn("message_service_modules::DefaultSnowflakeWorkerId", consumer)
        self.assertIn("message_service_modules::DisabledEventPublishError", consumer)
        for non_consumer in (chat_server, delivery_worker, relation_service_worker):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.message_service_runtime_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_message_service_runtime_algorithms_gtest", test_cmake)
        self.assertIn("ChatMessageServiceRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.message_service_runtime_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.message_service_runtime_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_query_service_imports_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "app" / "cxx_modules" / "ChatRelationQueryServiceRuntime.cppm")
        consumer = read(CHAT_SERVER / "app" / "ChatRelationQueryService.cpp")
        chat_server = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        message_service = read(CHAT_SERVER / "app" / "ChatMessageService.cpp")
        relation_service_worker = read(CHAT_SERVER / "app" / "ChatRelationServiceWorker.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRelationQueryServiceRuntimeAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatRelationQueryService", cmake)
        self.assertIn(
            "memochat.chat.relation_query_service_runtime_algorithms=app/cxx_modules/ChatRelationQueryServiceRuntime.cppm",
            cmake,
        )
        self.assertIn("app/ChatRelationQueryService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_query_service_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsConfigFlag", module_interface)
        self.assertIn("DefaultServiceName", module_interface)
        self.assertIn("DefaultSnowflakeWorkerId", module_interface)
        self.assertIn("LoggerName", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_query_service_runtime_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_query_service_modules::IsConfigFlag", consumer)
        self.assertIn("relation_query_service_modules::DefaultServiceName", consumer)
        self.assertIn("relation_query_service_modules::DefaultSnowflakeWorkerId", consumer)
        self.assertIn("relation_query_service_modules::LoggerName", consumer)
        for non_consumer in (chat_server, message_service, relation_service_worker):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_query_service_runtime_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_query_service_runtime_algorithms_gtest", test_cmake)
        self.assertIn("ChatRelationQueryServiceRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_query_service_runtime_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_query_service_runtime_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_delivery_worker_imports_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "app" / "cxx_modules" / "ChatDeliveryWorkerRuntime.cppm")
        consumer = read(CHAT_SERVER / "app" / "ChatDeliveryWorker.cpp")
        chat_server = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        message_service = read(CHAT_SERVER / "app" / "ChatMessageService.cpp")
        relation_query_service = read(CHAT_SERVER / "app" / "ChatRelationQueryService.cpp")
        relation_service_worker = read(CHAT_SERVER / "app" / "ChatRelationServiceWorker.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatDeliveryWorkerRuntimeAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatDeliveryWorker", cmake)
        self.assertIn(
            "memochat.chat.delivery_worker_runtime_algorithms=app/cxx_modules/ChatDeliveryWorkerRuntime.cppm",
            cmake,
        )
        self.assertIn("app/ChatDeliveryWorker.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.delivery_worker_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsConfigFlag", module_interface)
        self.assertIn("DefaultSnowflakeWorkerId", module_interface)
        self.assertIn("EmptyWorkerNameMessage", module_interface)
        self.assertIn("WorkerEnabledText", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.delivery_worker_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("delivery_worker_modules::IsConfigFlag", consumer)
        self.assertIn("delivery_worker_modules::DefaultSnowflakeWorkerId", consumer)
        self.assertIn("delivery_worker_modules::EmptyWorkerNameMessage", consumer)
        self.assertIn("delivery_worker_modules::WorkerEnabledText", consumer)
        for non_consumer in (chat_server, message_service, relation_query_service, relation_service_worker):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.delivery_worker_runtime_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_delivery_worker_runtime_algorithms_gtest", test_cmake)
        self.assertIn("ChatDeliveryWorkerRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.delivery_worker_runtime_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.delivery_worker_runtime_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_service_worker_imports_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "app" / "cxx_modules" / "ChatRelationServiceWorkerRuntime.cppm")
        consumer = read(CHAT_SERVER / "app" / "ChatRelationServiceWorker.cpp")
        chat_server = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        message_service = read(CHAT_SERVER / "app" / "ChatMessageService.cpp")
        relation_query_service = read(CHAT_SERVER / "app" / "ChatRelationQueryService.cpp")
        delivery_worker = read(CHAT_SERVER / "app" / "ChatDeliveryWorker.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRelationServiceWorkerRuntimeAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatRelationServiceWorker", cmake)
        self.assertIn(
            "memochat.chat.relation_service_worker_runtime_algorithms=app/cxx_modules/ChatRelationServiceWorkerRuntime.cppm",
            cmake,
        )
        self.assertIn("app/ChatRelationServiceWorker.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_service_worker_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsConfigFlag", module_interface)
        self.assertIn("DefaultSnowflakeWorkerId", module_interface)
        self.assertIn("IsRabbitMqBackend", module_interface)
        self.assertIn("IsKafkaBackend", module_interface)
        self.assertIn("EventBusUnavailableError", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_service_worker_runtime_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_service_worker_modules::IsConfigFlag", consumer)
        self.assertIn("relation_service_worker_modules::DefaultServiceName", consumer)
        self.assertIn("relation_service_worker_modules::DefaultSnowflakeWorkerId", consumer)
        self.assertIn("relation_service_worker_modules::IsRabbitMqBackend", consumer)
        self.assertIn("relation_service_worker_modules::IsKafkaBackend", consumer)
        self.assertIn("relation_service_worker_modules::EventBusUnavailableError", consumer)
        for non_consumer in (chat_server, message_service, relation_query_service, delivery_worker):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_service_worker_runtime_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_service_worker_runtime_algorithms_gtest", test_cmake)
        self.assertIn("ChatRelationServiceWorkerRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_service_worker_runtime_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_service_worker_runtime_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_message_core_imports_message_delivery_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "message" / "cxx_modules" / "MessageDelivery.cppm")
        consumer = read(CHAT_SERVER / "domain" / "message" / "MessageDeliveryPayload.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessageCore", cmake)
        self.assertIn(
            "memochat.chat.message_delivery_algorithms=domain/message/cxx_modules/MessageDelivery.cppm",
            cmake,
        )
        self.assertIn("domain/message/MessageDeliveryPayload.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.message_delivery_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("SelectPrivateNotifyRecipient", module_interface)
        self.assertIn("HasValidPrivateNotifyEnvelope", module_interface)
        self.assertIn("IsValidPrivateTextItem", module_interface)
        self.assertIn("export namespace memochat::chat::delivery::modules", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.message_delivery_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SelectPrivateNotifyRecipient", consumer)
        self.assertIn("modules::HasValidPrivateNotifyEnvelope", consumer)
        self.assertIn("modules::IsValidPrivateTextItem", consumer)
        self.assertNotIn("memochat::chat::message::modules", consumer)
        self.assertIn("chatserver_message_delivery_payload_gtest", test_cmake)
        self.assertIn("MessageDeliveryAlgorithmsTest.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_message_delivery_payload_gtest", test_cmake)

    def test_chat_message_core_imports_group_response_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "message" / "cxx_modules" / "GroupResponse.cppm")
        consumer = read(CHAT_SERVER / "domain" / "message" / "GroupResponseFormatter.cpp")
        service_source = read(CHAT_SERVER / "domain" / "message" / "GroupMessageService.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessageCore", cmake)
        self.assertIn(
            "memochat.chat.group_response_algorithms=domain/message/cxx_modules/GroupResponse.cppm",
            cmake,
        )
        self.assertIn("domain/message/GroupResponseFormatter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.group_response_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::message::group_response::modules", module_interface)
        self.assertIn("HasPermissionBit", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.group_response_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("group_response::modules::HasPermissionBit", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.group_response_algorithms\s*;", service_source, flags=re.M)
        )
        self.assertIn("chatserver_message_command_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_message_command_dtos_gtest", test_cmake)

    def test_chat_message_internal_grpc_service_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(
            CHAT_SERVER / "domain" / "message" / "cxx_modules" / "ChatMessageInternalGrpcService.cppm"
        )
        consumer = read(CHAT_SERVER / "domain" / "message" / "ChatMessageInternalGrpcService.cpp")
        header = read(CHAT_SERVER / "domain" / "message" / "ChatMessageInternalGrpcService.hpp")
        message_client = read(CHAT_SERVER / "clients" / "MessageGrpcClient.cpp")
        service_factory = read(CHAT_SERVER / "domain" / "message" / "MessageServiceFactory.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatMessageInternalGrpcServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessageCore", cmake)
        self.assertIn(
            "memochat.chat.message_internal_grpc_service_algorithms=domain/message/cxx_modules/ChatMessageInternalGrpcService.cppm",
            cmake,
        )
        self.assertIn("domain/message/ChatMessageInternalGrpcService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.message_internal_grpc_service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultPayloadJson", module_interface)
        self.assertIn("MissingPrivateMessageCommandRequestMessage", module_interface)
        self.assertIn("PrivateMessageCommandServiceNotConfiguredMessage", module_interface)
        self.assertIn("MissingGroupMessageCommandRequestMessage", module_interface)
        self.assertIn("GroupMessageCommandServiceNotConfiguredMessage", module_interface)
        self.assertIn("ShouldReportMissingRequestOrResponse", module_interface)
        self.assertIn("TcpMessageId", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.message_internal_grpc_service_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("message_internal_grpc_modules::TcpMessageId", consumer)
        self.assertIn("message_internal_grpc_modules::DefaultPayloadJson", consumer)
        self.assertIn("message_internal_grpc_modules::ShouldReportMissingRequestOrResponse", consumer)
        for non_consumer in (header, message_client, service_factory):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.message_internal_grpc_service_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_message_internal_grpc_service_algorithms_gtest", test_cmake)
        self.assertIn("ChatMessageInternalGrpcServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.message_internal_grpc_service_algorithms", test_cmake)
        self.assertIn("ChatMessageCore", test_cmake)
        self.assertNotIn(
            "${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/message/ChatMessageInternalGrpcService.cpp",
            test_cmake,
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.message_internal_grpc_service_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_internal_grpc_service_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(
            CHAT_SERVER / "domain" / "relation" / "cxx_modules" / "ChatRelationInternalGrpcService.cppm"
        )
        consumer = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationInternalGrpcService.cpp")
        header = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationInternalGrpcService.hpp")
        relation_client = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        relation_query_client = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.cpp")
        service_factory = read(CHAT_SERVER / "domain" / "relation" / "RelationServiceFactory.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRelationInternalGrpcServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatRelationCore", cmake)
        self.assertIn(
            "memochat.chat.relation_internal_grpc_service_algorithms=domain/relation/cxx_modules/ChatRelationInternalGrpcService.cppm",
            cmake,
        )
        self.assertIn("domain/relation/ChatRelationInternalGrpcService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_internal_grpc_service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultPayloadJson", module_interface)
        self.assertIn("MissingBootstrapRequestMessage", module_interface)
        self.assertIn("RelationServiceNotConfiguredMessage", module_interface)
        self.assertIn("UidMustBePositiveMessage", module_interface)
        self.assertIn("MissingRelationCommandRequestMessage", module_interface)
        self.assertIn("ShouldReportMissingRequestOrResponse", module_interface)
        self.assertIn("ShouldReportInvalidUid", module_interface)
        self.assertIn("TcpMessageId", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_internal_grpc_service_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_internal_grpc_modules::TcpMessageId", consumer)
        self.assertIn("relation_internal_grpc_modules::DefaultPayloadJson", consumer)
        self.assertIn("relation_internal_grpc_modules::ShouldReportMissingRequestOrResponse", consumer)
        for non_consumer in (header, relation_client, relation_query_client, service_factory):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_internal_grpc_service_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_internal_grpc_service_algorithms_gtest", test_cmake)
        self.assertIn("ChatRelationInternalGrpcServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_internal_grpc_service_algorithms", test_cmake)
        self.assertIn("ChatServerCore", test_cmake)
        self.assertNotIn(
            "${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/domain/relation/ChatRelationInternalGrpcService.cpp",
            test_cmake,
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_internal_grpc_service_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_service_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "relation" / "cxx_modules" / "ChatRelationService.cppm")
        consumer = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationService.cpp")
        internal_service = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationInternalGrpcService.cpp")
        service_factory = read(CHAT_SERVER / "domain" / "relation" / "RelationServiceFactory.cpp")
        session_adapter = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationSessionAdapter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRelationServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatRelationCore", cmake)
        self.assertIn(
            "memochat.chat.relation_service_algorithms=domain/relation/cxx_modules/ChatRelationService.cppm",
            cmake,
        )
        self.assertIn("domain/relation/ChatRelationService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::relation_service::modules", module_interface)
        self.assertIn("PrivateDialogType", module_interface)
        self.assertIn("RelationNotifyTaskName", module_interface)
        self.assertIn("ShouldRejectSearchUserRequest", module_interface)
        self.assertIn("ShouldRejectDeleteFriend", module_interface)
        self.assertIn("ShouldRejectDialogType", module_interface)
        self.assertIn("NormalizeMuteState", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.relation_service_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("relation_service_modules::ShouldRejectSearchUserRequest", consumer)
        self.assertIn("relation_service_modules::RelationNotifyTaskName", consumer)
        self.assertIn("relation_service_modules::NormalizeMuteState", consumer)
        for non_consumer in (internal_service, service_factory, session_adapter):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.relation_service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_relation_service_algorithms_gtest", test_cmake)
        self.assertIn("ChatRelationServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_service_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.relation_service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_chat_delivery_payload_core_imports_task_payload_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "delivery" / "cxx_modules" / "DeliveryTaskPayload.cppm")
        consumer = read(CHAT_SERVER / "domain" / "delivery" / "MessageDeliveryTaskPayload.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatDeliveryPayloadCore", cmake)
        self.assertIn(
            "memochat.chat.delivery_task_payload_algorithms=domain/delivery/cxx_modules/DeliveryTaskPayload.cppm",
            cmake,
        )
        self.assertIn("domain/delivery/MessageDeliveryTaskPayload.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.delivery_task_payload_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::delivery::task_payload::modules", module_interface)
        self.assertIn("NormalizeTaskMessageId", module_interface)
        self.assertIn("HasRequiredTaskPayloadShape", module_interface)
        self.assertIn("IsValidParsedDeliveryTask", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.delivery_task_payload_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("task_payload::modules::NormalizeTaskMessageId", consumer)
        self.assertIn("task_payload::modules::HasRequiredTaskPayloadShape", consumer)
        self.assertIn("task_payload::modules::IsValidParsedDeliveryTask", consumer)
        self.assertIn("chatserver_message_delivery_task_payload_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_message_delivery_task_payload_gtest", test_cmake)

    def test_chat_delivery_core_imports_delivery_runtime_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "delivery" / "cxx_modules" / "DeliveryRuntime.cppm")
        consumer = read(CHAT_SERVER / "domain" / "delivery" / "ChatDeliveryRuntime.cpp")
        dispatcher = read(CHAT_SERVER / "domain" / "delivery" / "TaskDispatcher.cpp")
        payload_codec = read(CHAT_SERVER / "domain" / "delivery" / "MessageDeliveryTaskPayload.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatDeliveryCore", cmake)
        self.assertIn(
            "memochat.chat.delivery_runtime_algorithms=domain/delivery/cxx_modules/DeliveryRuntime.cppm",
            cmake,
        )
        self.assertIn("domain/delivery/ChatDeliveryRuntime.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.delivery_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::delivery::runtime::modules", module_interface)
        self.assertIn("InitialStartedExpected", module_interface)
        self.assertIn("StopRequestedWhenStarting", module_interface)
        self.assertIn("StartedAfterStopAndJoin", module_interface)
        self.assertIn("ShouldRunLoop", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.delivery_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("delivery_runtime_modules::InitialStartedExpected", consumer)
        self.assertIn("delivery_runtime_modules::StopRequestedWhenStarting", consumer)
        self.assertIn("delivery_runtime_modules::ShouldJoinThread", consumer)
        self.assertIn("delivery_runtime_modules::ShouldRunLoop", consumer)
        for non_consumer in (dispatcher, payload_codec):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.delivery_runtime_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_delivery_runtime_algorithms_gtest", test_cmake)
        self.assertIn("DeliveryRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.delivery_runtime_algorithms", test_cmake)

    def test_chat_delivery_core_imports_task_dispatcher_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "delivery" / "cxx_modules" / "TaskDispatcher.cppm")
        consumer = read(CHAT_SERVER / "domain" / "delivery" / "TaskDispatcher.cpp")
        runtime = read(CHAT_SERVER / "domain" / "delivery" / "ChatDeliveryRuntime.cpp")
        payload_codec = read(CHAT_SERVER / "domain" / "delivery" / "MessageDeliveryTaskPayload.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatDeliveryCore", cmake)
        self.assertIn(
            "memochat.chat.task_dispatcher_algorithms=domain/delivery/cxx_modules/TaskDispatcher.cppm",
            cmake,
        )
        self.assertIn("domain/delivery/TaskDispatcher.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.task_dispatcher_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::delivery::task_dispatcher::modules", module_interface)
        self.assertIn("IsDeliveryTaskType", module_interface)
        self.assertIn("IsOutboxRepairTaskType", module_interface)
        self.assertIn("ShouldExpediteOutboxRepair", module_interface)
        self.assertIn("TaskBusUnavailableError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.task_dispatcher_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("task_dispatcher_modules::ShouldPublishTask", consumer)
        self.assertIn("task_dispatcher_modules::IsDeliveryTaskType", consumer)
        self.assertIn("task_dispatcher_modules::ShouldExpediteOutboxRepair", consumer)
        self.assertIn("task_dispatcher_modules::UnknownTaskTypeEventName", consumer)
        for non_consumer in (runtime, payload_codec):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.task_dispatcher_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_task_dispatcher_algorithms_gtest", test_cmake)
        self.assertIn("TaskDispatcherAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.task_dispatcher_algorithms", test_cmake)

    def test_chat_messaging_core_imports_config_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "MessagingConfig.cppm")
        kafka_consumer = read(CHAT_SERVER / "messaging" / "KafkaConfig.cpp")
        rabbit_consumer = read(CHAT_SERVER / "messaging" / "RabbitMqConfig.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.messaging_config_algorithms=messaging/cxx_modules/MessagingConfig.cppm",
            cmake,
        )
        self.assertIn("messaging/KafkaConfig.cpp", cmake)
        self.assertIn("messaging/RabbitMqConfig.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.messaging_config_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ParseIntOr", module_interface)
        self.assertIn("AtLeast", module_interface)
        self.assertIn("ParseBoolFlagOr", module_interface)
        for consumer in (kafka_consumer, rabbit_consumer):
            self.assertIsNotNone(
                re.search(r"^\s*import\s+memochat\.chat\.messaging_config_algorithms\s*;", consumer, flags=re.M)
            )
        self.assertIn("modules::ParseIntOr", kafka_consumer)
        self.assertIn("modules::ParseBoolFlagOr", kafka_consumer)
        self.assertIn("modules::ParseIntOr", rabbit_consumer)
        self.assertIn("modules::AtLeast", rabbit_consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.messaging_config_algorithms\s*;", kafka_bus, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.messaging_config_algorithms\s*;", rabbit_bus, flags=re.M)
        )
        self.assertIn("chatserver_messaging_config_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_messaging_config_algorithms_gtest", test_cmake)

    def test_chat_messaging_core_imports_envelope_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "MessagingEnvelope.cppm")
        task_consumer = read(CHAT_SERVER / "messaging" / "ChatTaskEnvelope.cpp")
        event_consumer = read(CHAT_SERVER / "messaging" / "ChatAsyncEvent.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.messaging_envelope_algorithms=messaging/cxx_modules/MessagingEnvelope.cppm",
            cmake,
        )
        self.assertIn("messaging/ChatAsyncEvent.cpp", cmake)
        self.assertIn("messaging/ChatTaskEnvelope.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.messaging_envelope_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("NonNegative", module_interface)
        self.assertIn("MinInt", module_interface)
        self.assertIn("MaxInt", module_interface)
        for consumer in (task_consumer, event_consumer):
            self.assertIsNotNone(
                re.search(r"^\s*import\s+memochat\.chat\.messaging_envelope_algorithms\s*;", consumer, flags=re.M)
            )
        self.assertIn("envelope_modules::NonNegative", task_consumer)
        self.assertIn("envelope_modules::MinInt", event_consumer)
        self.assertIn("envelope_modules::MaxInt", event_consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.messaging_envelope_algorithms\s*;", kafka_bus, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.messaging_envelope_algorithms\s*;", rabbit_bus, flags=re.M)
        )
        self.assertIn("memochat.chat.messaging_envelope_algorithms", test_cmake)
        self.assertIn("chatserver_envelope_codec_gtest", test_cmake)

    def test_chat_messaging_core_imports_inline_task_bus_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "InlineTaskBus.cppm")
        consumer = read(CHAT_SERVER / "messaging" / "InlineTaskBus.cpp")
        dispatcher = read(CHAT_SERVER / "messaging" / "AsyncEventDispatcher.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.inline_task_bus_algorithms=messaging/cxx_modules/InlineTaskBus.cppm",
            cmake,
        )
        self.assertIn("messaging/InlineTaskBus.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.inline_task_bus_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::messaging::inline_task_bus::modules", module_interface)
        self.assertIn("AcceptsAllRoutingKeys", module_interface)
        self.assertIn("ShouldConsumeAvailableTask", module_interface)
        self.assertIn("ShouldDropAfterRetry", module_interface)
        self.assertIn("RetryDelayMs", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.inline_task_bus_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("inline_task_modules::AcceptsAllRoutingKeys", consumer)
        self.assertIn("inline_task_modules::ShouldConsumeAvailableTask", consumer)
        self.assertIn("inline_task_modules::ShouldDropAfterRetry", consumer)
        self.assertIn("inline_task_modules::RetryDelayMs", consumer)
        for non_consumer in (dispatcher, kafka_bus, rabbit_bus):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.inline_task_bus_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_inline_task_bus_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_inline_task_bus_algorithms_gtest", test_cmake)

    def test_chat_messaging_core_imports_async_event_dispatcher_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "AsyncEventDispatcher.cppm")
        consumer = read(CHAT_SERVER / "messaging" / "AsyncEventDispatcher.cpp")
        inline_bus = read(CHAT_SERVER / "messaging" / "InlineTaskBus.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        redis_bus = read(CHAT_SERVER / "messaging" / "RedisAsyncEventBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.async_event_dispatcher_algorithms=messaging/cxx_modules/AsyncEventDispatcher.cppm",
            cmake,
        )
        self.assertIn("messaging/AsyncEventDispatcher.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.async_event_dispatcher_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::messaging::async_event_dispatcher::modules", module_interface)
        self.assertIn("ShouldRejectPublish", module_interface)
        self.assertIn("ShouldAckInvalidEvent", module_interface)
        self.assertIn("DialogSyncPublishFailedEvent", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.async_event_dispatcher_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("async_dispatch_modules::ShouldRejectPublish", consumer)
        self.assertIn("async_dispatch_modules::ShouldAckInvalidEvent", consumer)
        self.assertIn("async_dispatch_modules::DialogSyncPublishFailedEvent", consumer)
        for non_consumer in (inline_bus, kafka_bus, rabbit_bus, redis_bus):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.async_event_dispatcher_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_async_event_dispatcher_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_async_event_dispatcher_algorithms_gtest", test_cmake)

    def test_chat_messaging_core_imports_redis_async_event_bus_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "RedisAsyncEventBus.cppm")
        consumer = read(CHAT_SERVER / "messaging" / "RedisAsyncEventBus.cpp")
        dispatcher = read(CHAT_SERVER / "messaging" / "AsyncEventDispatcher.cpp")
        inline_bus = read(CHAT_SERVER / "messaging" / "InlineTaskBus.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.redis_async_event_bus_algorithms=messaging/cxx_modules/RedisAsyncEventBus.cppm",
            cmake,
        )
        self.assertIn("messaging/RedisAsyncEventBus.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.redis_async_event_bus_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::messaging::redis_event_bus::modules", module_interface)
        self.assertIn("ShouldRejectSerializedPayload", module_interface)
        self.assertIn("ShouldRequeueLastConsumed", module_interface)
        self.assertIn("SerializeFailedError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.redis_async_event_bus_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("redis_event_modules::ShouldRejectSerializedPayload", consumer)
        self.assertIn("redis_event_modules::ShouldRequeueLastConsumed", consumer)
        self.assertIn("redis_event_modules::QueuePublishFailedError", consumer)
        for non_consumer in (dispatcher, inline_bus, kafka_bus, rabbit_bus):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.redis_async_event_bus_algorithms\s*;", non_consumer, flags=re.M
                )
            )
        self.assertIn("chatserver_redis_async_event_bus_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_redis_async_event_bus_algorithms_gtest", test_cmake)

    def test_chat_messaging_core_imports_kafka_async_event_bus_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "KafkaAsyncEventBus.cppm")
        consumer = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        dispatcher = read(CHAT_SERVER / "messaging" / "AsyncEventDispatcher.cpp")
        inline_bus = read(CHAT_SERVER / "messaging" / "InlineTaskBus.cpp")
        redis_bus = read(CHAT_SERVER / "messaging" / "RedisAsyncEventBus.cpp")
        rabbit_bus = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.kafka_async_event_bus_algorithms=messaging/cxx_modules/KafkaAsyncEventBus.cppm",
            cmake,
        )
        self.assertIn("messaging/KafkaAsyncEventBus.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.kafka_async_event_bus_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::messaging::kafka_event_bus::modules", module_interface)
        self.assertIn("ShouldRejectPublish", module_interface)
        self.assertIn("ShouldRouteToDlq", module_interface)
        self.assertIn("KafkaBuildDisabledError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.kafka_async_event_bus_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("kafka_event_modules::ShouldRejectPublish", consumer)
        self.assertIn("kafka_event_modules::ShouldRouteToDlq", consumer)
        self.assertIn("kafka_event_modules::ProducerUnavailableError", consumer)
        for non_consumer in (dispatcher, inline_bus, redis_bus, rabbit_bus):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.kafka_async_event_bus_algorithms\s*;", non_consumer, flags=re.M
                )
            )
        self.assertIn("chatserver_kafka_async_event_bus_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_kafka_async_event_bus_algorithms_gtest", test_cmake)

    def test_chat_messaging_core_imports_rabbitmq_task_bus_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "messaging" / "cxx_modules" / "RabbitMqTaskBus.cppm")
        consumer = read(CHAT_SERVER / "messaging" / "RabbitMqTaskBus.cpp")
        dispatcher = read(CHAT_SERVER / "messaging" / "AsyncEventDispatcher.cpp")
        inline_bus = read(CHAT_SERVER / "messaging" / "InlineTaskBus.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        redis_bus = read(CHAT_SERVER / "messaging" / "RedisAsyncEventBus.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatMessagingCore", cmake)
        self.assertIn(
            "memochat.chat.rabbitmq_task_bus_algorithms=messaging/cxx_modules/RabbitMqTaskBus.cppm",
            cmake,
        )
        self.assertIn("messaging/RabbitMqTaskBus.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.rabbitmq_task_bus_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::messaging::rabbitmq_task_bus::modules", module_interface)
        self.assertIn("ShouldRejectRpcReply", module_interface)
        self.assertIn("ShouldRouteToDeadLetter", module_interface)
        self.assertIn("RabbitMqBuildDisabledError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.rabbitmq_task_bus_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("rabbit_task_modules::ShouldReconnect", consumer)
        self.assertIn("rabbit_task_modules::ShouldRouteToDeadLetter", consumer)
        self.assertIn("rabbit_task_modules::RabbitMqPublishFailedError", consumer)
        for non_consumer in (dispatcher, inline_bus, kafka_bus, redis_bus):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.rabbitmq_task_bus_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_rabbitmq_task_bus_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_rabbitmq_task_bus_algorithms_gtest", test_cmake)

    def test_chat_service_factories_import_backend_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "cxx_modules" / "ServiceFactory.cppm")
        message_consumer = read(CHAT_SERVER / "domain" / "message" / "MessageServiceFactory.cpp")
        relation_consumer = read(CHAT_SERVER / "domain" / "relation" / "RelationServiceFactory.cpp")
        relation_query_consumer = read(CHAT_SERVER / "clients" / "RelationQueryServiceFactory.cpp")
        private_service = read(CHAT_SERVER / "domain" / "message" / "PrivateMessageService.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn("memochat_enable_gnu_modules(ChatMessageCore", cmake)
        self.assertIn("memochat_enable_gnu_modules(ChatRelationCore", cmake)
        self.assertIn(
            "memochat.chat.service_factory_algorithms=domain/cxx_modules/ServiceFactory.cppm",
            cmake,
        )
        self.assertIn("clients/RelationQueryServiceFactory.cpp", cmake)
        self.assertIn("domain/message/MessageServiceFactory.cpp", cmake)
        self.assertIn("domain/relation/RelationServiceFactory.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.service_factory_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsInProcessBackend", module_interface)
        self.assertIn("IsRemoteBackend", module_interface)
        for consumer in (message_consumer, relation_consumer, relation_query_consumer):
            self.assertIsNotNone(
                re.search(r"^\s*import\s+memochat\.chat\.service_factory_algorithms\s*;", consumer, flags=re.M)
            )
            self.assertIn("factory::modules::IsInProcessBackend", consumer)
            self.assertIn("factory::modules::IsRemoteBackend", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.service_factory_algorithms\s*;", private_service, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.service_factory_algorithms\s*;", transport_session, flags=re.M)
        )
        self.assertIn("memochat_enable_gnu_modules(chatserver_message_service_factory_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_relation_service_factory_gtest", test_cmake)
        self.assertIn("chatserver_relation_query_service_factory_gtest", test_cmake)
        self.assertIn("ChatClientCore", test_cmake)

    def test_chat_legacy_grpc_client_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "clients" / "cxx_modules" / "ChatGrpcClient.cppm")
        consumer = read(CHAT_SERVER / "clients" / "ChatGrpcClient.cpp")
        header = read(CHAT_SERVER / "clients" / "ChatGrpcClient.hpp")
        message_client = read(CHAT_SERVER / "clients" / "MessageGrpcClient.cpp")
        relation_client = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        relation_query_client = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.cpp")
        factory = read(CHAT_SERVER / "clients" / "RelationQueryServiceFactory.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatGrpcClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn(
            "memochat.chat.chat_grpc_client_algorithms=clients/cxx_modules/ChatGrpcClient.cppm",
            cmake,
        )
        self.assertIn("clients/ChatGrpcClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.chat_grpc_client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldSkipSelfNode", module_interface)
        self.assertIn("DefaultRemotePoolSize", module_interface)
        self.assertIn("ShouldSkipRemoteCall", module_interface)
        self.assertIn("ShouldReportGrpcStatusError", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.chat_grpc_client_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertNotIn('#include "CSession.hpp"', consumer)
        self.assertNotIn('#include "UserMgr.hpp"', consumer)
        self.assertIn("chat_grpc_modules::ShouldSkipSelfNode", consumer)
        self.assertIn("chat_grpc_modules::DefaultRemotePoolSize", consumer)
        self.assertIn("chat_grpc_modules::ShouldSkipRemoteCall", consumer)
        self.assertIn("chat_grpc_modules::ShouldReportGrpcStatusError", consumer)
        for non_consumer in (header, message_client, relation_client, relation_query_client, factory):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.chat_grpc_client_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_chat_grpc_client_algorithms_gtest", test_cmake)
        self.assertIn("ChatGrpcClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.chat_grpc_client_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.chat_grpc_client_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_message_grpc_client_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "clients" / "cxx_modules" / "MessageGrpcClient.cppm")
        consumer = read(CHAT_SERVER / "clients" / "MessageGrpcClient.cpp")
        header = read(CHAT_SERVER / "clients" / "MessageGrpcClient.hpp")
        relation_client = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        relation_query_client = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.cpp")
        factory = read(CHAT_SERVER / "clients" / "RelationQueryServiceFactory.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "MessageGrpcClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn(
            "memochat.chat.message_grpc_client_algorithms=clients/cxx_modules/MessageGrpcClient.cppm",
            cmake,
        )
        self.assertIn("clients/MessageGrpcClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.message_grpc_client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("PrivateCommandMethodName", module_interface)
        self.assertIn("GroupCommandMethodName", module_interface)
        self.assertIn("BuildGroupListMethod", module_interface)
        self.assertIn("RemoteErrorCodeField", module_interface)
        self.assertIn("InvalidPayloadJsonMessage", module_interface)
        self.assertIn("PrivateStubNotConfiguredMessage", module_interface)
        self.assertIn("GroupStubNotConfiguredMessage", module_interface)
        self.assertIn("PrivateBusinessErrorMessage", module_interface)
        self.assertIn("GroupBusinessErrorMessage", module_interface)
        self.assertIn("ShouldReportStatusError", module_interface)
        self.assertIn("ShouldReportBusinessError", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.message_grpc_client_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("message_grpc_modules::PrivateCommandMethodName", consumer)
        self.assertIn("message_grpc_modules::GroupCommandMethodName", consumer)
        self.assertIn("message_grpc_modules::BuildGroupListMethod", consumer)
        self.assertIn("message_grpc_modules::RemoteErrorCodeField", consumer)
        self.assertIn("message_grpc_modules::GroupBusinessErrorMessage", consumer)
        for non_consumer in (header, relation_client, relation_query_client, factory):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.message_grpc_client_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_message_grpc_client_algorithms_gtest", test_cmake)
        self.assertIn("MessageGrpcClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.message_grpc_client_algorithms", test_cmake)
        self.assertIn("ChatClientCore", test_cmake)
        self.assertNotIn(
            "${CMAKE_SOURCE_DIR}/apps/server/core/ChatServer/clients/MessageGrpcClient.cpp",
            test_cmake,
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.message_grpc_client_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_grpc_client_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "clients" / "cxx_modules" / "RelationGrpcClient.cppm")
        consumer = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        header = read(CHAT_SERVER / "clients" / "RelationGrpcClient.hpp")
        relation_query_client = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.cpp")
        factory = read(CHAT_SERVER / "clients" / "RelationQueryServiceFactory.cpp")
        message_client = read(CHAT_SERVER / "clients" / "MessageGrpcClient.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "RelationGrpcClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn(
            "memochat.chat.relation_grpc_client_algorithms=clients/cxx_modules/RelationGrpcClient.cppm",
            cmake,
        )
        self.assertIn("clients/RelationGrpcClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_grpc_client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("QueryMethodName", module_interface)
        self.assertIn("CommandMethodName", module_interface)
        self.assertIn("RemoteErrorCodeField", module_interface)
        self.assertIn("InvalidPayloadJsonMessage", module_interface)
        self.assertIn("StubNotConfiguredMessage", module_interface)
        self.assertIn("BusinessErrorMessage", module_interface)
        self.assertIn("ShouldReportStatusError", module_interface)
        self.assertIn("ShouldReportBusinessError", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_grpc_client_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_grpc_modules::QueryMethodName", consumer)
        self.assertIn("relation_grpc_modules::CommandMethodName", consumer)
        self.assertIn("relation_grpc_modules::RemoteErrorCodeField", consumer)
        self.assertIn("relation_grpc_modules::BusinessErrorMessage", consumer)
        for non_consumer in (header, relation_query_client, factory, message_client):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_grpc_client_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_grpc_client_algorithms_gtest", test_cmake)
        self.assertIn("RelationGrpcClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_grpc_client_algorithms", test_cmake)
        self.assertIn("ChatClientCore", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_grpc_client_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_grpc_service_adapter_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "clients" / "cxx_modules" / "RelationGrpcServiceAdapter.cppm")
        consumer = read(CHAT_SERVER / "clients" / "RelationGrpcServiceAdapter.cpp")
        header = read(CHAT_SERVER / "clients" / "RelationGrpcServiceAdapter.hpp")
        message_adapter = read(CHAT_SERVER / "clients" / "MessageGrpcServiceAdapter.cpp")
        relation_client = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "RelationGrpcServiceAdapterAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn(
            "memochat.chat.relation_grpc_service_adapter_algorithms=clients/cxx_modules/RelationGrpcServiceAdapter.cppm",
            cmake,
        )
        self.assertIn("clients/RelationGrpcServiceAdapter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_grpc_service_adapter_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("RelationQueryForwardCount", module_interface)
        self.assertIn("RelationCommandForwardCount", module_interface)
        self.assertIn("DefaultRelationAdapterTimeoutMs", module_interface)
        self.assertIn("DidForwardExpectedRelationSurface", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_grpc_service_adapter_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_service_adapter_modules::RelationQueryForwardCount", consumer)
        self.assertIn("relation_service_adapter_modules::RelationCommandForwardCount", consumer)
        self.assertIn("relation_service_adapter_modules::UsesDefaultRelationAdapterTimeout", consumer)
        for non_consumer in (header, message_adapter, relation_client, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_grpc_service_adapter_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_grpc_service_adapter_algorithms_gtest", test_cmake)
        self.assertIn("RelationGrpcServiceAdapterAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_grpc_service_adapter_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_grpc_service_adapter_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_relation_query_grpc_client_imports_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "clients" / "cxx_modules" / "RelationQueryGrpcClient.cppm")
        consumer = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.cpp")
        header = read(CHAT_SERVER / "clients" / "RelationQueryGrpcClient.hpp")
        factory = read(CHAT_SERVER / "clients" / "RelationQueryServiceFactory.cpp")
        relation_client = read(CHAT_SERVER / "clients" / "RelationGrpcClient.cpp")
        message_client = read(CHAT_SERVER / "clients" / "MessageGrpcClient.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "RelationQueryGrpcClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatClientCore", cmake)
        self.assertIn(
            "memochat.chat.relation_query_grpc_client_algorithms=clients/cxx_modules/RelationQueryGrpcClient.cppm",
            cmake,
        )
        self.assertIn("clients/RelationQueryGrpcClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_query_grpc_client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("QueryMethodName", module_interface)
        self.assertIn("RemoteErrorCodeField", module_interface)
        self.assertIn("ShouldMergePayload", module_interface)
        self.assertIn("ShouldReportInvalidPayload", module_interface)
        self.assertIn("ShouldReportMissingStub", module_interface)
        self.assertIn("ShouldReportStatusError", module_interface)
        self.assertIn("ShouldReportBusinessError", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_query_grpc_client_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("relation_query_grpc_modules::QueryMethodName", consumer)
        self.assertIn("relation_query_grpc_modules::ErrorField", consumer)
        self.assertIn("relation_query_grpc_modules::ShouldMergePayload", consumer)
        self.assertIn("relation_query_grpc_modules::ShouldReportStatusError", consumer)
        for non_consumer in (header, factory, relation_client, message_client):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_query_grpc_client_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_query_grpc_client_algorithms_gtest", test_cmake)
        self.assertIn("RelationQueryGrpcClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_query_grpc_client_algorithms", test_cmake)
        self.assertIn("ChatClientCore", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_query_grpc_client_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_persistence_core_imports_relation_bootstrap_cache_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "RelationBootstrapCache.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "RedisRelationBootstrapCache.cpp")
        online_route_store = read(CHAT_SERVER / "persistence" / "RedisOnlineRouteStore.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.relation_bootstrap_cache_algorithms=persistence/cxx_modules/RelationBootstrapCache.cppm",
            cmake,
        )
        self.assertIn("persistence/RedisRelationBootstrapCache.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_bootstrap_cache_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::relation_bootstrap_cache::modules",
            module_interface,
        )
        self.assertIn("SchemaVersion", module_interface)
        self.assertIn("IsValidUid", module_interface)
        self.assertIn("SelectTtlSec", module_interface)
        self.assertIn("IsCurrentSchemaVersion", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_bootstrap_cache_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("cache_modules::SchemaVersion", consumer)
        self.assertIn("cache_modules::IsValidUid", consumer)
        self.assertIn("cache_modules::SelectTtlSec", consumer)
        self.assertIn("cache_modules::IsCurrentSchemaVersion", consumer)
        self.assertIsNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_bootstrap_cache_algorithms\s*;",
                online_route_store,
                flags=re.M,
            )
        )
        self.assertIsNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_bootstrap_cache_algorithms\s*;",
                transport_session,
                flags=re.M,
            )
        )
        self.assertIn("chatserver_relation_bootstrap_cache_algorithms_gtest", test_cmake)
        self.assertIn("RelationBootstrapCacheAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_bootstrap_cache_algorithms", test_cmake)

    def test_chat_persistence_core_imports_online_route_store_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "OnlineRouteStore.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "RedisOnlineRouteStore.cpp")
        redis_mgr_header = read(CHAT_SERVER / "persistence" / "RedisMgr.hpp")
        session_support = read(CHAT_SERVER / "persistence" / "OnlineRouteSessionSupport.cpp")
        session_support_header = read(CHAT_SERVER / "persistence" / "OnlineRouteSessionSupport.hpp")
        relation_bootstrap = read(CHAT_SERVER / "persistence" / "RedisRelationBootstrapCache.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "OnlineRouteStoreAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.online_route_store_algorithms=persistence/cxx_modules/OnlineRouteStore.cppm",
            cmake,
        )
        self.assertIn("persistence/OnlineRouteSessionSupport.cpp", cmake)
        self.assertIn("persistence/RedisOnlineRouteStore.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.online_route_store_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::online_route_store::modules", module_interface)
        for helper in (
            "IsValidUid",
            "ShouldCheckRepairSession",
            "HasUsableServerName",
            "ShouldReadUserRoute",
            "ShouldSearchOnlineSets",
            "ShouldWriteUserRoute",
            "ShouldClearTrackedRoute",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.online_route_store_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("online_route_modules::ShouldCheckRepairSession", consumer)
        self.assertIn("online_route_modules::ShouldReadUserRoute", consumer)
        self.assertIn("online_route_modules::ShouldWriteUserRoute", consumer)
        self.assertIn("ExtractOnlineRouteSessionId(session)", consumer)
        self.assertIn("RepairOnlineRouteAtomic(uid_str, server_name, ExtractOnlineRouteSessionId(session))", consumer)
        self.assertIn("memochat::chat::lua_scripts::krepair_online_route", consumer)
        self.assertIn("RedisMgr::GetInstance()->getRawConnection()", consumer)
        self.assertIn("RedisMgr::GetInstance()->returnConnection(ctx)", consumer)
        self.assertIn("redisContext* getRawConnection()", redis_mgr_header)
        self.assertIn("void returnConnection(redisContext* ctx)", redis_mgr_header)
        self.assertNotIn('#include "CSession.hpp"', consumer)
        self.assertIn('#include "CSession.hpp"', session_support)
        self.assertIn("class CSession;", session_support_header)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.online_route_store_algorithms\s*;", session_support, flags=re.M)
        )
        for non_consumer in (relation_bootstrap, transport_session):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.online_route_store_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_online_route_store_algorithms_gtest", test_cmake)
        self.assertIn("OnlineRouteStoreAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.online_route_store_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.online_route_store_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_persistence_core_imports_relation_repository_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "RelationRepository.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "ChatRelationRepository.cpp")
        mongo_mgr = read(CHAT_SERVER / "persistence" / "MongoMgr.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "RelationRepositoryAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.relation_repository_algorithms=persistence/cxx_modules/RelationRepository.cppm",
            cmake,
        )
        self.assertIn("persistence/ChatRelationRepository.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.relation_repository_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::relation_repository::modules", module_interface)
        for helper in (
            "IsPositiveUid",
            "IsPositiveGroupId",
            "ShouldQueryPrivateFriendship",
            "ShouldFilterFriendUids",
            "ShouldQueryGroupMembership",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.relation_repository_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("relation_repository_modules::ShouldQueryPrivateFriendship", consumer)
        self.assertIn("relation_repository_modules::ShouldFilterFriendUids", consumer)
        self.assertIn("relation_repository_modules::ShouldQueryGroupMembership", consumer)
        for non_consumer in (mongo_mgr, postgres_mgr, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.relation_repository_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_relation_repository_algorithms_gtest", test_cmake)
        self.assertIn("RelationRepositoryAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.relation_repository_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.relation_repository_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_persistence_core_imports_session_repository_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "ChatSessionRepository.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "ChatSessionRepository.cpp")
        redis_mgr = read(CHAT_SERVER / "persistence" / "RedisMgr.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.session_repository_algorithms=persistence/cxx_modules/ChatSessionRepository.cppm",
            cmake,
        )
        self.assertIn("persistence/ChatSessionRepository.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.session_repository_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::session_repository::modules",
            module_interface,
        )
        self.assertIn("ShouldAcquireDuplicateLoginLock", module_interface)
        self.assertIn("ShouldReleaseDuplicateLoginLock", module_interface)
        self.assertIn("ShouldQueryUndeliveredPrivateMessages", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.session_repository_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("session_repository_modules::ShouldAcquireDuplicateLoginLock", consumer)
        self.assertIn("session_repository_modules::ShouldReleaseDuplicateLoginLock", consumer)
        self.assertIn("session_repository_modules::ShouldQueryUndeliveredPrivateMessages", consumer)
        for non_consumer in (redis_mgr, postgres_mgr, transport_session):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.session_repository_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_session_repository_algorithms_gtest", test_cmake)
        self.assertIn("ChatSessionRepositoryAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.session_repository_algorithms", test_cmake)

    def test_chat_persistence_core_imports_message_repository_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "MessageRepository.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "ChatMessageRepository.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        mongo_mgr = read(CHAT_SERVER / "persistence" / "MongoMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.message_repository_algorithms=persistence/cxx_modules/MessageRepository.cppm",
            cmake,
        )
        self.assertIn("persistence/ChatMessageRepository.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.message_repository_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::message_repository::modules", module_interface)
        for helper in (
            "ShouldAbortAfterPrimaryWrite",
            "ShouldAttemptMongo",
            "HasLoadedMessage",
            "ShouldFallbackToPostgres",
            "MergeReadSuccess",
            "ShouldLogMongoWriteFailure",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.message_repository_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("message_repository_modules::ShouldAbortAfterPrimaryWrite", consumer)
        self.assertIn("message_repository_modules::HasLoadedMessage", consumer)
        self.assertIn("message_repository_modules::ShouldFallbackToPostgres", consumer)
        self.assertIn("message_repository_modules::ShouldLogMongoWriteFailure", consumer)
        for non_consumer in (postgres_mgr, mongo_mgr, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.message_repository_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_message_repository_algorithms_gtest", test_cmake)
        self.assertIn("MessageRepositoryAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.message_repository_algorithms", test_cmake)

    def test_chat_persistence_core_imports_mongo_dao_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "MongoDao.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        mongo_mgr = read(CHAT_SERVER / "persistence" / "MongoMgr.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.mongo_dao_algorithms=persistence/cxx_modules/MongoDao.cppm",
            cmake,
        )
        self.assertIn("persistence/MongoDao.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.mongo_dao_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::mongo_dao::modules", module_interface)
        for helper in (
            "ParseBoolText",
            "ClampHistoryLimit",
            "ShouldApplyPrivateTieBreaker",
            "CanRequestPrivateMessageRevoke",
            "SelectOperationTimestamp",
            "IsPrivateMessageOwner",
            "CanRevokePrivateMessage",
            "ShouldApplyGroupSeqFilter",
            "CanApplyGroupMessageRevoke",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.mongo_dao_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("mongo_dao_modules::ParseBoolText", consumer)
        self.assertIn("mongo_dao_modules::ClampHistoryLimit", consumer)
        self.assertIn("mongo_dao_modules::CanRevokePrivateMessage", consumer)
        self.assertIn("mongo_dao_modules::CanApplyGroupMessageRevoke", consumer)
        for non_consumer in (mongo_mgr, postgres_mgr, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.mongo_dao_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_mongo_dao_algorithms_gtest", test_cmake)
        self.assertIn("MongoDaoAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.mongo_dao_algorithms", test_cmake)

    def test_chat_persistence_core_imports_mongo_mgr_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "MongoMgr.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "MongoMgr.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_consumer = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "MongoMgrAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.mongo_mgr_algorithms=persistence/cxx_modules/MongoMgr.cppm",
            cmake,
        )
        self.assertIn("persistence/MongoMgr.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.mongo_mgr_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::mongo_mgr::modules", module_interface)
        for helper in (
            "EnabledForwardCount",
            "PrivateMessageForwardCount",
            "GroupMessageForwardCount",
            "ForwardingSurfaceCount",
            "IsCompleteForwardingSurface",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.mongo_mgr_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("mongo_mgr_modules::IsCompleteForwardingSurface", consumer)
        self.assertIn("mongo_mgr_modules::ForwardingSurfaceCount() == 11", consumer)
        for non_consumer in (mongo_dao, postgres_mgr, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.mongo_mgr_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_mongo_mgr_algorithms_gtest", test_cmake)
        self.assertIn("MongoMgrAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.mongo_mgr_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.mongo_mgr_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_chat_persistence_core_imports_postgres_mgr_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresMgr.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_private = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        postgres_group_messages = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        postgres_dialogs = read(CHAT_SERVER / "persistence" / "PostgresDaoDialogs.cpp")
        postgres_groups = read(CHAT_SERVER / "persistence" / "PostgresDaoGroups.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_mgr_algorithms=persistence/cxx_modules/PostgresMgr.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresMgr.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_mgr_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::postgres_mgr::modules", module_interface)
        for helper in (
            "ShouldResetDao",
            "ShouldInitializeDao",
            "InitFailureEvent",
            "InitFailureMessage",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.postgres_mgr_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_mgr_modules::ShouldResetDao", consumer)
        self.assertIn("postgres_mgr_modules::ShouldInitializeDao", consumer)
        self.assertIn("postgres_mgr_modules::InitFailureEvent", consumer)
        for non_consumer in (
            postgres_dao,
            postgres_users,
            postgres_private,
            postgres_group_messages,
            postgres_dialogs,
            postgres_groups,
            mongo_dao,
            transport_session,
        ):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_mgr_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_mgr_algorithms_gtest", test_cmake)
        self.assertIn("PostgresMgrAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_mgr_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDao.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_private = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        postgres_group_messages = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        postgres_dialogs = read(CHAT_SERVER / "persistence" / "PostgresDaoDialogs.cpp")
        postgres_groups = read(CHAT_SERVER / "persistence" / "PostgresDaoGroups.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_algorithms=persistence/cxx_modules/PostgresDao.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDao.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::postgres_dao::modules", module_interface)
        for helper in (
            "ShouldUseFallbackSection",
            "HasPostgresHost",
            "ShouldEnablePostgres",
            "DefaultSslMode",
            "DefaultSchema",
            "SelectSslMode",
            "SelectSchema",
            "StartupPoolSize",
            "ShouldUsePostgresWarmupPath",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.postgres_dao_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_dao_modules::ShouldUseFallbackSection", consumer)
        self.assertIn("postgres_dao_modules::SelectSslMode", consumer)
        self.assertIn("postgres_dao_modules::StartupPoolSize", consumer)
        self.assertIn("postgres_dao_modules::ShouldUsePostgresWarmupPath", consumer)
        for non_consumer in (
            postgres_mgr,
            postgres_users,
            postgres_private,
            postgres_group_messages,
            postgres_dialogs,
            postgres_groups,
            mongo_dao,
            transport_session,
        ):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_users_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDaoUsers.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_users_algorithms=persistence/cxx_modules/PostgresDaoUsers.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoUsers.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_users_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::postgres_dao_users::modules", module_interface)
        for helper in (
            "IsValidUserPublicIdShape",
            "HasPositiveUid",
        ):
            self.assertIn(helper, module_interface)
        for removed_helper in (
            "LegacyXorCode",
            "DecodeLegacyXorChar",
            "ShouldAttemptLegacyXorDecode",
            "IsPasswordAccepted",
        ):
            self.assertNotIn(removed_helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.postgres_dao_users_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_dao_users_modules::IsValidUserPublicIdShape", consumer)
        self.assertIn("postgres_dao_users_modules::HasPositiveUid", consumer)
        self.assertIn('#include "auth/PasswordHasher.hpp"', consumer)
        self.assertIn("memochat::auth::HashPassword(pwd, password_hash)", consumer)
        self.assertIn("memochat::auth::HashPassword(newpwd, password_hash)", consumer)
        self.assertIn("memochat::auth::VerifyPassword(password_hash, pwd)", consumer)
        self.assertIn("password_hash", consumer)
        self.assertIn("pwd = ''", consumer)
        self.assertIn("userInfo.pwd.clear()", consumer)
        self.assertNotIn("if (pwd != origin_pwd)", consumer)
        self.assertNotIn("postgres_dao_users_modules::DecodeLegacyXorChar", consumer)
        self.assertNotIn("postgres_dao_users_modules::ShouldAttemptLegacyXorDecode", consumer)
        self.assertNotIn("postgres_dao_users_modules::IsPasswordAccepted", consumer)
        self.assertNotIn("DecodeLegacyXorPwd", consumer)
        for non_consumer in (postgres_mgr, postgres_dao, mongo_dao, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_users_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_users_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoUsersAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_users_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_private_messages_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDaoPrivateMessages.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_groups = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_private_messages_algorithms="
            "persistence/cxx_modules/PostgresDaoPrivateMessages.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoPrivateMessages.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_private_messages_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::postgres_dao_private_messages::modules",
            module_interface,
        )
        for helper in (
            "CanReadUndeliveredPrivateMessages",
            "CanReadPrivateHistory",
            "SelectHistoryFetchLimit",
            "ShouldApplyHistoryTieBreaker",
            "HasOverflowPage",
            "CanUpdatePrivateMessage",
            "IsPrivateMessageOwner",
            "CanRevokePrivateMessage",
            "CanUpsertPrivateReadState",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.postgres_dao_private_messages_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("postgres_dao_private_messages_modules::CanReadPrivateHistory", consumer)
        self.assertIn("postgres_dao_private_messages_modules::SelectHistoryFetchLimit", consumer)
        self.assertIn("postgres_dao_private_messages_modules::CanUpdatePrivateMessage", consumer)
        self.assertIn("postgres_dao_private_messages_modules::CanRevokePrivateMessage", consumer)
        self.assertIn("postgres_dao_private_messages_modules::CanUpsertPrivateReadState", consumer)
        for non_consumer in (postgres_mgr, postgres_dao, postgres_users, postgres_groups, transport_session):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_private_messages_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_private_messages_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoPrivateMessagesAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_private_messages_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_group_messages_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDaoGroupMessages.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_private = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_group_messages_algorithms="
            "persistence/cxx_modules/PostgresDaoGroupMessages.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoGroupMessages.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_group_messages_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::postgres_dao_group_messages::modules",
            module_interface,
        )
        for helper in (
            "CanSaveGroupMessage",
            "NormalizeNextGroupSeq",
            "ShouldWriteGroupMessageExt",
            "CanOperatorEditGroupMessage",
            "CanRevokeGroupMessage",
            "ClampHistoryLimit",
            "ShouldApplyGroupSeqCursor",
            "CanFindGroupMessage",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.postgres_dao_group_messages_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("postgres_dao_group_messages_modules::CanSaveGroupMessage", consumer)
        self.assertIn("postgres_dao_group_messages_modules::NormalizeNextGroupSeq", consumer)
        self.assertIn("postgres_dao_group_messages_modules::CanOperatorEditGroupMessage", consumer)
        self.assertIn("postgres_dao_group_messages_modules::CanRevokeGroupMessage", consumer)
        self.assertIn("postgres_dao_group_messages_modules::ClampHistoryLimit", consumer)
        self.assertIn("postgres_dao_group_messages_modules::CanFindGroupMessage", consumer)
        for non_consumer in (
            postgres_mgr,
            postgres_dao,
            postgres_users,
            postgres_private,
            mongo_dao,
            transport_session,
        ):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_group_messages_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_group_messages_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoGroupMessagesAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_group_messages_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_dialogs_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDaoDialogs.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDaoDialogs.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_private = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        postgres_group = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_dialogs_algorithms=persistence/cxx_modules/PostgresDaoDialogs.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoDialogs.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_dialogs_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::postgres_dao_dialogs::modules",
            module_interface,
        )
        for helper in (
            "CanLoadDialogMeta",
            "ConversationUidMin",
            "NormalizeUnreadCount",
            "CanUpsertGroupReadState",
            "CanUpsertDialogMeta",
            "NormalizePinnedRank",
            "ClampGroupApplyLimit",
            "HasGroupCodeHeader",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.postgres_dao_dialogs_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("postgres_dao_dialogs_modules::CanLoadDialogMeta", consumer)
        self.assertIn("postgres_dao_dialogs_modules::ConversationUidMin", consumer)
        self.assertIn("postgres_dao_dialogs_modules::NormalizeUnreadCount", consumer)
        self.assertIn("postgres_dao_dialogs_modules::CanUpsertDialogMeta", consumer)
        self.assertIn("postgres_dao_dialogs_modules::ClampGroupApplyLimit", consumer)
        self.assertIn("postgres_dao_dialogs_modules::HasGroupCodeHeader", consumer)
        for non_consumer in (
            postgres_mgr,
            postgres_dao,
            postgres_users,
            postgres_private,
            postgres_group,
            mongo_dao,
            transport_session,
        ):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_dialogs_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_dialogs_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoDialogsAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_dialogs_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_dao_groups_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresDaoGroups.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresDaoGroups.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        postgres_dao = read(CHAT_SERVER / "persistence" / "PostgresDao.cpp")
        postgres_users = read(CHAT_SERVER / "persistence" / "PostgresDaoUsers.cpp")
        postgres_private = read(CHAT_SERVER / "persistence" / "PostgresDaoPrivateMessages.cpp")
        postgres_group_messages = read(CHAT_SERVER / "persistence" / "PostgresDaoGroupMessages.cpp")
        postgres_dialogs = read(CHAT_SERVER / "persistence" / "PostgresDaoDialogs.cpp")
        mongo_dao = read(CHAT_SERVER / "persistence" / "MongoDao.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_dao_groups_algorithms=persistence/cxx_modules/PostgresDaoGroups.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresDaoGroups.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_dao_groups_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn(
            "export namespace memochat::chat::persistence::postgres_dao_groups::modules",
            module_interface,
        )
        for helper in (
            "CanCreateGroup",
            "ClampMemberLimit",
            "ShouldKeepInitialMember",
            "HasGroupCodeHeader",
            "ReviewedApplyStatus",
            "JoinSourceForApplyType",
            "CanOperatorChangeTargetRole",
            "NormalizeAdminPermissionBitsForRole",
            "CanModerateOperatorRole",
            "ClampMuteUntil",
            "FallbackPermissionBitsForRole",
            "HasRequiredPermissionBits",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.chat\.postgres_dao_groups_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("postgres_dao_groups_modules::CanCreateGroup", consumer)
        self.assertIn("postgres_dao_groups_modules::ClampMemberLimit", consumer)
        self.assertIn("postgres_dao_groups_modules::HasGroupCodeHeader", consumer)
        self.assertIn("postgres_dao_groups_modules::ReviewedApplyStatus", consumer)
        self.assertIn("postgres_dao_groups_modules::NormalizeAdminPermissionBitsForRole", consumer)
        self.assertIn("postgres_dao_groups_modules::CanModerateOperatorRole", consumer)
        self.assertIn("postgres_dao_groups_modules::HasRequiredPermissionBits", consumer)
        for non_consumer in (
            postgres_mgr,
            postgres_dao,
            postgres_users,
            postgres_private,
            postgres_group_messages,
            postgres_dialogs,
            mongo_dao,
            transport_session,
        ):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.chat\.postgres_dao_groups_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("chatserver_postgres_dao_groups_algorithms_gtest", test_cmake)
        self.assertIn("PostgresDaoGroupsAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_dao_groups_algorithms", test_cmake)

    def test_chat_persistence_core_imports_dist_lock_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "DistLock.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "DistLock.cpp")
        redis_mgr = read(CHAT_SERVER / "persistence" / "RedisMgr.cpp")
        session_repository = read(CHAT_SERVER / "persistence" / "ChatSessionRepository.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.dist_lock_algorithms=persistence/cxx_modules/DistLock.cppm",
            cmake,
        )
        self.assertIn("persistence/DistLock.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.dist_lock_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::dist_lock::modules", module_interface)
        self.assertIn("IsAcquireReplyOk", module_interface)
        self.assertIn("IsReleaseReplyDeleted", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.dist_lock_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("dist_lock_modules::IsAcquireReplyOk", consumer)
        self.assertIn("dist_lock_modules::IsReleaseReplyDeleted", consumer)
        for non_consumer in (redis_mgr, session_repository, transport_session):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.dist_lock_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_dist_lock_algorithms_gtest", test_cmake)
        self.assertIn("DistLockAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.dist_lock_algorithms", test_cmake)

    def test_chat_persistence_core_imports_redis_mgr_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "RedisMgr.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "RedisMgr.cpp")
        dist_lock = read(CHAT_SERVER / "persistence" / "DistLock.cpp")
        session_repository = read(CHAT_SERVER / "persistence" / "ChatSessionRepository.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.redis_mgr_algorithms=persistence/cxx_modules/RedisMgr.cppm",
            cmake,
        )
        self.assertIn("persistence/RedisMgr.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.redis_mgr_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::redis_mgr::modules", module_interface)
        for helper in (
            "NormalizeExpireSeconds",
            "IsStatusOk",
            "IsStringReply",
            "IsPositiveIntegerReply",
            "IsArrayReply",
            "ShouldTreatEmptyLockIdentifierAsReleased",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.redis_mgr_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("redis_mgr_modules::NormalizeExpireSeconds", consumer)
        self.assertIn("redis_mgr_modules::IsStatusOk", consumer)
        self.assertIn("redis_mgr_modules::IsPositiveIntegerReply", consumer)
        self.assertIn("redis_mgr_modules::ShouldTreatEmptyLockIdentifierAsReleased", consumer)
        for non_consumer in (dist_lock, session_repository, transport_session):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.redis_mgr_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_redis_mgr_algorithms_gtest", test_cmake)
        self.assertIn("RedisMgrAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.redis_mgr_algorithms", test_cmake)

    def test_chat_persistence_core_imports_postgres_pool_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "PostgresPool.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "PostgresPool.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        redis_mgr = read(CHAT_SERVER / "persistence" / "RedisMgr.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.postgres_pool_algorithms=persistence/cxx_modules/PostgresPool.cppm",
            cmake,
        )
        self.assertIn("persistence/PostgresPool.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.postgres_pool_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::postgres_pool::modules", module_interface)
        for helper in (
            "ShouldCreateInitialConnection",
            "ConnectionWaitTimeoutSeconds",
            "ShouldReconnect",
            "ShouldWakeForConnection",
            "ShouldReturnNullAfterWait",
            "ShouldAcceptReturnedConnection",
        ):
            self.assertIn(helper, module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.postgres_pool_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("postgres_pool_modules::ShouldCreateInitialConnection", consumer)
        self.assertIn("postgres_pool_modules::ConnectionWaitTimeoutSeconds", consumer)
        self.assertIn("postgres_pool_modules::ShouldWakeForConnection", consumer)
        self.assertIn("postgres_pool_modules::ShouldReturnNullAfterWait", consumer)
        for non_consumer in (postgres_mgr, redis_mgr, transport_session):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.postgres_pool_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_postgres_pool_algorithms_gtest", test_cmake)
        self.assertIn("PostgresPoolAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.postgres_pool_algorithms", test_cmake)

    def test_chat_persistence_core_imports_outbox_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "persistence" / "cxx_modules" / "Outbox.cppm")
        consumer = read(CHAT_SERVER / "persistence" / "ChatOutboxService.cpp")
        postgres_mgr = read(CHAT_SERVER / "persistence" / "PostgresMgr.cpp")
        kafka_bus = read(CHAT_SERVER / "messaging" / "KafkaAsyncEventBus.cpp")
        transport_session = read(CHAT_SERVER / "transport" / "CSession.cpp")
        entrypoint = read(CHAT_SERVER / "app" / "ChatServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatPersistenceCore", cmake)
        self.assertIn(
            "memochat.chat.outbox_algorithms=persistence/cxx_modules/Outbox.cppm",
            cmake,
        )
        self.assertIn("persistence/ChatOutboxService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.outbox_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::persistence::outbox::modules", module_interface)
        self.assertIn("NextRetryCount", module_interface)
        self.assertIn("SelectBackoffMs", module_interface)
        self.assertIn("IsTerminalRetry", module_interface)
        self.assertIn("ShouldScheduleRepairTask", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.chat\.outbox_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("outbox_modules::NextRetryCount", consumer)
        self.assertIn("outbox_modules::SelectBackoffMs", consumer)
        self.assertIn("outbox_modules::IsTerminalRetry", consumer)
        self.assertIn("outbox_modules::ShouldScheduleRepairTask", consumer)
        for non_consumer in (postgres_mgr, kafka_bus, transport_session, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.outbox_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_outbox_algorithms_gtest", test_cmake)
        self.assertIn("OutboxAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.outbox_algorithms", test_cmake)

    def test_chat_user_support_core_imports_user_profile_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "users" / "cxx_modules" / "UserProfile.cppm")
        consumer = read(CHAT_SERVER / "domain" / "users" / "ChatUserProfileDto.cpp")
        support_source = read(CHAT_SERVER / "domain" / "users" / "ChatUserSupport.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatUserSupportCore", cmake)
        self.assertIn(
            "memochat.chat.user_profile_algorithms=domain/users/cxx_modules/UserProfile.cppm",
            cmake,
        )
        self.assertIn("domain/users/ChatUserProfileDto.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.user_profile_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::users::modules", module_interface)
        self.assertNotIn("ShouldProjectPassword", module_interface)
        self.assertIn("ShouldProjectIcon", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.user_profile_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertNotIn("modules::ShouldProjectPassword", consumer)
        self.assertIn("modules::ShouldProjectIcon", consumer)
        self.assertNotIn('out["pwd"]', consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.chat\.user_profile_algorithms\s*;", support_source, flags=re.M)
        )
        self.assertIn("chatserver_user_profile_dto_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(chatserver_user_profile_dto_gtest", test_cmake)

    def test_chat_user_support_core_imports_user_support_algorithms_module(self):
        cmake = read(CHAT_SERVER / "CMakeLists.txt")
        module_interface = read(CHAT_SERVER / "domain" / "users" / "cxx_modules" / "UserSupport.cppm")
        consumer = read(CHAT_SERVER / "domain" / "users" / "ChatUserSupport.cpp")
        profile_dto = read(CHAT_SERVER / "domain" / "users" / "ChatUserProfileDto.cpp")
        user_mgr = read(CHAT_SERVER / "domain" / "users" / "UserMgr.cpp")
        relation_service = read(CHAT_SERVER / "domain" / "relation" / "ChatRelationService.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatUserSupportAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(ChatUserSupportCore", cmake)
        self.assertIn(
            "memochat.chat.user_support_algorithms=domain/users/cxx_modules/UserSupport.cppm",
            cmake,
        )
        self.assertIn("domain/users/ChatUserSupport.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.chat\.user_support_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::chat::user_support::modules", module_interface)
        self.assertIn("IsAsciiDigit", module_interface)
        self.assertIn("ShouldUseCachedProfile", module_interface)
        self.assertIn("ShouldReportMissingUser", module_interface)
        self.assertIn("FriendApplyLimit", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.user_support_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("user_support_modules::IsAsciiDigit", consumer)
        self.assertIn("user_support_modules::ShouldUseCachedProfile", consumer)
        self.assertIn("user_support_modules::FriendApplyLimit", consumer)
        for non_consumer in (profile_dto, user_mgr, relation_service):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.chat\.user_support_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("chatserver_user_support_algorithms_gtest", test_cmake)
        self.assertIn("ChatUserSupportAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.chat.user_support_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.chat\.user_support_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_media_service_imports_media_config_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(MEDIA_SERVICE / "core" / "cxx_modules" / "MediaConfig.cppm")
        storage_consumer = read(MEDIA_SERVICE / "core" / "storage" / "MediaStorage.cpp")
        support_consumer = read(MEDIA_SERVICE / "core" / "support" / "Http2MediaSupport.cpp")
        service_consumer = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaCore", cmake)
        self.assertIn("memochat_enable_gnu_modules(GateMediaDomain", cmake)
        self.assertIn("memochat.media.config_algorithms=core/cxx_modules/MediaConfig.cppm", cmake)
        self.assertIn("core/storage/MediaStorage.cpp", cmake)
        self.assertIn("core/support/Http2MediaSupport.cpp", cmake)
        self.assertIn("domain/services/media/MediaService.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.media\.config_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("LowerAsciiInPlace", module_interface)
        self.assertIn("ClampInt", module_interface)
        for consumer in (storage_consumer, support_consumer, service_consumer):
            self.assertIsNotNone(
                re.search(r"^\s*import\s+memochat\.media\.config_algorithms\s*;", consumer, flags=re.M)
            )
        self.assertIn("mediaservice_storage_gtest", test_cmake)

    def test_media_service_imports_public_dto_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(MEDIA_SERVICE / "domain" / "services" / "media" / "cxx_modules" / "MediaPublicDto.cppm")
        consumer = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaPublicDtos.cpp")
        service_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        route_source = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteModule.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaCore", cmake)
        self.assertIn(
            "memochat.media.public_dto_algorithms=domain/services/media/cxx_modules/MediaPublicDto.cppm",
            cmake,
        )
        self.assertIn("domain/services/media/MediaPublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.media\.public_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::media::public_dto::modules", module_interface)
        self.assertIn("ShouldUseDefaultMediaType", module_interface)
        self.assertIn("ContainsApplicationJson", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.public_dto_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("public_dto::modules::ShouldUseDefaultMediaType", consumer)
        self.assertIn("public_dto::modules::ContainsApplicationJson", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.media\.public_dto_algorithms\s*;", service_source, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.media\.public_dto_algorithms\s*;", route_source, flags=re.M)
        )
        self.assertIn("mediaservice_public_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(mediaservice_public_dtos_gtest", test_cmake)

    def test_media_service_imports_upload_session_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MEDIA_SERVICE / "domain" / "services" / "media" / "cxx_modules" / "MediaUploadSession.cppm"
        )
        consumer = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaUploadSessionDto.cpp")
        public_dto_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaPublicDtos.cpp")
        service_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaCore", cmake)
        self.assertIn(
            "memochat.media.upload_session_algorithms=domain/services/media/cxx_modules/MediaUploadSession.cppm",
            cmake,
        )
        self.assertIn("domain/services/media/MediaUploadSessionDto.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.media\.upload_session_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::media::upload_session::modules", module_interface)
        self.assertIn("ShouldUseDefaultMediaType", module_interface)
        self.assertIn("ShouldUseDefaultStorageProvider", module_interface)
        self.assertIn("HasValidUploadSessionShape", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.upload_session_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("upload_session::modules::ShouldUseDefaultMediaType", consumer)
        self.assertIn("upload_session::modules::ShouldUseDefaultStorageProvider", consumer)
        self.assertIn("upload_session::modules::HasValidUploadSessionShape", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.media\.upload_session_algorithms\s*;", public_dto_source, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.media\.upload_session_algorithms\s*;", service_source, flags=re.M)
        )
        self.assertIn("mediaservice_upload_session_dto_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(mediaservice_upload_session_dto_gtest", test_cmake)

    def test_media_service_imports_service_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(MEDIA_SERVICE / "domain" / "services" / "media" / "cxx_modules" / "MediaService.cppm")
        consumer = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        public_dto_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaPublicDtos.cpp")
        upload_session_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaUploadSessionDto.cpp")
        route_source = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteModule.cpp")
        route_schema_source = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteSchemas.cpp")
        http_service_source = read(MEDIA_SERVICE / "domain" / "MediaHttpService.cpp")
        entrypoint = read(MEDIA_SERVICE / "app" / "MediaGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MediaService"
            / "MediaServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaDomain", cmake)
        self.assertIn(
            "memochat.media.service_algorithms=domain/services/media/cxx_modules/MediaService.cppm",
            cmake,
        )
        self.assertIn("domain/services/media/MediaService.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.media\.service_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::media::service::modules", module_interface)
        self.assertIn("HasValidUploadInitRequest", module_interface)
        self.assertIn("HasValidChunkUploadRequest", module_interface)
        self.assertIn("IsReadableAsset", module_interface)
        self.assertIn("ShouldRedirectToPublicUrl", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.media\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("service_modules::HasValidUploadInitRequest", consumer)
        self.assertIn("service_modules::HasValidChunkUploadRequest", consumer)
        self.assertIn("service_modules::IsReadableAsset", consumer)
        self.assertIn("service_modules::ShouldRedirectToPublicUrl", consumer)
        for non_consumer in (
            public_dto_source,
            upload_session_source,
            route_source,
            route_schema_source,
            http_service_source,
            entrypoint,
        ):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.media\.service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("mediaservice_service_algorithms_gtest", test_cmake)
        self.assertIn("MediaServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(mediaservice_service_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_media_service_imports_route_schema_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MEDIA_SERVICE / "domain" / "modules" / "media" / "cxx_modules" / "MediaRouteSchema.cppm"
        )
        consumer = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteSchemas.cpp")
        route_source = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteModule.cpp")
        service_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        entrypoint = read(MEDIA_SERVICE / "app" / "MediaGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MediaService"
            / "MediaRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaDomain", cmake)
        self.assertIn(
            "memochat.media.route_schema_algorithms=domain/modules/media/cxx_modules/MediaRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/media/MediaRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.media\.route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::media::route_schema::modules", module_interface)
        self.assertIn("UploadInitPath", module_interface)
        self.assertIn("UploadChunkJsonRouteName", module_interface)
        self.assertIn("UploadAssetResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::UploadInitPath", consumer)
        self.assertIn("modules::UploadChunkJsonRouteName", consumer)
        self.assertIn("modules::UploadAssetResponseTypeName", consumer)
        for non_consumer in (route_source, service_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.media\.route_schema_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("mediaservice_route_schema_gtest", test_cmake)
        self.assertIn("MediaRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(mediaservice_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.route_schema_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_media_service_imports_route_registration_algorithms_module(self):
        cmake = read(MEDIA_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MEDIA_SERVICE / "domain" / "modules" / "media" / "cxx_modules" / "MediaRouteRegistration.cppm"
        )
        consumer = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteModule.cpp")
        schema_source = read(MEDIA_SERVICE / "domain" / "modules" / "media" / "MediaRouteSchemas.cpp")
        service_source = read(MEDIA_SERVICE / "domain" / "services" / "media" / "MediaService.cpp")
        entrypoint = read(MEDIA_SERVICE / "app" / "MediaGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MediaService"
            / "MediaRouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MediaService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateMediaDomain", cmake)
        self.assertIn(
            "memochat.media.route_registration_algorithms=domain/modules/media/cxx_modules/MediaRouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/media/MediaRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.media\.route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::media::route_registration::modules", module_interface)
        self.assertIn("UploadInitPath", module_interface)
        self.assertIn("UploadStatusPath", module_interface)
        self.assertIn("MediaDownloadPath", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.route_registration_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::UploadInitPath", consumer)
        self.assertIn("modules::UploadStatusPath", consumer)
        self.assertIn("modules::MediaDownloadPath", consumer)
        for non_consumer in (schema_source, service_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.media\.route_registration_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("mediaservice_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("MediaRouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(mediaservice_route_registration_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.media\.route_registration_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_source_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18Source.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn("memochat.r18.source_algorithms=domain/services/r18/cxx_modules/R18Source.cppm", cmake)
        self.assertIn("domain/services/r18/R18SourceService.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.r18\.source_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("NormalizeSourceIdChar", module_interface)
        self.assertIn("NormalizePathSegmentChar", module_interface)
        self.assertIn("LowerAsciiInPlace", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.r18\.source_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::NormalizeSourceIdChar", consumer)
        self.assertIn("modules::NormalizePathSegmentChar", consumer)
        self.assertIn("r18service_source_service_gtest", test_cmake)

    def test_r18_service_imports_adapter_utils_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18AdapterUtils.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18AdapterUtils.cpp")
        public_dtos = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PublicDtos.cpp")
        source_record_codec = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceRecordCodec.cpp")
        source_service = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "R18Service"
            / "R18AdapterUtilsAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn(
            "memochat.r18.adapter_utils_algorithms=domain/services/r18/cxx_modules/R18AdapterUtils.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18AdapterUtils.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.adapter_utils_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::adapter_utils::modules", module_interface)
        self.assertIn("DefaultUserAgent", module_interface)
        self.assertIn("SelectDefaultPort", module_interface)
        self.assertIn("IsUrlEncodeUnreserved", module_interface)
        self.assertIn("XmlEscapeText", module_interface)
        self.assertIn("DefaultCachedImageContentType", module_interface)
        self.assertIn("Base64InvalidMarker", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.adapter_utils_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("adapter_utils::modules::DefaultUserAgent", consumer)
        self.assertIn("adapter_utils::modules::SelectDefaultPort", consumer)
        self.assertIn("adapter_utils::modules::IsUrlEncodeUnreserved", consumer)
        self.assertIn("adapter_utils::modules::XmlEscapeText", consumer)
        self.assertIn("adapter_utils::modules::DefaultCachedImageContentType", consumer)
        for non_consumer in (
            public_dtos,
            source_record_codec,
            source_service,
            service_source,
            route_source,
            entrypoint,
        ):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.adapter_utils_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_adapter_utils_algorithms_gtest", test_cmake)
        self.assertIn("R18AdapterUtilsAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_adapter_utils_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.adapter_utils_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_jm_adapter_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18JmAdapter.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18JmAdapter.cpp")
        adapter_utils = read(R18_SERVICE / "domain" / "services" / "r18" / "R18AdapterUtils.cpp")
        picacg_adapter = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PicacgAdapter.cpp")
        source_service = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "R18Service"
            / "R18JmAdapterAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn(
            "memochat.r18.jm_adapter_algorithms=domain/services/r18/cxx_modules/R18JmAdapter.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18JmAdapter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.jm_adapter_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::jm_adapter::modules", module_interface)
        self.assertIn("SourceId", module_interface)
        self.assertIn("ApiHostAt", module_interface)
        self.assertIn("NormalizeSearchPage", module_interface)
        self.assertIn("IsAllowedImageUrl", module_interface)
        self.assertIn("DefaultImageContentType", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.r18\.jm_adapter_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("jm_adapter::modules::SourceId", consumer)
        self.assertIn("jm_adapter::modules::ApiHostAt", consumer)
        self.assertIn("jm_adapter::modules::NormalizeSearchPage", consumer)
        self.assertIn("jm_adapter::modules::IsAllowedImageUrl", consumer)
        self.assertIn("jm_adapter::modules::DefaultImageContentType", consumer)
        for non_consumer in (adapter_utils, picacg_adapter, source_service, service_source, route_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.jm_adapter_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_jm_adapter_algorithms_gtest", test_cmake)
        self.assertIn("R18JmAdapterAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_jm_adapter_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.jm_adapter_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_picacg_adapter_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18PicacgAdapter.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PicacgAdapter.cpp")
        adapter_utils = read(R18_SERVICE / "domain" / "services" / "r18" / "R18AdapterUtils.cpp")
        jm_adapter = read(R18_SERVICE / "domain" / "services" / "r18" / "R18JmAdapter.cpp")
        source_service = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        public_dtos = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PublicDtos.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        route_schema_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteSchemas.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "R18Service"
            / "R18PicacgAdapterAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn(
            "memochat.r18.picacg_adapter_algorithms=domain/services/r18/cxx_modules/R18PicacgAdapter.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18PicacgAdapter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.picacg_adapter_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::picacg_adapter::modules", module_interface)
        self.assertIn("SourceId", module_interface)
        self.assertIn("ApiHost", module_interface)
        self.assertIn("HmacKey", module_interface)
        self.assertIn("NormalizeSearchPage", module_interface)
        self.assertIn("ShouldStripLeadingSlash", module_interface)
        self.assertIn("ShouldRejectImageScheme", module_interface)
        self.assertIn("DefaultImageContentType", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.picacg_adapter_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("picacg_adapter::modules::ApiKey", consumer)
        self.assertIn("picacg_adapter::modules::HmacKey", consumer)
        self.assertIn("picacg_adapter::modules::NormalizeSearchPage", consumer)
        self.assertIn("picacg_adapter::modules::ShouldStripLeadingSlash", consumer)
        self.assertIn("picacg_adapter::modules::ShouldRejectImageScheme", consumer)
        self.assertIn("picacg_adapter::modules::DefaultImageContentType", consumer)
        for non_consumer in (
            adapter_utils,
            jm_adapter,
            source_service,
            public_dtos,
            service_source,
            route_source,
            route_schema_source,
            entrypoint,
        ):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.picacg_adapter_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_picacg_adapter_algorithms_gtest", test_cmake)
        self.assertIn("R18PicacgAdapterAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_picacg_adapter_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.picacg_adapter_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_service_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18Service.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        adapter_utils = read(R18_SERVICE / "domain" / "services" / "r18" / "R18AdapterUtils.cpp")
        jm_adapter = read(R18_SERVICE / "domain" / "services" / "r18" / "R18JmAdapter.cpp")
        picacg_adapter = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PicacgAdapter.cpp")
        source_service = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        public_dtos = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PublicDtos.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        route_schema_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteSchemas.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "R18ServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Domain", cmake)
        self.assertIn(
            "memochat.r18.service_algorithms=domain/services/r18/cxx_modules/R18Service.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18Service.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.r18\.service_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::r18::service::modules", module_interface)
        self.assertIn("TokenInvalidMessage", module_interface)
        self.assertIn("ShouldRejectImportPayload", module_interface)
        self.assertIn("SuccessHttpStatus", module_interface)
        self.assertIn("UnauthorizedHttpStatus", module_interface)
        self.assertIn("BadGatewayHttpStatus", module_interface)
        self.assertIn("ImageFetchFailedPrefix", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.r18\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("service::modules::TokenInvalidMessage", consumer)
        self.assertIn("service::modules::ShouldRejectImportPayload", consumer)
        self.assertIn("service::modules::SuccessHttpStatus", consumer)
        self.assertIn("service::modules::PlainTextContentType", consumer)
        self.assertIn("service::modules::ImageFetchFailedPrefix", consumer)
        for non_consumer in (
            adapter_utils,
            jm_adapter,
            picacg_adapter,
            source_service,
            public_dtos,
            route_source,
            route_schema_source,
            entrypoint,
        ):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_service_algorithms_gtest", test_cmake)
        self.assertIn("R18ServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_service_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_public_dto_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18PublicDto.cppm")
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18PublicDtos.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn(
            "memochat.r18.public_dto_algorithms=domain/services/r18/cxx_modules/R18PublicDto.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18PublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.public_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::public_dto::modules", module_interface)
        self.assertIn("ShouldUseDefaultSourceId", module_interface)
        self.assertIn("SelectPageOrDefault", module_interface)
        self.assertIn("SelectFavoriteStateOrDefault", module_interface)
        self.assertIn("SelectPageIndexOrDefault", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.r18\.public_dto_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("public_dto::modules::ShouldUseDefaultSourceId", consumer)
        self.assertIn("public_dto::modules::SelectPageOrDefault", consumer)
        self.assertIn("public_dto::modules::SelectFavoriteStateOrDefault", consumer)
        self.assertIn("public_dto::modules::SelectPageIndexOrDefault", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.r18\.public_dto_algorithms\s*;", service_source, flags=re.M)
        )
        self.assertIsNone(re.search(r"^\s*import\s+memochat\.r18\.public_dto_algorithms\s*;", route_source, flags=re.M))
        self.assertIn("r18service_public_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_public_dtos_gtest", test_cmake)

    def test_r18_service_imports_source_record_codec_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(
            R18_SERVICE / "domain" / "services" / "r18" / "cxx_modules" / "R18SourceRecordCodec.cppm"
        )
        consumer = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceRecordCodec.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        source_service = read(R18_SERVICE / "domain" / "services" / "r18" / "R18SourceService.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Core", cmake)
        self.assertIn(
            "memochat.r18.source_record_codec_algorithms=domain/services/r18/cxx_modules/R18SourceRecordCodec.cppm",
            cmake,
        )
        self.assertIn("domain/services/r18/R18SourceRecordCodec.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.source_record_codec_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ShouldUseFallbackString", module_interface)
        self.assertIn("HasDecodeOutput", module_interface)
        self.assertIn("HasRequiredSourceRecordIdentity", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.source_record_codec_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("source_record_codec::modules::ShouldUseFallbackString", consumer)
        self.assertIn("source_record_codec::modules::HasDecodeOutput", consumer)
        self.assertIn("source_record_codec::modules::HasRequiredSourceRecordIdentity", consumer)
        for non_consumer in (service_source, source_service, route_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.source_record_codec_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_source_record_codec_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_source_record_codec_gtest", test_cmake)

    def test_r18_service_imports_route_schema_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(R18_SERVICE / "domain" / "modules" / "r18" / "cxx_modules" / "R18RouteSchema.cppm")
        consumer = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteSchemas.cpp")
        route_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "R18Service"
            / "R18RouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Domain", cmake)
        self.assertIn(
            "memochat.r18.route_schema_algorithms=domain/modules/r18/cxx_modules/R18RouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/r18/R18RouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::route_schema::modules", module_interface)
        self.assertIn("SourceEnableRouteName", module_interface)
        self.assertIn("FavoriteToggleRequestTypeName", module_interface)
        self.assertIn("HistoryUpdateResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SourceEnableRouteName", consumer)
        self.assertIn("modules::FavoriteToggleRequestTypeName", consumer)
        self.assertIn("modules::HistoryUpdateResponseTypeName", consumer)
        for non_consumer in (route_source, service_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.r18\.route_schema_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("r18service_route_schema_gtest", test_cmake)
        self.assertIn("R18RouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.route_schema_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_r18_service_imports_route_registration_algorithms_module(self):
        cmake = read(R18_SERVICE / "CMakeLists.txt")
        module_interface = read(
            R18_SERVICE / "domain" / "modules" / "r18" / "cxx_modules" / "R18RouteRegistration.cppm"
        )
        consumer = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteModule.cpp")
        schema_source = read(R18_SERVICE / "domain" / "modules" / "r18" / "R18RouteSchemas.cpp")
        service_source = read(R18_SERVICE / "domain" / "services" / "r18" / "R18Service.cpp")
        route_modules_source = read(R18_SERVICE / "domain" / "GateR18Routes.cpp")
        entrypoint = read(R18_SERVICE / "app" / "R18GatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "R18Service"
            / "R18RouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "R18Service" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateR18Domain", cmake)
        self.assertIn(
            "memochat.r18.route_registration_algorithms=domain/modules/r18/cxx_modules/R18RouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/r18/R18RouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.r18\.route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::r18::route_registration::modules", module_interface)
        self.assertIn("SourcesPath", module_interface)
        self.assertIn("SourceDeletePath", module_interface)
        self.assertIn("ImagePath", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.r18\.route_registration_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SourcesPath", consumer)
        self.assertIn("modules::SourceDeletePath", consumer)
        self.assertIn("modules::ImagePath", consumer)
        for non_consumer in (schema_source, service_source, route_modules_source, entrypoint):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.r18\.route_registration_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("r18service_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("R18RouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(r18service_route_registration_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.r18\.route_registration_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_call_service_imports_public_dto_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "core" / "support" / "cxx_modules" / "CallPublicDto.cppm")
        consumer = read(CALL_SERVICE / "core" / "support" / "CallPublicDtos.cpp")
        service_source = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        route_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteModule.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallCore", cmake)
        self.assertIn(
            "memochat.call.public_dto_algorithms=core/support/cxx_modules/CallPublicDto.cppm",
            cmake,
        )
        self.assertIn("core/support/CallPublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.public_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::call::public_dto::modules", module_interface)
        self.assertIn("ShouldReadOptionalInt", module_interface)
        self.assertIn("ShouldReadOptionalText", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.call\.public_dto_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("public_dto::modules::ShouldReadOptionalInt", consumer)
        self.assertIn("public_dto::modules::ShouldReadOptionalText", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.call\.public_dto_algorithms\s*;", service_source, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.call\.public_dto_algorithms\s*;", route_source, flags=re.M)
        )
        self.assertIn("callservice_public_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(callservice_public_dtos_gtest", test_cmake)

    def test_call_service_imports_session_cache_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "core" / "support" / "cxx_modules" / "CallSessionCache.cppm")
        consumer = read(CALL_SERVICE / "core" / "support" / "CallSessionCacheDto.cpp")
        public_dto_source = read(CALL_SERVICE / "core" / "support" / "CallPublicDtos.cpp")
        service_source = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        route_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteModule.cpp")
        route_schema_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteSchemas.cpp")
        entrypoint = read(CALL_SERVICE / "app" / "CallGatewayServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallCore", cmake)
        self.assertIn(
            "memochat.call.session_cache_algorithms=core/support/cxx_modules/CallSessionCache.cppm",
            cmake,
        )
        self.assertIn("core/support/CallSessionCacheDto.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.session_cache_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::call::session_cache::modules", module_interface)
        self.assertIn("HasValidCacheIdentity", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.session_cache_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("session_cache::modules::HasValidCacheIdentity", consumer)
        for non_consumer in (public_dto_source, service_source, route_source, route_schema_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.call\.session_cache_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("callservice_session_cache_dto_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(callservice_session_cache_dto_gtest", test_cmake)

    def test_call_service_imports_route_module_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "domain" / "modules" / "call" / "cxx_modules" / "CallRouteModule.cppm")
        consumer = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteModule.cpp")
        route_schema_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteSchemas.cpp")
        entrypoint = read(CALL_SERVICE / "app" / "CallGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "CallService"
            / "CallRouteModuleAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallDomain", cmake)
        self.assertIn(
            "memochat.call.route_module_algorithms=domain/modules/call/cxx_modules/CallRouteModule.cppm",
            cmake,
        )
        self.assertIn("domain/modules/call/CallRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.route_module_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ParseUnsignedDecimalOrZero", module_interface)
        self.assertIn("IsJsonObjectTailForTraceAppend", module_interface)
        self.assertIn("StartPath", module_interface)
        self.assertIn("TokenPath", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.route_module_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("route_modules::ParseUnsignedDecimalOrZero", consumer)
        self.assertIn("route_modules::IsJsonObjectTailForTraceAppend", consumer)
        self.assertIn("modules::StartPath", consumer)
        self.assertIn("modules::TokenPath", consumer)
        for non_consumer in (route_schema_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.call\.route_module_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("callservice_route_module_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(callservice_route_module_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.route_module_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_call_service_imports_route_schema_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "domain" / "modules" / "call" / "cxx_modules" / "CallRouteSchema.cppm")
        consumer = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteSchemas.cpp")
        route_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteModule.cpp")
        service_source = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        entrypoint = read(CALL_SERVICE / "app" / "CallGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "CallService"
            / "CallRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallDomain", cmake)
        self.assertIn(
            "memochat.call.route_schema_algorithms=domain/modules/call/cxx_modules/CallRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/call/CallRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::call::route_schema::modules", module_interface)
        self.assertIn("StartRouteName", module_interface)
        self.assertIn("AuthRequestTypeName", module_interface)
        self.assertIn("TokenResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::StartRouteName", consumer)
        self.assertIn("modules::AuthRequestTypeName", consumer)
        self.assertIn("modules::TokenResponseTypeName", consumer)
        for non_consumer in (route_source, service_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.call\.route_schema_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("callservice_route_schema_gtest", test_cmake)
        self.assertIn("CallRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(callservice_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.route_schema_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_call_service_imports_service_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "core" / "support" / "cxx_modules" / "CallService.cppm")
        consumer = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        public_dto_source = read(CALL_SERVICE / "core" / "support" / "CallPublicDtos.cpp")
        session_cache_source = read(CALL_SERVICE / "core" / "support" / "CallSessionCacheDto.cpp")
        route_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteModule.cpp")
        route_schema_source = read(CALL_SERVICE / "domain" / "modules" / "call" / "CallRouteSchemas.cpp")
        entrypoint = read(CALL_SERVICE / "app" / "CallGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "CallService"
            / "CallServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallCore", cmake)
        self.assertIn(
            "memochat.call.service_algorithms=core/support/cxx_modules/CallService.cppm",
            cmake,
        )
        self.assertIn("core/support/CallService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::call::service::modules", module_interface)
        self.assertIn("IsEnabledText", module_interface)
        self.assertIn("NormalizeRingTimeoutSec", module_interface)
        self.assertIn("HasValidAuthRequest", module_interface)
        self.assertIn("IsSupportedCallType", module_interface)
        self.assertIn("IsActiveCallState", module_interface)
        self.assertIn("CancelTerminalEvent", module_interface)
        self.assertIn("DefaultParticipantRole", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.call\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("service_modules::IsEnabledText", consumer)
        self.assertIn("service_modules::HasValidAuthRequest", consumer)
        self.assertIn("service_modules::HasValidStartPeer", consumer)
        self.assertIn("service_modules::IsSupportedCallType", consumer)
        self.assertIn("service_modules::IsActiveCallState", consumer)
        self.assertIn("service_modules::CancelTerminalEvent", consumer)
        self.assertIn("service_modules::DefaultParticipantRole", consumer)
        for non_consumer in (public_dto_source, session_cache_source, route_source, route_schema_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.call\.service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("callservice_service_algorithms_gtest", test_cmake)
        self.assertIn("CallServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(callservice_service_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.call\.service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_account_shared_imports_auth_login_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "core" / "support" / "cxx_modules" / "AuthLogin.cppm")
        consumer = read(ACCOUNT_SHARED / "core" / "support" / "AuthLoginSupport.cpp")
        rate_limiter = read(ACCOUNT_SHARED / "core" / "support" / "AuthLoginRateLimiter.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountCore", cmake)
        self.assertIn(
            "memochat.account.auth_login_algorithms=core/support/cxx_modules/AuthLogin.cppm",
            cmake,
        )
        self.assertIn("core/support/AuthLoginSupport.cpp", cmake)
        self.assertIn("core/support/AuthLoginRateLimiter.cpp", cmake)
        self.assertIn("${CMAKE_BINARY_DIR}/generated/lua/common", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_login_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("ParseSemVer", module_interface)
        self.assertIn("CompareSemVer", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_login_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::ParseSemVer", consumer)
        self.assertIn("modules::CompareSemVer", consumer)
        self.assertIn('#include "common_lua_scripts.hpp"', rate_limiter)
        self.assertIn("memochat::common::lua_scripts::kincr_with_expire", rate_limiter)
        self.assertIn("accountshared_auth_login_support_gtest", test_cmake)

    def test_account_shared_imports_auth_cache_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "core" / "cache" / "cxx_modules" / "AuthCache.cppm")
        consumer = read(ACCOUNT_SHARED / "core" / "cache" / "AuthCache.cpp")
        login_support = read(ACCOUNT_SHARED / "core" / "support" / "AuthLoginSupport.cpp")
        async_worker = read(ACCOUNT_SHARED / "core" / "async" / "GateAsyncSideEffects.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AccountShared"
            / "AuthCacheAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountCore", cmake)
        self.assertIn(
            "memochat.account.auth_cache_algorithms=core/cache/cxx_modules/AuthCache.cppm",
            cmake,
        )
        self.assertIn("core/cache/AuthCache.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_cache_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::account::auth_cache::modules", module_interface)
        self.assertIn("VerificationCodePrefix", module_interface)
        self.assertIn("HttpTokenPrefix", module_interface)
        self.assertIn("LoginProfilePrefix", module_interface)
        self.assertIn("LoginProfileUidPrefix", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_cache_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("auth_cache_modules::VerificationCodePrefix", consumer)
        self.assertIn("auth_cache_modules::HttpTokenPrefix", consumer)
        self.assertIn("auth_cache_modules::LoginProfilePrefix", consumer)
        self.assertIn("auth_cache_modules::LoginProfileUidPrefix", consumer)
        self.assertNotIn('#include "const.hpp"', consumer)
        for non_consumer in (login_support, async_worker):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.account\.auth_cache_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("accountshared_auth_cache_algorithms_gtest", test_cmake)
        self.assertIn("AuthCacheAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.account.auth_cache_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_cache_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_account_shared_imports_auth_login_cache_profile_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "core" / "support" / "cxx_modules" / "AuthLoginCacheProfile.cppm")
        consumer = read(ACCOUNT_SHARED / "core" / "support" / "AuthLoginCacheProfileDto.cpp")
        login_support = read(ACCOUNT_SHARED / "core" / "support" / "AuthLoginSupport.cpp")
        public_dtos = read(ACCOUNT_SHARED / "core" / "support" / "AuthPublicDtos.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountCore", cmake)
        self.assertIn(
            "memochat.account.auth_login_cache_profile_algorithms=core/support/cxx_modules/AuthLoginCacheProfile.cppm",
            cmake,
        )
        self.assertIn("core/support/AuthLoginCacheProfileDto.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_login_cache_profile_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HasDecodeOutput", module_interface)
        self.assertIn("HasValidCacheProfileIdentity", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.auth_login_cache_profile_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("auth_login_cache_profile::modules::HasDecodeOutput", consumer)
        self.assertIn("auth_login_cache_profile::modules::HasValidCacheProfileIdentity", consumer)
        for non_consumer in (login_support, public_dtos):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.account\.auth_login_cache_profile_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("accountshared_auth_login_cache_profile_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(accountshared_auth_login_cache_profile_gtest", test_cmake)

    def test_account_shared_imports_async_side_effect_dto_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "core" / "async" / "cxx_modules" / "GateAsyncSideEffectDto.cppm")
        consumer = read(ACCOUNT_SHARED / "core" / "async" / "GateAsyncSideEffectDtos.cpp")
        worker_source = read(ACCOUNT_SHARED / "core" / "async" / "GateAsyncSideEffects.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountCore", cmake)
        self.assertIn(
            "memochat.account.async_side_effect_dto_algorithms=core/async/cxx_modules/GateAsyncSideEffectDto.cppm",
            cmake,
        )
        self.assertIn("core/async/GateAsyncSideEffectDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.async_side_effect_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsValidCacheInvalidatePayloadShape", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.async_side_effect_dto_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("async_side_effect::modules::IsValidCacheInvalidatePayloadShape", consumer)
        self.assertIsNone(
            re.search(
                r"^\s*import\s+memochat\.account\.async_side_effect_dto_algorithms\s*;",
                worker_source,
                flags=re.M,
            )
        )
        self.assertIn("accountshared_gate_async_side_effect_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(accountshared_gate_async_side_effect_dtos_gtest", test_cmake)

    def test_account_shared_imports_auth_public_dto_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "core" / "support" / "cxx_modules" / "AuthPublicDto.cppm")
        consumer = read(ACCOUNT_SHARED / "core" / "support" / "AuthPublicDtos.cpp")
        service = read(ACCOUNT_SHARED / "domain" / "services" / "auth" / "AuthService.cpp")
        profile_support = read(ACCOUNT_SHARED / "core" / "support" / "Http2ProfileSupport.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountCore", cmake)
        self.assertIn(
            "memochat.account.auth_public_dto_algorithms=core/support/cxx_modules/AuthPublicDto.cppm",
            cmake,
        )
        self.assertIn("core/support/AuthPublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_public_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HasAuthEmailRequiredShape", module_interface)
        self.assertIn("HasProfileUpdateRequiredShape", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_public_dto_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("auth_public::modules::HasAuthEmailRequiredShape", consumer)
        self.assertIn("auth_public::modules::HasProfileUpdateRequiredShape", consumer)
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_public_dto_algorithms\s*;", service, flags=re.M)
        )
        self.assertIsNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_public_dto_algorithms\s*;", profile_support, flags=re.M)
        )
        self.assertIn("accountshared_auth_public_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(accountshared_auth_public_dtos_gtest", test_cmake)

    def test_account_shared_imports_auth_route_schema_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "cxx_modules" / "AuthRouteSchema.cppm")
        consumer = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteSchemas.cpp")
        route_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteModule.cpp")
        service_source = read(ACCOUNT_SHARED / "domain" / "services" / "auth" / "AuthService.cpp")
        account_persistence = read(ACCOUNT_SHARED / "domain" / "services" / "account" / "AccountPersistence.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AccountShared"
            / "AuthRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAuthDomain", cmake)
        self.assertIn(
            "memochat.account.auth_route_schema_algorithms=domain/modules/auth/cxx_modules/AuthRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/auth/AuthRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::account::auth_route_schema::modules", module_interface)
        self.assertIn("RegisterRouteName", module_interface)
        self.assertIn("ResetPasswordRequestTypeName", module_interface)
        self.assertIn("ResetPasswordResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.auth_route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::RegisterRouteName", consumer)
        self.assertIn("modules::ResetPasswordRequestTypeName", consumer)
        self.assertIn("modules::ResetPasswordResponseTypeName", consumer)
        for non_consumer in (route_source, service_source, account_persistence):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.account\.auth_route_schema_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("accountshared_auth_route_schema_gtest", test_cmake)
        self.assertIn("AuthRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(accountshared_auth_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.auth_route_schema_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_account_shared_imports_auth_route_registration_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(
            ACCOUNT_SHARED / "domain" / "modules" / "auth" / "cxx_modules" / "AuthRouteRegistration.cppm"
        )
        consumer = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteModule.cpp")
        schema_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteSchemas.cpp")
        service_source = read(ACCOUNT_SHARED / "domain" / "services" / "auth" / "AuthService.cpp")
        account_persistence = read(ACCOUNT_SHARED / "domain" / "services" / "account" / "AccountPersistence.cpp")
        login_entrypoint = read(SERVER_CORE / "LoginService" / "app" / "LoginServer.cpp")
        register_entrypoint = read(SERVER_CORE / "RegisterService" / "app" / "RegisterServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AccountShared"
            / "AuthRouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAuthDomain", cmake)
        self.assertIn(
            "memochat.account.auth_route_registration_algorithms=domain/modules/auth/cxx_modules/AuthRouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/auth/AuthRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.auth_route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::account::auth_route_registration::modules", module_interface)
        self.assertIn("GetVarifyCodePath", module_interface)
        self.assertIn("UserRegisterPath", module_interface)
        self.assertIn("UserLoginPath", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.auth_route_registration_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("modules::GetVarifyCodePath", consumer)
        self.assertIn("modules::UserRegisterPath", consumer)
        self.assertIn("modules::UserLoginPath", consumer)
        for non_consumer in (schema_source, service_source, account_persistence, login_entrypoint, register_entrypoint):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.account\.auth_route_registration_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("accountshared_auth_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("AuthRouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn(
            "memochat_enable_gnu_modules(accountshared_auth_route_registration_algorithms_gtest",
            test_cmake,
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.auth_route_registration_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_account_shared_imports_profile_route_schema_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(
            ACCOUNT_SHARED / "domain" / "modules" / "profile" / "cxx_modules" / "ProfileRouteSchema.cppm"
        )
        consumer = read(ACCOUNT_SHARED / "domain" / "modules" / "profile" / "ProfileRouteSchemas.cpp")
        route_source = read(ACCOUNT_SHARED / "domain" / "modules" / "profile" / "ProfileRouteModule.cpp")
        auth_route_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteModule.cpp")
        auth_schema_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteSchemas.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AccountShared"
            / "ProfileRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountDomain", cmake)
        self.assertIn(
            "memochat.account.profile_route_schema_algorithms=domain/modules/profile/cxx_modules/ProfileRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/profile/ProfileRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.profile_route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::account::profile_route_schema::modules", module_interface)
        self.assertIn("UpdateProfileRouteName", module_interface)
        self.assertIn("GetUserInfoRequestTypeName", module_interface)
        self.assertIn("UserInfoResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.account\.profile_route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::UpdateProfileRouteName", consumer)
        self.assertIn("modules::GetUserInfoRequestTypeName", consumer)
        self.assertIn("modules::UserInfoResponseTypeName", consumer)
        for non_consumer in (route_source, auth_route_source, auth_schema_source):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.account\.profile_route_schema_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("accountshared_profile_route_schema_gtest", test_cmake)
        self.assertIn("ProfileRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(accountshared_profile_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.profile_route_schema_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_account_shared_imports_profile_route_registration_algorithms_module(self):
        cmake = read(ACCOUNT_SHARED / "CMakeLists.txt")
        module_interface = read(
            ACCOUNT_SHARED / "domain" / "modules" / "profile" / "cxx_modules" / "ProfileRouteRegistration.cppm"
        )
        consumer = read(ACCOUNT_SHARED / "domain" / "modules" / "profile" / "ProfileRouteModule.cpp")
        schema_source = read(ACCOUNT_SHARED / "domain" / "modules" / "profile" / "ProfileRouteSchemas.cpp")
        auth_route_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteModule.cpp")
        auth_schema_source = read(ACCOUNT_SHARED / "domain" / "modules" / "auth" / "AuthRouteSchemas.cpp")
        account_entrypoint = read(SERVER_CORE / "AccountService" / "app" / "AccountServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AccountShared"
            / "ProfileRouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AccountShared" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateAccountDomain", cmake)
        self.assertIn(
            "memochat.account.profile_route_registration_algorithms=domain/modules/profile/cxx_modules/ProfileRouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/profile/ProfileRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.account\.profile_route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::account::profile_route_registration::modules", module_interface)
        self.assertIn("UserUpdateProfilePath", module_interface)
        self.assertIn("GetUserInfoPath", module_interface)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.profile_route_registration_algorithms\s*;",
                consumer,
                flags=re.M,
            )
        )
        self.assertIn("modules::UserUpdateProfilePath", consumer)
        self.assertIn("modules::GetUserInfoPath", consumer)
        for non_consumer in (schema_source, auth_route_source, auth_schema_source, account_entrypoint):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.account\.profile_route_registration_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("accountshared_profile_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("ProfileRouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn(
            "memochat_enable_gnu_modules(accountshared_profile_route_registration_algorithms_gtest",
            test_cmake,
        )
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.account\.profile_route_registration_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_call_service_imports_token_algorithms_module(self):
        cmake = read(CALL_SERVICE / "CMakeLists.txt")
        module_interface = read(CALL_SERVICE / "core" / "support" / "cxx_modules" / "CallToken.cppm")
        consumer = read(CALL_SERVICE / "core" / "support" / "CallService.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "CallService" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(GateCallCore", cmake)
        self.assertIn(
            "memochat.call.token_algorithms=core/support/cxx_modules/CallToken.cppm",
            cmake,
        )
        self.assertIn("core/support/CallService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.call\.token_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("Base64UrlEncodedSize", module_interface)
        self.assertIn("EncodeBase64Url", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.call\.token_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::Base64UrlEncodedSize", consumer)
        self.assertIn("modules::EncodeBase64Url", consumer)
        self.assertIn("callservice_token_algorithms_gtest", test_cmake)

    def test_moments_service_imports_public_algorithms_module(self):
        cmake = read(MOMENTS_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MOMENTS_SERVICE / "domain" / "services" / "moments" / "cxx_modules" / "MomentsPublic.cppm"
        )
        consumer = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsPublicDtos.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MomentsService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateMomentsDomain", cmake)
        self.assertIn(
            "memochat.moments.public_algorithms=domain/services/moments/cxx_modules/MomentsPublic.cppm",
            cmake,
        )
        self.assertIn("domain/services/moments/MomentsPublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.moments\.public_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("NormalizeMomentItemKind", module_interface)
        self.assertIn("NormalizePageLimit", module_interface)
        self.assertIn("TryParseBoolText", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.moments\.public_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::NormalizeMomentItemKind", consumer)
        self.assertIn("modules::NormalizePageLimit", consumer)
        self.assertIn("modules::TryParseBoolText", consumer)
        self.assertIn("momentsservice_output_dtos_gtest", test_cmake)

    def test_moments_service_imports_output_algorithms_module(self):
        cmake = read(MOMENTS_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MOMENTS_SERVICE / "domain" / "services" / "moments" / "cxx_modules" / "MomentsOutput.cppm"
        )
        consumer = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsOutputDtos.cpp")
        public_dto_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsPublicDtos.cpp")
        service_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsService.cpp")
        route_source = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp")
        entrypoint = read(MOMENTS_SERVICE / "app" / "MomentsGatewayServer.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MomentsService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateMomentsDomain", cmake)
        self.assertIn(
            "memochat.moments.output_algorithms=domain/services/moments/cxx_modules/MomentsOutput.cppm",
            cmake,
        )
        self.assertIn("domain/services/moments/MomentsOutputDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.moments\.output_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::services::moments::output::modules", module_interface)
        self.assertIn("ShouldKeepLikeName", module_interface)
        self.assertIn("ShouldProjectContent", module_interface)
        self.assertIn("ShouldProjectLikes", module_interface)
        self.assertIn("ShouldProjectComments", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.moments\.output_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("output::modules::ShouldKeepLikeName", consumer)
        self.assertIn("output::modules::ShouldProjectContent", consumer)
        self.assertIn("output::modules::ShouldProjectLikes", consumer)
        self.assertIn("output::modules::ShouldProjectComments", consumer)
        for non_consumer in (public_dto_source, service_source, route_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.moments\.output_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("momentsservice_output_dtos_gtest", test_cmake)
        self.assertIn("memochat.moments.output_algorithms", test_cmake)

    def test_moments_service_imports_service_algorithms_module(self):
        cmake = read(MOMENTS_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MOMENTS_SERVICE / "domain" / "services" / "moments" / "cxx_modules" / "MomentsService.cppm"
        )
        consumer = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsService.cpp")
        public_dto_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsPublicDtos.cpp")
        output_dto_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsOutputDtos.cpp")
        route_source = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp")
        route_schema_source = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteSchemas.cpp")
        route_modules_source = read(MOMENTS_SERVICE / "domain" / "MomentsRouteModules.cpp")
        relation_client_source = read(MOMENTS_SERVICE / "clients" / "MomentsRelationClient.cpp")
        entrypoint = read(MOMENTS_SERVICE / "app" / "MomentsGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MomentsService"
            / "MomentsServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MomentsService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateMomentsDomain", cmake)
        self.assertIn(
            "memochat.moments.service_algorithms=domain/services/moments/cxx_modules/MomentsService.cppm",
            cmake,
        )
        self.assertIn("domain/services/moments/MomentsService.cpp", cmake)
        self.assertIsNotNone(
            re.search(r"^\s*export\s+module\s+memochat\.moments\.service_algorithms\s*;", module_interface, flags=re.M)
        )
        self.assertIn("export namespace memochat::moments::service::modules", module_interface)
        self.assertIn("DefaultRelationQueryEndpoint", module_interface)
        self.assertIn("IsFriendsOnlyForeignMoment", module_interface)
        self.assertIn("HasRequiredAuthFields", module_interface)
        self.assertIn("HasValidNewComment", module_interface)
        self.assertIn("CommentLikeFetchLimit", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.moments\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("service::modules::DefaultRelationQueryEndpoint", consumer)
        self.assertIn("service::modules::IsFriendsOnlyForeignMoment", consumer)
        self.assertIn("service::modules::HasRequiredAuthFields", consumer)
        self.assertIn("service::modules::HasValidNewComment", consumer)
        self.assertIn("service::modules::CommentLikeFetchLimit", consumer)
        for non_consumer in (
            public_dto_source,
            output_dto_source,
            route_source,
            route_schema_source,
            route_modules_source,
            relation_client_source,
            entrypoint,
        ):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.moments\.service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("momentsservice_service_algorithms_gtest", test_cmake)
        self.assertIn("MomentsServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(momentsservice_service_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.moments\.service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_moments_service_imports_route_schema_algorithms_module(self):
        cmake = read(MOMENTS_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MOMENTS_SERVICE / "domain" / "modules" / "moments" / "cxx_modules" / "MomentsRouteSchema.cppm"
        )
        consumer = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteSchemas.cpp")
        route_source = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp")
        service_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsService.cpp")
        entrypoint = read(MOMENTS_SERVICE / "app" / "MomentsGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MomentsService"
            / "MomentsRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MomentsService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateMomentsDomain", cmake)
        self.assertIn(
            "memochat.moments.route_schema_algorithms=domain/modules/moments/cxx_modules/MomentsRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/moments/MomentsRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.moments\.route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::moments::route_schema::modules", module_interface)
        self.assertIn("PublishRouteName", module_interface)
        self.assertIn("CommentMutationResponseTypeName", module_interface)
        self.assertIn("CommentLikeResponseTypeName", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.moments\.route_schema_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::PublishRouteName", consumer)
        self.assertIn("modules::CommentMutationResponseTypeName", consumer)
        self.assertIn("modules::CommentLikeResponseTypeName", consumer)
        for non_consumer in (route_source, service_source, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.moments\.route_schema_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("momentsservice_route_schema_gtest", test_cmake)
        self.assertIn("MomentsRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(momentsservice_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.moments\.route_schema_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_moments_service_imports_route_registration_algorithms_module(self):
        cmake = read(MOMENTS_SERVICE / "CMakeLists.txt")
        module_interface = read(
            MOMENTS_SERVICE / "domain" / "modules" / "moments" / "cxx_modules" / "MomentsRouteRegistration.cppm"
        )
        consumer = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteModule.cpp")
        schema_source = read(MOMENTS_SERVICE / "domain" / "modules" / "moments" / "MomentsRouteSchemas.cpp")
        service_source = read(MOMENTS_SERVICE / "domain" / "services" / "moments" / "MomentsService.cpp")
        route_modules_source = read(MOMENTS_SERVICE / "domain" / "MomentsRouteModules.cpp")
        entrypoint = read(MOMENTS_SERVICE / "app" / "MomentsGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "MomentsService"
            / "MomentsRouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "MomentsService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateMomentsDomain", cmake)
        self.assertIn(
            "memochat.moments.route_registration_algorithms=domain/modules/moments/cxx_modules/MomentsRouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/moments/MomentsRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.moments\.route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::moments::route_registration::modules", module_interface)
        self.assertIn("PublishPath", module_interface)
        self.assertIn("CommentListPath", module_interface)
        self.assertIn("CommentLikePath", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.moments\.route_registration_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::PublishPath", consumer)
        self.assertIn("modules::CommentListPath", consumer)
        self.assertIn("modules::CommentLikePath", consumer)
        for non_consumer in (schema_source, service_source, route_modules_source, entrypoint):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.moments\.route_registration_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("momentsservice_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("MomentsRouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(momentsservice_route_registration_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.moments\.route_registration_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_ai_gateway_imports_gateway_runtime_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "app" / "cxx_modules" / "AIGatewayRuntime.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "app" / "AIGatewayRuntime.cpp")
        entrypoint = read(AI_GATEWAY_SERVICE / "app" / "AIGatewayServer.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(AIGatewayServer", cmake)
        self.assertIn(
            "memochat.ai.gateway_runtime_algorithms=app/cxx_modules/AIGatewayRuntime.cppm",
            cmake,
        )
        self.assertIn("app/AIGatewayRuntime.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.gateway_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("SelectListenPort", module_interface)
        self.assertIn("NormalizeIoThreadCount", module_interface)
        self.assertIn("SelectWorkerThreadCount", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.gateway_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::SelectListenPort", consumer)
        self.assertIn("modules::NormalizeIoThreadCount", consumer)
        self.assertIn("modules::SelectWorkerThreadCount", consumer)
        self.assertNotRegex(entrypoint, r"^\s*import\s+memochat\.ai\.gateway_runtime_algorithms\s*;")
        self.assertIn("SelectAIGatewayListenPort", entrypoint)
        self.assertIn("NormalizeAIGatewayIoThreads", entrypoint)
        self.assertIn("SelectAIGatewayWorkerThreads", entrypoint)
        self.assertIn("aigateway_runtime_algorithms_gtest", test_cmake)

    def test_ai_gateway_imports_public_dto_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "cxx_modules" / "AIPublicDto.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIPublicDtos.cpp")
        service = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIService.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateAiDomain", cmake)
        self.assertIn(
            "memochat.ai.public_dto_algorithms=domain/services/ai/cxx_modules/AIPublicDto.cppm",
            cmake,
        )
        self.assertIn("domain/services/ai/AIPublicDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.public_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::services::ai::modules", module_interface)
        self.assertIn("ShouldParseDynamicJson", module_interface)
        self.assertIn("ShouldKeepOptionalResponseObject", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.public_dto_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::ShouldParseDynamicJson", consumer)
        self.assertIn("modules::ShouldKeepOptionalResponseObject", consumer)
        self.assertIsNone(re.search(r"^\s*import\s+memochat\.ai\.public_dto_algorithms\s*;", service, flags=re.M))
        self.assertIn("aigateway_public_dtos_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aigateway_public_dtos_gtest", test_cmake)

    def test_ai_gateway_imports_route_module_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "cxx_modules" / "AIRouteModule.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "domain" / "AIRouteModules.cpp")
        service = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIService.cpp")
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateAiDomain", cmake)
        self.assertIn(
            "memochat.ai.route_module_algorithms=domain/modules/cxx_modules/AIRouteModule.cppm",
            cmake,
        )
        self.assertIn("domain/AIRouteModules.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.route_module_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::modules::ai::route_algorithms", module_interface)
        self.assertIn("AtLeast", module_interface)
        self.assertIn("StartsWith", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.route_module_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("route_algorithms::AtLeast", consumer)
        self.assertIn("route_algorithms::StartsWith", consumer)
        self.assertIsNone(re.search(r"^\s*import\s+memochat\.ai\.route_module_algorithms\s*;", service, flags=re.M))
        self.assertIn("aigateway_route_module_algorithms_gtest", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aigateway_route_module_algorithms_gtest", test_cmake)

    def test_ai_gateway_imports_route_registration_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "cxx_modules" / "AIRouteRegistration.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteModule.cpp")
        schema_source = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteSchemas.cpp")
        proxy_source = read(AI_GATEWAY_SERVICE / "domain" / "AIRouteModules.cpp")
        service = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIService.cpp")
        client = read(AI_GATEWAY_SERVICE / "domain" / "AIServiceClient.cpp")
        entrypoint = read(AI_GATEWAY_SERVICE / "app" / "AIGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIGatewayService"
            / "AIRouteRegistrationAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateAiDomain", cmake)
        self.assertIn(
            "memochat.ai.route_registration_algorithms=domain/modules/cxx_modules/AIRouteRegistration.cppm",
            cmake,
        )
        self.assertIn("domain/modules/ai/AIRouteModule.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.route_registration_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::route_registration::modules", module_interface)
        self.assertIn("ChatPath", module_interface)
        self.assertIn("ModelApiRegisterPath", module_interface)
        self.assertIn("TaskResumePath", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.route_registration_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::ChatPath", consumer)
        self.assertIn("modules::ModelApiRegisterPath", consumer)
        self.assertIn("modules::TaskResumePath", consumer)
        for non_consumer in (schema_source, proxy_source, service, client, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.ai\.route_registration_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("aigateway_route_registration_algorithms_gtest", test_cmake)
        self.assertIn("AIRouteRegistrationAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aigateway_route_registration_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.route_registration_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_ai_gateway_imports_gateway_service_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "cxx_modules" / "AIService.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIService.cpp")
        public_dtos = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIPublicDtos.cpp")
        route_module = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteModule.cpp")
        route_schema = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteSchemas.cpp")
        proxy_source = read(AI_GATEWAY_SERVICE / "domain" / "AIRouteModules.cpp")
        client = read(AI_GATEWAY_SERVICE / "domain" / "AIServiceClient.cpp")
        entrypoint = read(AI_GATEWAY_SERVICE / "app" / "AIGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIGatewayService"
            / "AIServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateAiDomain", cmake)
        self.assertIn(
            "memochat.ai.gateway_service_algorithms=domain/services/ai/cxx_modules/AIService.cppm",
            cmake,
        )
        self.assertIn("domain/services/ai/AIService.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.gateway_service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::gate::services::ai::service_modules", module_interface)
        self.assertIn("SuccessStatusCode", module_interface)
        self.assertIn("InvalidJsonMessage", module_interface)
        self.assertIn("DefaultTaskListLimit", module_interface)
        self.assertIn("ShouldRejectEmptyContent", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.gateway_service_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("service_modules::SuccessStatusCode", consumer)
        self.assertIn("service_modules::InvalidJsonErrorCode", consumer)
        self.assertIn("service_modules::DefaultHistoryLimit", consumer)
        self.assertIn("service_modules::DefaultModelType", consumer)
        self.assertIn("service_modules::DefaultTaskListLimit", consumer)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.gateway_service_algorithms\s*;", proxy_source, flags=re.M)
        )
        self.assertIn("service_modules::TokenInvalidErrorCode", proxy_source)
        self.assertIn("service_modules::TokenInvalidMessage", proxy_source)
        self.assertIn("service_modules::UnauthorizedStatusCode", proxy_source)
        self.assertIn("service_modules::ShouldRejectUserAuth", proxy_source)
        for non_consumer in (public_dtos, route_module, route_schema, client, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.ai\.gateway_service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("aigateway_service_algorithms_gtest", test_cmake)
        self.assertIn("AIServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aigateway_service_algorithms_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.gateway_service_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_ai_gateway_imports_route_schema_algorithms_module(self):
        cmake = read(AI_GATEWAY_SERVICE / "CMakeLists.txt")
        module_interface = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "cxx_modules" / "AIRouteSchema.cppm")
        consumer = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteSchemas.cpp")
        route_source = read(AI_GATEWAY_SERVICE / "domain" / "modules" / "ai" / "AIRouteModule.cpp")
        service = read(AI_GATEWAY_SERVICE / "domain" / "services" / "ai" / "AIService.cpp")
        client = read(AI_GATEWAY_SERVICE / "domain" / "AIServiceClient.cpp")
        entrypoint = read(AI_GATEWAY_SERVICE / "app" / "AIGatewayServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIGatewayService"
            / "AIRouteSchemaAlgorithmsConsumer.cpp"
        )
        test_cmake = read(
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIGatewayService" / "CMakeLists.txt"
        )

        self.assertIn("memochat_enable_gnu_modules(GateAiDomain", cmake)
        self.assertIn(
            "memochat.ai.route_schema_algorithms=domain/modules/cxx_modules/AIRouteSchema.cppm",
            cmake,
        )
        self.assertIn("domain/modules/ai/AIRouteSchemas.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.route_schema_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::route_schema::modules", module_interface)
        self.assertIn("RegisterApiProviderPath", module_interface)
        self.assertIn("DeleteApiProviderRouteName", module_interface)
        self.assertIn("SimpleResponseTypeName", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.route_schema_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::RegisterApiProviderPath", consumer)
        self.assertIn("modules::DeleteApiProviderRouteName", consumer)
        self.assertIn("modules::SimpleResponseTypeName", consumer)
        for non_consumer in (route_source, service, client, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.ai\.route_schema_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("aigateway_route_schema_gtest", test_cmake)
        self.assertIn("AIRouteSchemaAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aigateway_route_schema_gtest", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.route_schema_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_ai_server_imports_service_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIService.cppm")
        consumer = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        client = read(AI_SERVER / "AIServiceClient.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.service_algorithms=cxx_modules/AIService.cppm",
            cmake,
        )
        self.assertIn("AIServiceAlgorithms.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("IsSuccessfulModelListPayload", module_interface)
        self.assertIn("NormalizeAgentTaskListLimit", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("modules::IsSuccessfulModelListPayload", consumer)
        self.assertIn("modules::NormalizeAgentTaskListLimit", consumer)
        self.assertIn("ai_service_algorithms::IsSuccessfulModelListPayload", mapper)
        self.assertIn("ai_service_algorithms::NormalizeAgentTaskListLimit", client)
        self.assertIn("AIServiceAlgorithmsTest.cpp", test_cmake)
        self.assertIn("memochat_enable_gnu_modules(aiserver_gtest_json_mapper_test", test_cmake)

    def test_ai_server_imports_json_dto_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIServiceJsonDto.cppm")
        consumer = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        service_algorithms = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        client = read(AI_SERVER / "AIServiceClient.cpp")
        core = read(AI_SERVER / "AIServiceCore.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.json_dto_algorithms=cxx_modules/AIServiceJsonDto.cppm",
            cmake,
        )
        self.assertIn("AIServiceJsonDtos.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.json_dto_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::json_dto::modules", module_interface)
        self.assertIn("ShouldInspectObjectMembers", module_interface)
        self.assertIn("ShouldReadArrayMember", module_interface)
        self.assertIn("ShouldReadObjectMember", module_interface)
        self.assertIn("ShouldFallbackToFirstModel", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.json_dto_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("json_dto::modules::ShouldInspectObjectMembers", consumer)
        self.assertIn("json_dto::modules::ShouldReadArrayMember", consumer)
        self.assertIn("json_dto::modules::ShouldReadObjectMember", consumer)
        self.assertIn("json_dto::modules::ShouldFallbackToFirstModel", consumer)
        for non_consumer in (service_algorithms, mapper, client, core, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.ai\.json_dto_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("AIServiceJsonMapperTest.cpp", test_cmake)
        self.assertIn("memochat.ai.json_dto_algorithms", test_cmake)

    def test_ai_server_imports_conversation_context_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "ConversationContext.cppm")
        consumer = read(AI_SERVER / "ConversationContext.cpp")
        json_dtos = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        client = read(AI_SERVER / "AIServiceClient.cpp")
        core = read(AI_SERVER / "AIServiceCore.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.conversation_context_algorithms=cxx_modules/ConversationContext.cppm",
            cmake,
        )
        self.assertIn("ConversationContext.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.conversation_context_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("UserRole", module_interface)
        self.assertIn("AssistantRole", module_interface)
        self.assertIn("ShouldPruneMessages", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.conversation_context_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("conversation_context::modules::UserRole", consumer)
        self.assertIn("conversation_context::modules::AssistantRole", consumer)
        self.assertIn("conversation_context::modules::ShouldPruneMessages", consumer)
        for non_consumer in (json_dtos, mapper, client, core, entrypoint):
            self.assertIsNone(
                re.search(
                    r"^\s*import\s+memochat\.ai\.conversation_context_algorithms\s*;",
                    non_consumer,
                    flags=re.M,
                )
            )
        self.assertIn("AIServiceJsonMapperTest.cpp", test_cmake)
        self.assertIn("memochat.ai.conversation_context_algorithms", test_cmake)

    def test_ai_server_imports_client_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIServiceClient.cppm")
        consumer = read(AI_SERVER / "AIServiceClient.cpp")
        service_algorithms = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        json_dtos = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        core = read(AI_SERVER / "AIServiceCore.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIServer"
            / "AIServiceClientAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.client_algorithms=cxx_modules/AIServiceClient.cppm",
            cmake,
        )
        self.assertIn("AIServiceClient.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.client_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::client::modules", module_interface)
        self.assertIn("ShouldKeepUrlByte", module_interface)
        self.assertIn("IsSseDataLine", module_interface)
        self.assertIn("DefaultAgentTaskLimit", module_interface)
        self.assertIn("AgentMemoryPath", module_interface)
        self.assertIn("ResumeSuffix", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.client_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("client_modules::ShouldKeepUrlByte", consumer)
        self.assertIn("client_modules::IsSseDataLine", consumer)
        self.assertIn("client_modules::DefaultAgentTaskLimit", consumer)
        self.assertIn("client_modules::AgentMemoryPath", consumer)
        self.assertIn("client_modules::ResumeSuffix", consumer)
        for non_consumer in (service_algorithms, json_dtos, mapper, core, entrypoint):
            self.assertIsNone(re.search(r"^\s*import\s+memochat\.ai\.client_algorithms\s*;", non_consumer, flags=re.M))
        self.assertIn("aiserver_client_algorithms_gtest", test_cmake)
        self.assertIn("AIServiceClientAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.ai.client_algorithms", test_cmake)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.client_algorithms\s*;", test_consumer, flags=re.M))

    def test_ai_server_imports_impl_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIServiceImpl.cppm")
        consumer = read(AI_SERVER / "AIServiceImpl.cpp")
        service_algorithms = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        client = read(AI_SERVER / "AIServiceClient.cpp")
        json_dtos = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        core = read(AI_SERVER / "AIServiceCore.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIServer"
            / "AIServiceImplAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.impl_algorithms=cxx_modules/AIServiceImpl.cppm",
            cmake,
        )
        self.assertIn("AIServiceImpl.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.impl_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::impl::modules", module_interface)
        self.assertIn("RpcKind", module_interface)
        self.assertIn("ChatSpan", module_interface)
        self.assertIn("RegisterApiProviderSpan", module_interface)
        self.assertIn("AgentTaskResumeSpan", module_interface)
        self.assertIn("ConfirmSpan", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.impl_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("impl_modules::RpcKind", consumer)
        self.assertIn("impl_modules::ChatSpan", consumer)
        self.assertIn("impl_modules::RegisterApiProviderSpan", consumer)
        self.assertIn("impl_modules::AgentTaskResumeSpan", consumer)
        self.assertIn("impl_modules::ConfirmSpan", consumer)
        for non_consumer in (service_algorithms, client, json_dtos, mapper, core, entrypoint):
            self.assertIsNone(re.search(r"^\s*import\s+memochat\.ai\.impl_algorithms\s*;", non_consumer, flags=re.M))
        self.assertIn("aiserver_impl_algorithms_gtest", test_cmake)
        self.assertIn("AIServiceImplAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.ai.impl_algorithms", test_cmake)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.impl_algorithms\s*;", test_consumer, flags=re.M))

    def test_ai_server_imports_core_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIServiceCore.cppm")
        consumer = read(AI_SERVER / "AIServiceCore.cpp")
        service_algorithms = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        json_dtos = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        client = read(AI_SERVER / "AIServiceClient.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIServer"
            / "AIServiceCoreAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.core_algorithms=cxx_modules/AIServiceCore.cppm",
            cmake,
        )
        self.assertIn("AIServiceCore.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.core_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::core::modules", module_interface)
        self.assertIn("SelectHistoryLimit", module_interface)
        self.assertIn("ShouldCreateSessionFromRequest", module_interface)
        self.assertIn("ShouldRejectSessionUpdate", module_interface)
        self.assertIn("DefaultApiProviderAdapter", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.core_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("core_modules::IsAsciiTrimChar", consumer)
        self.assertIn("core_modules::SelectHistoryLimit", consumer)
        self.assertIn("core_modules::ShouldRejectSessionUpdate", consumer)
        self.assertIn("core_modules::DefaultApiProviderAdapter", consumer)
        for non_consumer in (service_algorithms, json_dtos, mapper, client, entrypoint):
            self.assertIsNone(re.search(r"^\s*import\s+memochat\.ai\.core_algorithms\s*;", non_consumer, flags=re.M))
        self.assertIn("aiserver_core_algorithms_gtest", test_cmake)
        self.assertIn("AIServiceCoreAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.ai.core_algorithms", test_cmake)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.ai\.core_algorithms\s*;", test_consumer, flags=re.M))

    def test_ai_server_imports_repository_algorithms_module(self):
        cmake = read(AI_SERVER / "CMakeLists.txt")
        module_interface = read(AI_SERVER / "cxx_modules" / "AIServerRepository.cppm")
        session_repo = read(AI_SERVER / "db" / "AISessionRepo.cpp")
        smart_log_repo = read(AI_SERVER / "db" / "AISmartLogRepo.cpp")
        service_algorithms = read(AI_SERVER / "AIServiceAlgorithms.cpp")
        json_dtos = read(AI_SERVER / "AIServiceJsonDtos.cpp")
        mapper = read(AI_SERVER / "AIServiceJsonMapper.cpp")
        core = read(AI_SERVER / "AIServiceCore.cpp")
        entrypoint = read(AI_SERVER / "AIServer.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "AIServer"
            / "AIServerRepositoryAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "AIServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(AIServer", cmake)
        self.assertIn(
            "memochat.ai.repository_algorithms=cxx_modules/AIServerRepository.cppm",
            cmake,
        )
        self.assertIn("db/AISessionRepo.cpp", cmake)
        self.assertIn("db/AISmartLogRepo.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.ai\.repository_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("export namespace memochat::ai::repository::modules", module_interface)
        self.assertIn("DefaultPostgresSchema", module_interface)
        self.assertIn("ShouldUseDefaultSchema", module_interface)
        self.assertIn("DefaultSessionTitle", module_interface)
        self.assertIn("HasAffectedRows", module_interface)
        self.assertIn("ShouldReturnNoSession", module_interface)
        for consumer in (session_repo, smart_log_repo):
            self.assertIsNotNone(
                re.search(r"^\s*import\s+memochat\.ai\.repository_algorithms\s*;", consumer, flags=re.M)
            )
            self.assertIn("repository_modules::DefaultPostgresSchema", consumer)
            self.assertIn("repository_modules::ShouldUseDefaultSchema", consumer)
        self.assertIn("repository_modules::DefaultSessionTitle", session_repo)
        self.assertIn("repository_modules::HasAffectedRows", session_repo)
        self.assertIn("repository_modules::ShouldReturnNoSession", session_repo)
        for non_consumer in (service_algorithms, json_dtos, mapper, core, entrypoint):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.ai\.repository_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("aiserver_repository_algorithms_gtest", test_cmake)
        self.assertIn("AIServerRepositoryAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.ai.repository_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.ai\.repository_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_varify_server_imports_verify_code_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "VerifyCode.cppm")
        consumer = read(VARIFY_SERVER / "VerifyCodePolicy.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServerCore", cmake)
        self.assertIn(
            "memochat.varify.verify_code_algorithms=cxx_modules/VerifyCode.cppm",
            cmake,
        )
        self.assertIn("VerifyCodePolicy.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.verify_code_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("EndsWithAsciiCaseInsensitive", module_interface)
        self.assertIn("NormalizeVerifyCodeLength", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.verify_code_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("modules::EndsWithAsciiCaseInsensitive", consumer)
        self.assertIn("modules::NormalizeVerifyCodeLength", consumer)
        self.assertIn("varifyserver_gtest", test_cmake)

    def test_varify_server_imports_email_delivery_task_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "EmailDeliveryTask.cppm")
        consumer = read(VARIFY_SERVER / "EmailDeliveryTaskCodec.cpp")
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServerCore", cmake)
        self.assertIn(
            "memochat.varify.email_delivery_task_algorithms=cxx_modules/EmailDeliveryTask.cppm",
            cmake,
        )
        self.assertIn("EmailDeliveryTaskCodec.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.email_delivery_task_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("HasParseOutput", module_interface)
        self.assertIn("HasRequiredEmailTaskFields", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.email_delivery_task_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("email_delivery::modules::HasParseOutput", consumer)
        self.assertIn("email_delivery::modules::HasRequiredEmailTaskFields", consumer)
        self.assertIn("varifyserver_gtest", test_cmake)
        self.assertIn("memochat.varify.email_delivery_task_algorithms", test_cmake)

    def test_varify_server_imports_server_runtime_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "VarifyServerRuntime.cppm")
        consumer = read(VARIFY_SERVER / "VarifyServer.cpp")
        service_impl = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        redis_mgr = read(VARIFY_SERVER / "VarifyRedisMgr.cpp")
        email_task_bus = read(VARIFY_SERVER / "EmailTaskBus.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "VarifyServerRuntimeAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.server_runtime_algorithms=cxx_modules/VarifyServerRuntime.cppm",
            cmake,
        )
        self.assertIn("VarifyServer.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.server_runtime_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultHealthPort", module_interface)
        self.assertIn("IsHealthPath", module_interface)
        self.assertIn("IsReadinessPath", module_interface)
        self.assertIn("ShouldReportRedisDown", module_interface)
        self.assertIn('return "/healthz"', module_interface)
        self.assertIn('return R"({"status":"ok","service":"VarifyServer"})"', module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.server_runtime_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("varify_runtime_modules::IsHealthPath", consumer)
        self.assertIn("varify_runtime_modules::ShouldReportRedisDown", consumer)
        self.assertNotIn('target == "/healthz"', consumer)
        self.assertNotIn('res.body() = R"({"status":"ok","service":"VarifyServer"})"', consumer)
        for non_consumer in (service_impl, redis_mgr, email_task_bus):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.server_runtime_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_runtime_algorithms_gtest", test_cmake)
        self.assertIn("VarifyServerRuntimeAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.server_runtime_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*import\s+memochat\.varify\.server_runtime_algorithms\s*;",
                test_consumer,
                flags=re.M,
            )
        )

    def test_varify_server_imports_rate_limiter_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "RateLimiter.cppm")
        consumer = read(VARIFY_SERVER / "RateLimiter.cpp")
        service_impl = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        redis_mgr = read(VARIFY_SERVER / "VarifyRedisMgr.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "RateLimiterAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.rate_limiter_algorithms=cxx_modules/RateLimiter.cppm",
            cmake,
        )
        self.assertIn("RateLimiter.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.rate_limiter_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("EmailRateLimitPrefix", module_interface)
        self.assertIn("IpRateLimitPrefix", module_interface)
        self.assertIn("IsRedisCounterError", module_interface)
        self.assertIn("ShouldRateLimit", module_interface)
        self.assertIn('return "varify_rl_email:"', module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.rate_limiter_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("rate_limiter_modules::EmailRateLimitPrefix", consumer)
        self.assertIn("rate_limiter_modules::ShouldRateLimit", consumer)
        self.assertNotIn('"varify_rl_email:" + email', consumer)
        self.assertNotIn("count > max_requests", consumer)
        for non_consumer in (service_impl, redis_mgr):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.rate_limiter_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_rate_limiter_algorithms_gtest", test_cmake)
        self.assertIn("RateLimiterAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.rate_limiter_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.rate_limiter_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_varify_server_imports_email_task_bus_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "EmailTaskBus.cppm")
        consumer = read(VARIFY_SERVER / "EmailTaskBus.cpp")
        service_impl = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        redis_mgr = read(VARIFY_SERVER / "VarifyRedisMgr.cpp")
        rate_limiter = read(VARIFY_SERVER / "RateLimiter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "EmailTaskBusAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.email_task_bus_algorithms=cxx_modules/EmailTaskBus.cppm",
            cmake,
        )
        self.assertIn("EmailTaskBus.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.email_task_bus_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultDirectExchange", module_interface)
        self.assertIn("DefaultVerifyDeliveryQueue", module_interface)
        self.assertIn("NormalizeRetryDelayMs", module_interface)
        self.assertIn("ShouldRetryTask", module_interface)
        self.assertIn('return "memochat.direct"', module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.email_task_bus_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("email_task_bus_modules::DefaultDirectExchange", consumer)
        self.assertIn("email_task_bus_modules::NormalizeRetryDelayMs", consumer)
        self.assertIn("email_task_bus_modules::ShouldRetryTask", consumer)
        self.assertNotIn('config_exchange_direct_ = "memochat.direct"', consumer)
        self.assertNotIn("task.retry_count < config_max_retries_", consumer)
        for non_consumer in (service_impl, redis_mgr, rate_limiter):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.email_task_bus_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_email_task_bus_algorithms_gtest", test_cmake)
        self.assertIn("EmailTaskBusAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.email_task_bus_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.email_task_bus_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_varify_server_imports_email_sender_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "EmailSender.cppm")
        consumer = read(VARIFY_SERVER / "EmailSender.cpp")
        service_impl = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        email_task_bus = read(VARIFY_SERVER / "EmailTaskBus.cpp")
        rate_limiter = read(VARIFY_SERVER / "RateLimiter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "EmailSenderAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.email_sender_algorithms=cxx_modules/EmailSender.cppm",
            cmake,
        )
        self.assertIn("EmailSender.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.email_sender_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultSmtpPort", module_interface)
        self.assertIn("ShouldUseImplicitSsl", module_interface)
        self.assertIn("HasStatusCodePrefix", module_interface)
        self.assertIn("IsExpectedStatusCode", module_interface)
        self.assertIn("return 465", module_interface)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.email_sender_algorithms\s*;", consumer, flags=re.M)
        )
        self.assertIn("email_sender_modules::DefaultSmtpPort", consumer)
        self.assertIn("email_sender_modules::ShouldUseImplicitSsl", consumer)
        self.assertIn("email_sender_modules::IsExpectedStatusCode", consumer)
        self.assertNotIn("int port = 465;", consumer)
        self.assertNotIn("bool use_ssl = (port == 465);", consumer)
        for non_consumer in (service_impl, email_task_bus, rate_limiter):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.email_sender_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_email_sender_algorithms_gtest", test_cmake)
        self.assertIn("EmailSenderAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.email_sender_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.email_sender_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_varify_server_imports_redis_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "VarifyRedis.cppm")
        consumer = read(VARIFY_SERVER / "VarifyRedisMgr.cpp")
        service_impl = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        email_sender = read(VARIFY_SERVER / "EmailSender.cpp")
        email_task_bus = read(VARIFY_SERVER / "EmailTaskBus.cpp")
        rate_limiter = read(VARIFY_SERVER / "RateLimiter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "VarifyRedisAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.redis_algorithms=cxx_modules/VarifyRedis.cppm",
            cmake,
        )
        self.assertIn("VarifyRedisMgr.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.redis_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("DefaultRedisPort", module_interface)
        self.assertIn("DefaultConnectionPoolSize", module_interface)
        self.assertIn("ShouldDropPoolConnection", module_interface)
        self.assertIn("RedisCounterError", module_interface)
        self.assertIn("RedisMissingTtl", module_interface)
        self.assertIn("return 6379", module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.varify\.redis_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("redis_modules::DefaultRedisPort", consumer)
        self.assertIn("redis_modules::DefaultConnectionPoolSize", consumer)
        self.assertIn("redis_modules::ShouldDropPoolConnection", consumer)
        self.assertIn("redis_modules::RedisCounterError", consumer)
        self.assertNotIn("int port = 6379;", consumer)
        self.assertNotIn("pool_.reset(new VarifyRedisConPool(5", consumer)
        for non_consumer in (service_impl, email_sender, email_task_bus, rate_limiter):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.redis_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_redis_algorithms_gtest", test_cmake)
        self.assertIn("VarifyRedisAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.redis_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.redis_algorithms\s*;", test_consumer, flags=re.M)
        )

    def test_varify_server_imports_service_algorithms_module(self):
        cmake = read(VARIFY_SERVER / "CMakeLists.txt")
        module_interface = read(VARIFY_SERVER / "cxx_modules" / "VarifyService.cppm")
        consumer = read(VARIFY_SERVER / "VarifyServiceImpl.cpp")
        redis_mgr = read(VARIFY_SERVER / "VarifyRedisMgr.cpp")
        email_sender = read(VARIFY_SERVER / "EmailSender.cpp")
        email_task_bus = read(VARIFY_SERVER / "EmailTaskBus.cpp")
        rate_limiter = read(VARIFY_SERVER / "RateLimiter.cpp")
        test_consumer = read(
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "VarifyServer"
            / "VarifyServiceAlgorithmsConsumer.cpp"
        )
        test_cmake = read(REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "VarifyServer" / "CMakeLists.txt")

        self.assertIn("memochat_enable_gnu_modules(VarifyServer", cmake)
        self.assertIn(
            "memochat.varify.service_algorithms=cxx_modules/VarifyService.cppm",
            cmake,
        )
        self.assertIn("VarifyServiceImpl.cpp", cmake)
        self.assertIsNotNone(
            re.search(
                r"^\s*export\s+module\s+memochat\.varify\.service_algorithms\s*;",
                module_interface,
                flags=re.M,
            )
        )
        self.assertIn("Ipv4PeerPrefix", module_interface)
        self.assertIn("Ipv6PeerPrefix", module_interface)
        self.assertIn("PeerAddressPrefixLength", module_interface)
        self.assertIn("GrpcLogModuleName", module_interface)
        self.assertIn("ShouldSendEmailForSyntheticAccount", module_interface)
        self.assertIn('return "ipv4:"', module_interface)
        self.assertIn('return "grpc"', module_interface)
        self.assertIsNotNone(re.search(r"^\s*import\s+memochat\.varify\.service_algorithms\s*;", consumer, flags=re.M))
        self.assertIn("varify_service_modules::Ipv4PeerPrefix", consumer)
        self.assertIn("varify_service_modules::PeerAddressPrefixLength", consumer)
        self.assertIn("varify_service_modules::GrpcLogModuleName", consumer)
        self.assertIn("varify_service_modules::ShouldSendEmailForSyntheticAccount", consumer)
        self.assertNotIn('peer.find("ipv4:") == 0', consumer)
        self.assertNotIn('{"module", "grpc"}', consumer)
        for non_consumer in (redis_mgr, email_sender, email_task_bus, rate_limiter):
            self.assertIsNone(
                re.search(r"^\s*import\s+memochat\.varify\.service_algorithms\s*;", non_consumer, flags=re.M)
            )
        self.assertIn("varifyserver_service_algorithms_gtest", test_cmake)
        self.assertIn("VarifyServiceAlgorithmsConsumer.cpp", test_cmake)
        self.assertIn("memochat.varify.service_algorithms", test_cmake)
        self.assertIsNotNone(
            re.search(r"^\s*import\s+memochat\.varify\.service_algorithms\s*;", test_consumer, flags=re.M)
        )


if __name__ == "__main__":
    unittest.main()
