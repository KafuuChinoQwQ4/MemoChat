# cmake/UpdateModuleStamp.cmake
# Called after compiling a C++ module CMI to update the stamp file only when
# the CMI content has actually changed. Consumer sources depend on the stamp
# (not the .gcm directly), so they are only rebuilt when module content changes,
# not every time GCC touches the .gcm timestamp during a consumer compile.
#
# Usage (from add_custom_command):
#   COMMAND ${CMAKE_COMMAND}
#       -DCMI_FILE=<path/to/module.gcm>
#       -DSTAMP_FILE=<path/to/module.stamp>
#       -P cmake/UpdateModuleStamp.cmake

if(NOT DEFINED CMI_FILE OR NOT DEFINED STAMP_FILE)
    message(FATAL_ERROR "UpdateModuleStamp: CMI_FILE and STAMP_FILE must be defined")
endif()

if(NOT EXISTS "${CMI_FILE}")
    message(FATAL_ERROR "UpdateModuleStamp: CMI_FILE '${CMI_FILE}' does not exist after compilation")
endif()

file(SHA256 "${CMI_FILE}" _new_hash)

set(_old_hash "")
if(EXISTS "${STAMP_FILE}")
    file(READ "${STAMP_FILE}" _old_hash)
    string(STRIP "${_old_hash}" _old_hash)
endif()

if(NOT _old_hash STREQUAL _new_hash)
    file(WRITE "${STAMP_FILE}" "${_new_hash}")
endif()
# If hashes match, stamp is not touched, so its timestamp stays old and
# consumers see "nothing changed", skipping recompilation.
