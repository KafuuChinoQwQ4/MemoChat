option(MEMOCHAT_ENABLE_GNU_STD_MODULES "Build libstdc++ std/std.compat modules for GNU module consumers" ON)

function(memochat_gnu_module_dialect out_variant out_options)
    if(MEMOCHAT_NO_EXCEPTIONS)
        set(${out_variant} "no-exceptions" PARENT_SCOPE)
        set(${out_options} "-fno-exceptions" PARENT_SCOPE)
    else()
        set(${out_variant} "exceptions" PARENT_SCOPE)
        set(${out_options} "" PARENT_SCOPE)
    endif()
endfunction()

function(memochat_find_gnu_std_module_source module_file out_var)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpfullversion -dumpversion
        OUTPUT_VARIABLE _gcc_full_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    set(_candidate_roots
        "/usr/include/c++/${CMAKE_CXX_COMPILER_VERSION}"
        "/usr/local/include/c++/${CMAKE_CXX_COMPILER_VERSION}"
    )
    if(_gcc_full_version)
        list(APPEND _candidate_roots
            "/usr/include/c++/${_gcc_full_version}"
            "/usr/local/include/c++/${_gcc_full_version}"
        )
    endif()
    list(REMOVE_DUPLICATES _candidate_roots)

    find_file(_module_source
        NAMES "bits/${module_file}.cc"
        PATHS ${_candidate_roots}
        NO_CACHE
    )
    set(${out_var} "${_module_source}" PARENT_SCOPE)
endfunction()

