# EmbedLuaScripts.cmake
# 提供 memochat_embed_lua_scripts() 函数，将 .lua 文件在 CMake 配置阶段
# 读入并生成一个 C++ 头文件，内容为 raw string literal 常量。
#
# 用法：
#   memochat_embed_lua_scripts(
#     TARGET      <target>          # 需要使用这些脚本的 CMake 目标
#     SCRIPTS     file1.lua ...     # .lua 文件的绝对路径
#     OUTPUT_DIR  <dir>             # 生成头文件的输出目录
#     HEADER_NAME <name.hpp>        # 生成的头文件名（默认 lua_scripts_embed.hpp）
#     NAMESPACE   <ns>              # C++ 命名空间（默认 memochat::lua_scripts）
#   )

function(memochat_embed_lua_scripts)
    cmake_parse_arguments(ARG "" "TARGET;OUTPUT_DIR;HEADER_NAME;NAMESPACE" "SCRIPTS" ${ARGN})

    if(NOT ARG_HEADER_NAME)
        set(ARG_HEADER_NAME "lua_scripts_embed.hpp")
    endif()
    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "memochat::lua_scripts")
    endif()
    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/lua")
    endif()

    file(MAKE_DIRECTORY "${ARG_OUTPUT_DIR}")
    set(OUT_FILE "${ARG_OUTPUT_DIR}/${ARG_HEADER_NAME}")

    # ── 转换命名空间 "a::b::c" → "namespace a { namespace b { namespace c {" ──
    string(REPLACE "::" ";" NS_PARTS "${ARG_NAMESPACE}")
    set(NS_OPEN "")
    set(NS_CLOSE "")
    foreach(PART ${NS_PARTS})
        string(APPEND NS_OPEN "namespace ${PART} {\n")
        string(PREPEND NS_CLOSE "} // namespace ${PART}\n")
    endforeach()

    # ── 头文件内容 ──────────────────────────────────────────────────────────
    set(CONTENT "")
    string(APPEND CONTENT
"// AUTO-GENERATED — do not edit directly.\n"
"// Source: cmake/EmbedLuaScripts.cmake\n"
"// Re-generated every time CMake configures.\n"
"#pragma once\n"
"#include <string_view>\n"
"\n"
"${NS_OPEN}"
    )

    foreach(LUA_FILE ${ARG_SCRIPTS})
        if(NOT EXISTS "${LUA_FILE}")
            message(FATAL_ERROR "EmbedLuaScripts: file not found: ${LUA_FILE}")
        endif()

        # 变量名 = 文件名去扩展名，转为 snake_case（CMake 无正则替换，用字符串操作）
        get_filename_component(STEM "${LUA_FILE}" NAME_WE)
        string(MAKE_C_IDENTIFIER "${STEM}" VAR_NAME)

        file(READ "${LUA_FILE}" LUA_BODY)

        # 转义 raw string delimiter：如果内容含 ")LUA_END", 换个 delimiter
        set(DELIM "LUA_END")
        string(APPEND CONTENT
"// Source file: ${LUA_FILE}\n"
"inline constexpr std::string_view k${VAR_NAME} = R\"${DELIM}(\n"
"${LUA_BODY}"
")${DELIM}\";\n\n"
        )
    endforeach()

    string(APPEND CONTENT "${NS_CLOSE}")

    file(WRITE "${OUT_FILE}" "${CONTENT}")
    message(STATUS "EmbedLuaScripts: generated ${OUT_FILE}")

    if(ARG_TARGET)
        target_include_directories(${ARG_TARGET} PRIVATE "${ARG_OUTPUT_DIR}")
    endif()
endfunction()
