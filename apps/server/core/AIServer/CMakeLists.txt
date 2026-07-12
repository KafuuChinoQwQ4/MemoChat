add_executable(AIServer
    AIServer.cpp
    AIServiceAlgorithms.cpp
    AIServiceAlgorithms.hpp
    AIServiceImpl.cpp
    AIServiceCore.cpp
    AIServiceClient.cpp
    AIServiceJsonDtos.cpp
    AIServiceJsonMapper.cpp
    ConfigMgr.cpp
    db/AISessionRepo.cpp
    db/AISmartLogRepo.cpp
)

set_target_properties(AIServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(AIServer)
target_link_libraries(AIServer PRIVATE memochat_boost_stubs ${MEMOCHAT_POSTGRES_TARGET})
if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    if(DEFINED ENV{WINSDK_LIB_UM} AND NOT "$ENV{WINSDK_LIB_UM}" STREQUAL "")
        set(_WINSDK_LIB_UM "$ENV{WINSDK_LIB_UM}")
    else()
        set(_WINSDK_LIB_UM "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64")
    endif()
    if(DEFINED ENV{WINSDK_LIB_UCRT} AND NOT "$ENV{WINSDK_LIB_UCRT}" STREQUAL "")
        set(_WINSDK_LIB_UCRT "$ENV{WINSDK_LIB_UCRT}")
    else()
        set(_WINSDK_LIB_UCRT "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/ucrt/x64")
    endif()
    target_link_options(AIServer PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
endif()

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(AIServer
        MODULES
            memochat.ai.client_algorithms=cxx_modules/AIServiceClient.cppm
            memochat.ai.core_algorithms=cxx_modules/AIServiceCore.cppm
            memochat.ai.impl_algorithms=cxx_modules/AIServiceImpl.cppm
            memochat.ai.json_dto_algorithms=cxx_modules/AIServiceJsonDto.cppm
            memochat.ai.repository_algorithms=cxx_modules/AIServerRepository.cppm
            memochat.ai.service_algorithms=cxx_modules/AIService.cppm
        CONSUMER_SOURCES
            AIServiceClient.cpp
            AIServiceCore.cpp
            AIServiceImpl.cpp
            AIServiceAlgorithms.cpp
            AIServiceJsonDtos.cpp
            db/AISessionRepo.cpp
            db/AISmartLogRepo.cpp
    )
endif()
