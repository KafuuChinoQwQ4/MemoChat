include_guard(GLOBAL)

option(MEMOCHAT_AUTO_FORMAT "Run C++ and Python source formatters before build targets" ON)
option(MEMOCHAT_FORMAT_CPP "Run clang-format as part of MemoChat formatting" ON)
option(MEMOCHAT_FORMAT_PYTHON "Run Ruff format/import sorting as part of MemoChat formatting" ON)
set(MEMOCHAT_FORMAT_PYTHON_EXECUTABLE "" CACHE FILEPATH
    "Python interpreter with MemoChat formatting tools. Defaults to .venv when present, then Python3.")

function(memochat_collect_python_script_dirs output_variable)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c
                "import os, site, sys, sysconfig; userbase = site.getuserbase(); paths = [sysconfig.get_path('scripts'), os.path.join(userbase, 'bin'), os.path.join(userbase, 'Scripts'), os.path.join(userbase, f'Python{sys.version_info.major}{sys.version_info.minor}', 'Scripts')]; print(';'.join(path for path in paths if path))"
        OUTPUT_VARIABLE MEMOCHAT_PYTHON_SCRIPT_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${output_variable} "${MEMOCHAT_PYTHON_SCRIPT_DIRS}" PARENT_SCOPE)
endfunction()

function(memochat_define_format_targets)
    set(MEMOCHAT_FORMAT_DEPENDENCIES)
    set(MEMOCHAT_FORMAT_CHECK_DEPENDENCIES)

    if(MEMOCHAT_AUTO_FORMAT)
        if(NOT MEMOCHAT_FORMAT_PYTHON_EXECUTABLE)
            if(WIN32)
                set(MEMOCHAT_REPO_VENV_PYTHON "${PROJECT_SOURCE_DIR}/.venv/Scripts/python.exe")
            else()
                set(MEMOCHAT_REPO_VENV_PYTHON "${PROJECT_SOURCE_DIR}/.venv/bin/python")
            endif()
            if(EXISTS "${MEMOCHAT_REPO_VENV_PYTHON}")
                set(MEMOCHAT_FORMAT_PYTHON_EXECUTABLE "${MEMOCHAT_REPO_VENV_PYTHON}" CACHE FILEPATH
                    "Python interpreter with MemoChat formatting tools." FORCE)
            endif()
        endif()

        if(MEMOCHAT_FORMAT_PYTHON_EXECUTABLE)
            set(Python3_EXECUTABLE "${MEMOCHAT_FORMAT_PYTHON_EXECUTABLE}")
        endif()
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        set(MEMOCHAT_FORMAT_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH
            "Python interpreter with MemoChat formatting tools." FORCE)
        memochat_collect_python_script_dirs(MEMOCHAT_PYTHON_SCRIPT_DIRS)

        if(MEMOCHAT_FORMAT_CPP)
            find_program(MEMOCHAT_CLANG_FORMAT
                NAMES clang-format clang-format-22 clang-format-21 clang-format-20 clang-format-19 clang-format-18 clang-format-17 clang-format-16
                HINTS ${MEMOCHAT_PYTHON_SCRIPT_DIRS})
            if(NOT MEMOCHAT_CLANG_FORMAT)
                message(FATAL_ERROR
                    "MEMOCHAT_AUTO_FORMAT is ON but clang-format was not found. "
                    "Install dev requirements with `python -m pip install -r requirements-dev.txt`, "
                    "install clang-format system-wide, set -DMEMOCHAT_CLANG_FORMAT=/path/to/clang-format, "
                    "or configure with -DMEMOCHAT_AUTO_FORMAT=OFF.")
            endif()
            add_custom_target(format-cpp
                COMMAND ${Python3_EXECUTABLE}
                        "${PROJECT_SOURCE_DIR}/tools/scripts/dev/format_sources.py"
                        --mode cpp
                        --clang-format "${MEMOCHAT_CLANG_FORMAT}"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                COMMENT "Formatting MemoChat C++ sources with clang-format"
                VERBATIM
                USES_TERMINAL)
            list(APPEND MEMOCHAT_FORMAT_DEPENDENCIES format-cpp)
            add_custom_target(format-cpp-check
                COMMAND ${Python3_EXECUTABLE}
                        "${PROJECT_SOURCE_DIR}/tools/scripts/dev/format_sources.py"
                        --mode cpp
                        --check
                        --clang-format "${MEMOCHAT_CLANG_FORMAT}"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                COMMENT "Checking MemoChat C++ formatting with clang-format"
                VERBATIM
                USES_TERMINAL)
            list(APPEND MEMOCHAT_FORMAT_CHECK_DEPENDENCIES format-cpp-check)
        endif()

        if(MEMOCHAT_FORMAT_PYTHON)
            execute_process(
                COMMAND ${Python3_EXECUTABLE} -m ruff --version
                RESULT_VARIABLE MEMOCHAT_RUFF_RESULT
                OUTPUT_VARIABLE MEMOCHAT_RUFF_VERSION
                ERROR_VARIABLE MEMOCHAT_RUFF_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)
            if(NOT MEMOCHAT_RUFF_RESULT EQUAL 0)
                message(FATAL_ERROR
                    "MEMOCHAT_AUTO_FORMAT is ON but Ruff is not available for ${Python3_EXECUTABLE}. "
                    "Install dev requirements with `python -m pip install -r requirements-dev.txt` "
                    "or configure with -DMEMOCHAT_AUTO_FORMAT=OFF. ${MEMOCHAT_RUFF_ERROR}")
            endif()
            add_custom_target(format-python
                COMMAND ${Python3_EXECUTABLE}
                        "${PROJECT_SOURCE_DIR}/tools/scripts/dev/format_sources.py"
                        --mode python
                        --python "${Python3_EXECUTABLE}"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                COMMENT "Formatting MemoChat Python sources with Ruff"
                VERBATIM
                USES_TERMINAL)
            list(APPEND MEMOCHAT_FORMAT_DEPENDENCIES format-python)
            add_custom_target(format-python-check
                COMMAND ${Python3_EXECUTABLE}
                        "${PROJECT_SOURCE_DIR}/tools/scripts/dev/format_sources.py"
                        --mode python
                        --check
                        --python "${Python3_EXECUTABLE}"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                COMMENT "Checking MemoChat Python formatting with Ruff"
                VERBATIM
                USES_TERMINAL)
            list(APPEND MEMOCHAT_FORMAT_CHECK_DEPENDENCIES format-python-check)
        endif()

        if(MEMOCHAT_FORMAT_DEPENDENCIES)
            add_custom_target(format DEPENDS ${MEMOCHAT_FORMAT_DEPENDENCIES})
        else()
            add_custom_target(format
                COMMAND ${CMAKE_COMMAND} -E echo "MemoChat format target has no enabled formatter."
                VERBATIM)
        endif()
        if(MEMOCHAT_FORMAT_CHECK_DEPENDENCIES)
            add_custom_target(format-check DEPENDS ${MEMOCHAT_FORMAT_CHECK_DEPENDENCIES})
        else()
            add_custom_target(format-check
                COMMAND ${CMAKE_COMMAND} -E echo "MemoChat format-check target has no enabled formatter."
                VERBATIM)
        endif()
    else()
        add_custom_target(format
            COMMAND ${CMAKE_COMMAND} -E echo "MemoChat automatic formatting is disabled."
            VERBATIM)
        add_custom_target(format-check
            COMMAND ${CMAKE_COMMAND} -E echo "MemoChat automatic formatting is disabled."
            VERBATIM)
    endif()
