#!/bin/sh
set -eu

TARGET_BIN="${TARGET_BIN:-}"
CONFIG_PATH="${CONFIG_PATH:-/app/config.ini}"
RUN_WITH_CONFIG_ARG="${RUN_WITH_CONFIG_ARG:-false}"

if [ -z "${TARGET_BIN}" ]; then
  echo "TARGET_BIN is required" >&2
  exit 1
fi

if [ ! -x "${TARGET_BIN}" ]; then
  echo "Target binary is not executable: ${TARGET_BIN}" >&2
  exit 1
fi

if [ "${RUN_WITH_CONFIG_ARG}" = "true" ] && [ -f "${CONFIG_PATH}" ]; then
  exec "${TARGET_BIN}" --config "${CONFIG_PATH}"
fi

exec "${TARGET_BIN}"
