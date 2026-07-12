# R18Service owns the r18 domain code (relocated from GateShared in slice j).
# GateR18Core (the R18 source service implementation) and GateR18Domain (r18
# route module + service facade); target names unchanged.
#
# NOTE: the R18 source service implementation lives under
# domain/services/r18 (relocated out of common/r18 in slice d, finding ③).
# GateR18Core exports domain/services as a PUBLIC include root so the
# `#include "r18/R18SourceService.hpp"` spelling used by R18Service.cpp,
# R18GatewayServer and the legacy H1 transport keeps resolving.

# --- GateR18Core: R18 source service implementation. --------------------------
add_library(GateR18Core STATIC
    domain/services/r18/R18PublicDtos.cpp
    domain/services/r18/R18PublicDtos.hpp
    domain/services/r18/R18SourceRecordCodec.cpp
    domain/services/r18/R18SourceRecordCodec.hpp
    domain/services/r18/R18AdapterUtils.cpp
    domain/services/r18/R18AdapterUtils.hpp
    domain/services/r18/R18JmAdapter.cpp
    domain/services/r18/R18JmAdapter.hpp
    domain/services/r18/R18PicacgAdapter.cpp
    domain/services/r18/R18PicacgAdapter.hpp
    domain/services/r18/R18SourceService.cpp
    domain/services/r18/R18SourceService.hpp
)
memochat_configure_gate_core_lib(GateR18Core)
target_include_directories(GateR18Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/services)
target_link_libraries(GateR18Core PUBLIC GateInfraCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateR18Core
        MODULES
            memochat.r18.adapter_utils_algorithms=domain/services/r18/cxx_modules/R18AdapterUtils.cppm
            memochat.r18.jm_adapter_algorithms=domain/services/r18/cxx_modules/R18JmAdapter.cppm
            memochat.r18.picacg_adapter_algorithms=domain/services/r18/cxx_modules/R18PicacgAdapter.cppm
            memochat.r18.public_dto_algorithms=domain/services/r18/cxx_modules/R18PublicDto.cppm
            memochat.r18.source_algorithms=domain/services/r18/cxx_modules/R18Source.cppm
            memochat.r18.source_service_algorithms=domain/services/r18/cxx_modules/R18SourceService.cppm
            memochat.r18.source_record_codec_algorithms=domain/services/r18/cxx_modules/R18SourceRecordCodec.cppm
        CONSUMER_SOURCES
            domain/services/r18/R18AdapterUtils.cpp
            domain/services/r18/R18JmAdapter.cpp
            domain/services/r18/R18PicacgAdapter.cpp
            domain/services/r18/R18PublicDtos.cpp
            domain/services/r18/R18SourceRecordCodec.cpp
            domain/services/r18/R18SourceService.cpp
    )
endif()

# --- GateR18Domain: r18 source admin/search routes + service facade. ----------
add_library(GateR18Domain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateR18Profile.cpp
    domain/GateR18Routes.cpp
    domain/modules/r18/R18RouteModule.cpp
    domain/modules/r18/R18RouteSchemas.cpp
    domain/services/r18/R18Service.cpp
)
memochat_configure_gate_app_lib(GateR18Domain)
target_include_directories(GateR18Domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateR18Domain PUBLIC GateRuntimeCore GateR18Core GateAccountCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateR18Domain
        MODULES
            memochat.r18.route_registration_algorithms=domain/modules/r18/cxx_modules/R18RouteRegistration.cppm
            memochat.r18.route_schema_algorithms=domain/modules/r18/cxx_modules/R18RouteSchema.cppm
            memochat.r18.service_algorithms=domain/services/r18/cxx_modules/R18Service.cppm
        CONSUMER_SOURCES
            domain/modules/r18/R18RouteModule.cpp
            domain/modules/r18/R18RouteSchemas.cpp
            domain/services/r18/R18Service.cpp
    )
endif()

add_executable(R18GatewayServer
    app/R18GatewayServer.cpp
)
memochat_configure_gate_focused_service(R18GatewayServer DOMAINS GateR18Domain)
