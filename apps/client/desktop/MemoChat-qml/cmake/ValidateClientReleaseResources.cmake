include_guard(GLOBAL)

function(memochat_validate_client_release_resources)
    set(one_value_args CLIENT_ROOT)
    set(multi_value_args MANIFESTS)
    cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_CLIENT_ROOT)
        message(FATAL_ERROR "CLIENT_ROOT is required for the client release resource audit")
    endif()

    foreach(manifest IN LISTS ARG_MANIFESTS)
        if(IS_ABSOLUTE "${manifest}")
            set(manifest_path "${manifest}")
        else()
            set(manifest_path "${ARG_CLIENT_ROOT}/${manifest}")
        endif()
        if(NOT EXISTS "${manifest_path}")
            message(FATAL_ERROR "Client resource manifest is missing: ${manifest_path}")
        endif()

        file(READ "${manifest_path}" manifest_contents)
        set(audited_text "${manifest}\n${manifest_contents}")
        foreach(restricted_pattern IN ITEMS
                "(^|[./])live2d/"
                "KafuuChino"
                "Kafuuchino-voice"
                "src/KafuuChino")
            if(audited_text MATCHES "${restricted_pattern}")
                message(FATAL_ERROR
                    "Restricted local Live2D/model/voice asset referenced by release resource manifest: "
                    "${manifest_path}")
            endif()
        endforeach()
    endforeach()
endfunction()
