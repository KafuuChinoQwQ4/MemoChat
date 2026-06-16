# GateAppCore: the shared H1 application layer (transport + routing + every
# domain module/service) used by the focused single-domain services peeled off
# during the gateserver microservice split. It holds everything the retired
# GateServer monolith used to compile EXCEPT an entrypoint, so each service exe
# links the same application core and selects a LogicSystem::RouteProfile.
add_library(GateAppCore STATIC
    CServer.cpp
    GateHttpJsonSupport.cpp
    GateServerH1Routes.cpp
    GateR18Routes.cpp
    GateWorkerPool.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    app/GateDomainServer.cpp
    adapters/h1/H1RouteAdapter.cpp
    modules/ai/AIRouteModule.cpp
    modules/auth/AuthRouteModule.cpp
    modules/call/CallRouteModule.cpp
    modules/health/HealthRouteModule.cpp
    modules/media/MediaRouteModule.cpp
    modules/moments/MomentsRouteModule.cpp
    modules/profile/ProfileRouteModule.cpp
    modules/r18/R18RouteModule.cpp
    routing/RouteRegistry.cpp
    services/ai/AIService.cpp
    services/account/AccountPersistence.cpp
    services/auth/AuthService.cpp
    services/media/MediaService.cpp
    services/moments/MomentsService.cpp
    services/r18/R18Service.cpp
    MediaHttpService.cpp
    AIServiceClient.cpp
    MomentsRouteModules.cpp
    AIRouteModules.cpp
    MomentsRouteModules.h
    GateRouteModules.h
    AIRouteModules.h
    transports/h3/legacy_routes/GateHttp3ServiceRoutes.h
    CertUtil.cpp
    CertUtil.h
)
set_target_properties(GateAppCore PROPERTIES FOLDER "Server/GateShared")
memochat_configure_server_target(GateAppCore)
target_link_libraries(GateAppCore PUBLIC
    GateServerCore
    GateServerHttp3
    memochat_boost_stubs
)
target_include_directories(GateAppCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/listener
    ${CMAKE_CURRENT_SOURCE_DIR}/transports/h3/legacy_routes
)

# The GateServer monolith executable is RETIRED and removed: every business
# domain is served by its own peeled microservice (AIGatewayService, MediaService,
# MomentsService, CallService, R18Service, RegisterService, LoginService,
# AccountService), and Envoy routes each prefix directly to those services. This
# directory now owns ONLY the shared GateAppCore application-layer library that
# those services link; there is no longer a GateServer entrypoint, config.ini,
# or embedded HTTP/2 monolith here.