function(memochat_prepare_gnu_std_modules out_cmis out_stamps out_mapper_lines out_target)
    set(${out_cmis} "" PARENT_SCOPE)
    set(${out_stamps} "" PARENT_SCOPE)
    set(${out_mapper_lines} "" PARENT_SCOPE)
    set(${out_target} "" PARENT_SCOPE)

    if(NOT MEMOCHAT_ENABLE_GNU_STD_MODULES)
        return()
    endif()
    if(CMAKE_CXX_STANDARD LESS 23)
        return()
    endif()

    get_property(_configured GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULES_CONFIGURED)
    if(NOT _configured)
        memochat_find_gnu_std_module_source(std _std_module_source)
        if(NOT _std_module_source)
            message(FATAL_ERROR "MEMOCHAT_ENABLE_GNU_STD_MODULES requires libstdc++ bits/std.cc for ${CMAKE_CXX_COMPILER}.")
        endif()
        memochat_find_gnu_std_module_source(std.compat _std_compat_module_source)

        memochat_gnu_module_dialect(_module_variant _module_dialect_options)
        set(_std_module_build_dir "${CMAKE_BINARY_DIR}/CMakeFiles/memochat_gnu_std_modules/${_module_variant}")
        set(_std_gcm_cache_dir "${_std_module_build_dir}/gcm.cache")
        set(_std_mapper_file "${_std_module_build_dir}/std.mapper")
        set(_std_cmi "${_std_gcm_cache_dir}/std.gcm")
        set(_std_obj "${_std_module_build_dir}/std.o")
        set(_std_stamp "${_std_module_build_dir}/std.stamp")
        set(_std_mapper_lines "std ${_std_cmi}")
        set(_std_outputs "${_std_obj}" "${_std_cmi}" "${_std_stamp}")
        set(_std_cmis "${_std_cmi}")
        set(_std_stamps "${_std_stamp}")

        add_custom_command(
            OUTPUT "${_std_obj}" "${_std_cmi}" "${_std_stamp}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_std_module_build_dir}" "${_std_gcm_cache_dir}"
            COMMAND
                ${CMAKE_CXX_COMPILER}
                $<$<CONFIG:Debug>:-g>
                $<$<NOT:$<CONFIG:Debug>>:-O3>
                -std=c++${CMAKE_CXX_STANDARD}
                ${_module_dialect_options}
                -fmodules
                -fmodule-mapper=${_std_mapper_file}
                -c "${_std_module_source}"
                -o "${_std_obj}"
            COMMAND ${CMAKE_COMMAND}
                -DCMI_FILE=${_std_cmi}
                -DSTAMP_FILE=${_std_stamp}
                -P "${CMAKE_SOURCE_DIR}/cmake/UpdateModuleStamp.cmake"
            DEPENDS "${_std_module_source}" "${_std_mapper_file}"
            COMMENT "Building libstdc++ module std"
            VERBATIM
            COMMAND_EXPAND_LISTS
        )

        if(_std_compat_module_source)
            set(_std_compat_cmi "${_std_gcm_cache_dir}/std.compat.gcm")
            set(_std_compat_obj "${_std_module_build_dir}/std.compat.o")
            set(_std_compat_stamp "${_std_module_build_dir}/std.compat.stamp")
            list(APPEND _std_mapper_lines "std.compat ${_std_compat_cmi}")
            list(APPEND _std_outputs "${_std_compat_obj}" "${_std_compat_cmi}" "${_std_compat_stamp}")
            list(APPEND _std_cmis "${_std_compat_cmi}")
            list(APPEND _std_stamps "${_std_compat_stamp}")

            add_custom_command(
                OUTPUT "${_std_compat_obj}" "${_std_compat_cmi}" "${_std_compat_stamp}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_std_module_build_dir}" "${_std_gcm_cache_dir}"
                COMMAND
                    ${CMAKE_CXX_COMPILER}
                    $<$<CONFIG:Debug>:-g>
                    $<$<NOT:$<CONFIG:Debug>>:-O3>
                    -std=c++${CMAKE_CXX_STANDARD}
                    ${_module_dialect_options}
                    -fmodules
                    -fmodule-mapper=${_std_mapper_file}
                    -c "${_std_compat_module_source}"
                    -o "${_std_compat_obj}"
                COMMAND ${CMAKE_COMMAND}
                    -DCMI_FILE=${_std_compat_cmi}
                    -DSTAMP_FILE=${_std_compat_stamp}
                    -P "${CMAKE_SOURCE_DIR}/cmake/UpdateModuleStamp.cmake"
                DEPENDS "${_std_compat_module_source}" "${_std_stamp}" "${_std_mapper_file}"
                COMMENT "Building libstdc++ module std.compat"
                VERBATIM
                COMMAND_EXPAND_LISTS
            )
        endif()

        string(JOIN "\n" _std_mapper_content ${_std_mapper_lines})
        file(GENERATE
            OUTPUT "${_std_mapper_file}"
            CONTENT "${_std_mapper_content}\n"
        )

        add_custom_target(memochat_gnu_std_modules
            DEPENDS ${_std_outputs}
        )
        set_target_properties(memochat_gnu_std_modules PROPERTIES FOLDER "Server/Modules")

        set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULES_CONFIGURED TRUE)
        set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_CMIS "${_std_cmis}")
        set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_STAMPS "${_std_stamps}")
        set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_MAPPER_LINES "${_std_mapper_lines}")
        set_property(GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_TARGET memochat_gnu_std_modules)
    endif()

    get_property(_std_cmis GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_CMIS)
    get_property(_std_stamps GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_STAMPS)
    get_property(_std_mapper_lines GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_MAPPER_LINES)
    get_property(_std_target GLOBAL PROPERTY MEMOCHAT_GNU_STD_MODULE_TARGET)
    set(${out_cmis} "${_std_cmis}" PARENT_SCOPE)
    set(${out_stamps} "${_std_stamps}" PARENT_SCOPE)
    set(${out_mapper_lines} "${_std_mapper_lines}" PARENT_SCOPE)
    set(${out_target} "${_std_target}" PARENT_SCOPE)
endfunction()

