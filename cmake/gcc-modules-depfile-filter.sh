#!/usr/bin/env bash
# cmake/gcc-modules-depfile-filter.sh
#
# Compiler launcher wrapper for GNU C++ module consumers.
#
# Problem it solves:
#   g++ -fmodules combined with -MD/-MF writes a Make-style depfile that lists
#   phantom prerequisite targets named "<module.name>.c++-module" (plus a
#   "CXX_IMPORTS +=" block). No rule produces a file with that literal name, so
#   Ninja (deps = gcc) records it as an always-missing implicit dependency and
#   rebuilds the consumer object on every build even when nothing changed.
#
#   GCC also records raw .gcm CMI paths. MemoChat already wires content-hash
#   stamp files into OBJECT_DEPENDS, so raw .gcm mtimes are redundant and can
#   cause spurious rebuilds if the compiler touches CMI files while reading them.
#
# Fix:
#   Run the real compile, then sanitize the depfile in place. Header deps remain;
#   module-change deps are tracked by the explicit stamp files from CMake.
#
# Usage (as a CMake compiler launcher):
#   set_target_properties(<tgt> PROPERTIES CXX_COMPILER_LAUNCHER
#       "${CMAKE_SOURCE_DIR}/cmake/gcc-modules-depfile-filter.sh")
# The full compiler command line is passed as "$@".

set -euo pipefail

# Locate the -MF <depfile> / -MF<depfile> argument, if any.
depfile=""
args=("$@")
for ((i = 0; i < ${#args[@]}; ++i)); do
    arg="${args[$i]}"
    case "$arg" in
        -MF)
            if ((i + 1 < ${#args[@]})); then
                depfile="${args[$((i + 1))]}"
            fi
            ;;
        -MF*)
            depfile="${arg#-MF}"
            ;;
    esac
done

# Run the actual compilation.
set +e
"$@"
status=$?
set -e

# On success:
# 1. Drop the "CXX_IMPORTS += ..." bookkeeping block and its continuations.
# 2. Remove phantom "<module>.c++-module" prerequisites wherever GCC placed them.
# 3. Remove raw ".gcm" prerequisites; CMake stamp deps cover real module changes.
# 4. Remove empty continuation lines left behind by stripped module deps.
if [[ $status -eq 0 && -n "$depfile" && -f "$depfile" ]]; then
    perl -0pi -e \
        's/^CXX_IMPORTS[^\n]*(?:\\\n[^\n]*)*\n?//mg; s/[ \t]*[^ \t\n\\]+(?:\.c\+\+-module|\.gcm)\b//g; s/^[ \t]*\\[ \t]*\n//mg; s/[ \t]*\\[ \t]*\z/\n/s; s/[ \t]+$//mg' \
        "$depfile"
fi

exit $status