endfunction()

function(memochat_attach_format target_name)
    if(NOT MEMOCHAT_AUTO_FORMAT OR NOT TARGET format OR NOT TARGET ${target_name})
        return()
    endif()

    set(MEMOCHAT_FORMAT_RESERVED_TARGETS
        format
        format-check
        format-cpp
        format-cpp-check
        format-python
        format-python-check)
    if(target_name IN_LIST MEMOCHAT_FORMAT_RESERVED_TARGETS)
        return()
    endif()

    get_target_property(MEMOCHAT_TARGET_TYPE ${target_name} TYPE)
    if(MEMOCHAT_TARGET_TYPE MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY|UTILITY)$")
        add_dependencies(${target_name} format)
    endif()
endfunction()

function(memochat_attach_format_to_directory directory)
    get_property(MEMOCHAT_DIRECTORY_TARGETS DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(MEMOCHAT_DIRECTORY_TARGET IN LISTS MEMOCHAT_DIRECTORY_TARGETS)
        memochat_attach_format(${MEMOCHAT_DIRECTORY_TARGET})
    endforeach()

    get_property(MEMOCHAT_SUBDIRECTORIES DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(MEMOCHAT_SUBDIRECTORY IN LISTS MEMOCHAT_SUBDIRECTORIES)
        memochat_attach_format_to_directory("${MEMOCHAT_SUBDIRECTORY}")
    endforeach()
endfunction()

function(memochat_attach_format_to_all_targets)
    memochat_attach_format_to_directory("${PROJECT_SOURCE_DIR}")
endfunction()