function(memochat_enable_gnu_modules target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "memochat_enable_gnu_modules(${target_name}) called before target exists")
    endif()

    set(options)
    set(oneValueArgs)
    set(multiValueArgs
        MODULES
        PRIVATE_SOURCES
        CONSUMER_SOURCES
        MODULE_INCLUDE_DIRS
        MODULE_DEFINITIONS
        MODULE_OPTIONS
        MODULE_DEPENDS
    )
    cmake_parse_arguments(MEMOCHAT_MODULES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MEMOCHAT_MODULES_MODULES)
        message(FATAL_ERROR "memochat_enable_gnu_modules(${target_name}) requires MODULES <name=path> entries")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "MemoChat custom CMI management currently supports GNU g++ only")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
        message(FATAL_ERROR "MemoChat custom CMI management requires g++ >= 16")
    endif()
    if(CMAKE_CXX_STANDARD LESS 20)
        message(FATAL_ERROR "MemoChat custom CMI management requires C++20 or newer")
    endif()

    memochat_gnu_module_dialect(_module_variant _module_dialect_options)
    set(_module_build_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.modules/${_module_variant}")
    set(_gcm_cache_dir "${_module_build_dir}/gcm.cache")
    set(_mapper_file "${_module_build_dir}/module.mapper")
    set(_module_objects)
    set(_module_cmis)
    set(_module_stamps)
    set(_mapper_lines)
    set(_module_interface_paths)
    set(_module_include_args)
    set(_module_definition_args)
    set(_std_module_cmis)
    set(_std_module_stamps)
    set(_std_module_mapper_lines)
    set(_std_module_target)

    memochat_prepare_gnu_std_modules(_std_module_cmis _std_module_stamps _std_module_mapper_lines _std_module_target)
    list(APPEND _module_cmis ${_std_module_cmis})
    list(APPEND _module_stamps ${_std_module_stamps})
    list(APPEND _mapper_lines ${_std_module_mapper_lines})

    foreach(_module_include_dir IN LISTS MEMOCHAT_MODULES_MODULE_INCLUDE_DIRS)
        if(NOT IS_ABSOLUTE "${_module_include_dir}")
            set(_module_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_module_include_dir}")
        endif()
        list(APPEND _module_include_args "-I${_module_include_dir}")
    endforeach()

    foreach(_module_definition IN LISTS MEMOCHAT_MODULES_MODULE_DEFINITIONS)
        list(APPEND _module_definition_args "-D${_module_definition}")
    endforeach()

    foreach(_module_entry IN LISTS MEMOCHAT_MODULES_MODULES)
        if(NOT _module_entry MATCHES "^([^=]+)=(.+)$")
            message(FATAL_ERROR "Invalid module entry '${_module_entry}'. Expected <module-name>=<interface-path>.")
        endif()

        set(_module_name "${CMAKE_MATCH_1}")
        set(_module_source "${CMAKE_MATCH_2}")
        if(NOT IS_ABSOLUTE "${_module_source}")
            set(_module_source "${CMAKE_CURRENT_SOURCE_DIR}/${_module_source}")
        endif()
        if(NOT EXISTS "${_module_source}")
            message(FATAL_ERROR "Module interface '${_module_source}' for '${_module_name}' does not exist")
        endif()

        string(REPLACE "." "/" _module_cmi_rel "${_module_name}")
        get_filename_component(_module_cmi_dir "${_gcm_cache_dir}/${_module_cmi_rel}.gcm" DIRECTORY)
        set(_module_cmi "${_gcm_cache_dir}/${_module_cmi_rel}.gcm")
        string(MAKE_C_IDENTIFIER "${_module_name}" _module_id)
        set(_module_obj "${_module_build_dir}/${_module_id}.o")

        # Stamp file: only updated when CMI content (SHA256) actually changes.
        # Consumer sources depend on the stamp rather than the .gcm directly, so
        # they are not rebuilt just because GCC touched the .gcm timestamp while
        # reading it during a consumer compile.
        set(_module_stamp "${_module_build_dir}/${_module_id}.stamp")

        list(APPEND _mapper_lines "${_module_name} ${_module_cmi}")
        list(APPEND _module_interface_paths "${_module_source}")
        list(APPEND _module_objects "${_module_obj}")
        list(APPEND _module_cmis "${_module_cmi}")
        list(APPEND _module_stamps "${_module_stamp}")
        set_source_files_properties("${_module_obj}" PROPERTIES
            EXTERNAL_OBJECT TRUE
            GENERATED TRUE
        )

        add_custom_command(
            OUTPUT "${_module_obj}" "${_module_cmi}" "${_module_stamp}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_module_build_dir}" "${_module_cmi_dir}"
            COMMAND
                ${CMAKE_CXX_COMPILER}
                $<$<CONFIG:Debug>:-g>
                $<$<NOT:$<CONFIG:Debug>>:-O3>
                -std=c++${CMAKE_CXX_STANDARD}
                ${_module_dialect_options}
                -fmodules
                -fmodule-mapper=${_mapper_file}
                ${_module_include_args}
                ${_module_definition_args}
                ${MEMOCHAT_MODULES_MODULE_OPTIONS}
                -c "${_module_source}"
                -o "${_module_obj}"
            # Update stamp only when CMI content changed; keeps consumer timestamps stable.
            # The stamp is a declared OUTPUT so Ninja knows the rule that produces it;
            # CMake sets restat=1 on custom commands, so an unchanged stamp does not
            # cascade a rebuild to the consumers that depend on it.
            COMMAND ${CMAKE_COMMAND}
                -DCMI_FILE=${_module_cmi}
                -DSTAMP_FILE=${_module_stamp}
                -P "${CMAKE_SOURCE_DIR}/cmake/UpdateModuleStamp.cmake"
            DEPENDS "${_module_source}" "${_mapper_file}" ${_std_module_stamps} ${MEMOCHAT_MODULES_MODULE_DEPENDS}
            COMMENT "Building C++ module ${_module_name} for ${target_name}"
            VERBATIM
            COMMAND_EXPAND_LISTS
        )
    endforeach()

    string(JOIN "\n" _mapper_content ${_mapper_lines})
    file(GENERATE
        OUTPUT "${_mapper_file}"
        CONTENT "${_mapper_content}\n"
    )

    add_custom_target(${target_name}_cxx_modules
        DEPENDS ${_module_objects} ${_module_cmis} ${_module_stamps}
        SOURCES ${_module_interface_paths}
    )
    set_target_properties(${target_name}_cxx_modules PROPERTIES FOLDER "Server/Modules")
    if(_std_module_target)
        add_dependencies(${target_name}_cxx_modules ${_std_module_target})
    endif()

    add_dependencies(${target_name} ${target_name}_cxx_modules)

    # Route this target's compiles through the depfile filter. g++ -fmodules -MD
    # emits phantom "<module>.c++-module" prerequisites into the Make depfile that
    # Ninja (deps = gcc) treats as always-missing. It also records raw .gcm paths,
    # whose mtimes can move independently of CMI content. The launcher strips both
    # after compile; explicit OBJECT_DEPENDS on content-hash stamps below preserve
    # the real module-change dependency without poisoning Ninja's deps log.
    get_target_property(_existing_cxx_launcher ${target_name} CXX_COMPILER_LAUNCHER)
    set(_module_depfile_filter "${CMAKE_SOURCE_DIR}/cmake/gcc-modules-depfile-filter.sh")
    if(_existing_cxx_launcher)
        set(_new_cxx_launcher "${_existing_cxx_launcher};${_module_depfile_filter}")
    else()
        set(_new_cxx_launcher "${_module_depfile_filter}")
    endif()
    set_target_properties(${target_name} PROPERTIES CXX_COMPILER_LAUNCHER "${_new_cxx_launcher}")

    target_sources(${target_name} PRIVATE ${MEMOCHAT_MODULES_PRIVATE_SOURCES} ${_module_objects})
    set(_module_consumer_sources ${MEMOCHAT_MODULES_PRIVATE_SOURCES} ${MEMOCHAT_MODULES_CONSUMER_SOURCES})
    foreach(_consumer_source IN LISTS _module_consumer_sources)
        if(NOT IS_ABSOLUTE "${_consumer_source}")
            set(_consumer_source "${CMAKE_CURRENT_SOURCE_DIR}/${_consumer_source}")
        endif()
        # Depend on stamp files (content-hash gated) rather than raw .gcm files
        # (timestamp-based). This prevents spurious rebuilds when GCC touches .gcm
        # timestamps while reading them during consumer compilation.
        set_source_files_properties("${_consumer_source}" PROPERTIES
            OBJECT_DEPENDS "${_module_stamps};${_mapper_file}")
        set_property(SOURCE "${_consumer_source}" APPEND_STRING PROPERTY COMPILE_FLAGS
            " -fmodules -fmodule-mapper=${_mapper_file}"
        )
    endforeach()
    set_property(TARGET ${target_name} APPEND PROPERTY MEMOCHAT_CXX_MODULE_CMIS "${_module_cmis}")
    set_property(TARGET ${target_name} APPEND PROPERTY MEMOCHAT_CXX_MODULE_MAPPER "${_mapper_file}")
endfunction()
