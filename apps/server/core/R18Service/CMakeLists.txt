# R18Service owns the r18 domain code (relocated from GateShared in slice j).
# GateR18Core (the R18 source service implementation) and GateR18Domain (r18
# route module + service facade); target names unchanged.
#
# NOTE: the R18 source service implementation lives under
# domain/services/r18 (relocated out of common/r18 in slice d, finding ③).
# GateR18Core exports domain/services as a PUBLIC include root so the
# `#include "r18/R18SourceService.h"` spelling used by R18Service.cpp,
# R18GatewayServer and the legacy H1 transport keeps resolving.

# --- GateR18Core: R18 source service implementation. --------------------------
add_library(GateR18Core STATIC
    domain/services/r18/R18PublicDtos.cpp
    domain/services/r18/R18PublicDtos.h
    domain/services/r18/R18SourceRecordCodec.cpp
    domain/services/r18/R18SourceRecordCodec.h
    domain/services/r18/R18SourceService.cpp
    domain/services/r18/R18SourceService.h
)
memochat_configure_gate_core_lib(GateR18Core)
target_include_directories(GateR18Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/services)
target_link_libraries(GateR18Core PUBLIC GateInfraCore)

# --- GateR18Domain: r18 source admin/search routes + service facade. ----------
add_library(GateR18Domain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateR18Profile.cpp
    domain/GateR18Routes.cpp
    domain/modules/r18/R18RouteModule.cpp
    domain/modules/r18/R18RouteSchemas.cpp
    domain/services/r18/R18Service.cpp
)
memochat_configure_gate_app_lib(GateR18Domain)
target_include_directories(GateR18Domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateR18Domain PUBLIC GateRuntimeCore GateR18Core)

add_executable(R18GatewayServer
    app/R18GatewayServer.cpp
)
memochat_configure_gate_focused_service(R18GatewayServer DOMAINS GateR18Domain)
