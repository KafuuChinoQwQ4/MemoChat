#!/bin/sh
set -eu

OUT_DIR="${1:-/certs}"
CERT_FILE="${OUT_DIR}/servercert.pem"
KEY_FILE="${OUT_DIR}/serverkey.pem"

mkdir -p "${OUT_DIR}"

if [ -s "${CERT_FILE}" ] && [ -s "${KEY_FILE}" ]; then
  exit 0
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

openssl req -x509 -nodes -newkey rsa:2048 -days 3650 \
  -keyout "${tmpdir}/serverkey.pem" \
  -out "${tmpdir}/servercert.pem" \
  -subj "/CN=localhost" \
  -addext "subjectAltName=DNS:localhost,DNS:127.0.0.1,DNS:host.docker.internal,IP:127.0.0.1" \
  -addext "extendedKeyUsage=serverAuth"

openssl rsa -in "${tmpdir}/serverkey.pem" -out "${KEY_FILE}"
mv "${tmpdir}/servercert.pem" "${CERT_FILE}"

chmod 644 "${CERT_FILE}" "${KEY_FILE}"
