#!/usr/bin/env bash
set -Eeuo pipefail

umask 077

readonly SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
readonly PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)"
readonly RELEASE_VERIFIER="$SCRIPT_DIR/verify_release_tree.sh"
readonly LEGAL_VERIFIER="$SCRIPT_DIR/verify_release_legal.sh"
readonly CLIENT_SCAN_ALLOWLIST="$SCRIPT_DIR/client_release_scan.allowlist"

die() {
    printf '%s: %s\n' "$SCRIPT_NAME" "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Build a relocatable MemoChat Linux client directory and tar.gz from allowlisted inputs.

Usage:
  package_linux_client.sh \
    --binary build-linux-client-release-gcc16/bin/MemoChatQml \
    --config build-linux-client-release-gcc16/bin/config.ini \
    --output-dir artifacts/release \
    [--qt-root /path/to/qt] \
    [--qml-source-root apps/client/desktop/MemoChat-qml] \
    [--ca-cert /path/to/local-deployment-ca.pem] \
    [--library-dir /path/to/compiler/runtime/lib]... \
    [--source-sha 40-character-commit] \
    [--artifact-name MemoChatQml-linux-x86_64]

Recommended build input:
  cmake --preset linux-client-release-gcc16
  cmake --build --preset linux-client-release-gcc16 --target MemoChatQml

The input must be a Release ELF built with native Live2D disabled. The script
creates a fresh private staging directory, copies only the executable, public
config, an explicitly supplied public PEM X.509 deployment CA certificate, Qt runtime,
and recursively resolved non-system libraries, then strips ELFs, assigns
origin-relative RPATHs, audits dependencies and sensitive files, and writes
both MANIFEST.sha256 and a tarball checksum. Without --ca-cert, no local CA is
embedded and the client uses the system trust store.
EOF
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "required command is missing: $1"
}

canonical_file() {
    [[ -f "$1" ]] || die "required file does not exist: $1"
    realpath "$1"
}

is_single_pem_certificate() {
    LC_ALL=C awk '
        {
            line = $0
            sub(/\r$/, "", line)
            if (line == "-----BEGIN CERTIFICATE-----" && state == 0 && certificates == 0) {
                state = 1
                certificates = 1
                next
            }
            if (line == "-----END CERTIFICATE-----" && state == 1) {
                state = 2
                next
            }
            if (state == 1 && line ~ /^[A-Za-z0-9+\/]+={0,2}$/) {
                next
            }
            if (line == "" && (state == 0 || state == 2)) {
                next
            }
            invalid = 1
        }
        END { exit(invalid || state != 2 || certificates != 1 ? 1 : 0) }
    ' "$1"
}

is_elf() {
    file -Lb "$1" | grep -q '^ELF '
}

is_dynamic_elf() {
    is_elf "$1" && readelf -h "$1" 2>/dev/null | grep -Eq 'Type:[[:space:]]+(DYN|EXEC)'
}

has_native_live2d_dependency() {
    readelf -d "$1" 2>/dev/null | grep -Fq 'Shared library: [libLive2DCubismCore'
}

is_developer_path() {
    grep -Eq '/root/code/|/data/(Qt|vcpkg|third_party)/|/home/[^/]+/(code|src|build)/'
}

contains_developer_path() {
    grep -aEq '/root/code/|/data/(Qt|vcpkg|third_party)/|/home/[^/]+/(code|src|build)/' "$1"
}

contains_email_address() {
    local match
    match="$(LC_ALL=C strings "$1" 2>/dev/null |
        grep -Eim1 '[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}' || true)"
    [[ -n "$match" ]]
}

is_project_owned_release_file() {
    local candidate="$1"
    local stage_root="$2"
    local relative_path="${candidate#"$stage_root/"}"

    case "$relative_path" in
        MemoChatQml | MemoChat | config.ini | qt.conf | RELEASE-INFO.txt) return 0 ;;
        *) return 1 ;;
    esac
}

has_release_policy_marker() {
    readelf -p .note.memochat.release "$1" 2>/dev/null |
        grep -Fq 'MEMOCHAT_RELEASE_POLICY:v1;distributable=1;live2d_native=0;restricted_assets=0'
}

is_system_library() {
    case "$1" in
        /lib/* | /lib64/* | /usr/lib/* | /usr/lib64/*)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

is_qt_library() {
    local base
    base="$(basename "$1")"
    [[ "$base" == libQt6*.so* ]]
}

is_compiler_runtime() {
    case "$(basename "$1")" in
        libstdc++.so.6 | libgcc_s.so.1 | libatomic.so.1)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

binary=""
config=""
output_dir=""
qt_root=""
qml_source_root="$SCRIPT_DIR/../../../apps/client/desktop/MemoChat-qml"
ca_cert=""
source_sha=""
library_dirs=()
artifact_name="MemoChatQml-linux-x86_64"

while (($# > 0)); do
    case "$1" in
        --binary)
            (($# >= 2)) || die "--binary requires a value"
            binary="$2"
            shift 2
            ;;
        --config)
            (($# >= 2)) || die "--config requires a value"
            config="$2"
            shift 2
            ;;
        --output-dir)
            (($# >= 2)) || die "--output-dir requires a value"
            output_dir="$2"
            shift 2
            ;;
        --qt-root)
            (($# >= 2)) || die "--qt-root requires a value"
            qt_root="$2"
            shift 2
            ;;
        --qml-source-root)
            (($# >= 2)) || die "--qml-source-root requires a value"
            qml_source_root="$2"
            shift 2
            ;;
        --ca-cert)
            (($# >= 2)) || die "--ca-cert requires a value"
            ca_cert="$2"
            shift 2
            ;;
        --library-dir)
            (($# >= 2)) || die "--library-dir requires a value"
            library_dirs+=("$2")
            shift 2
            ;;
        --artifact-name)
            (($# >= 2)) || die "--artifact-name requires a value"
            artifact_name="$2"
            shift 2
            ;;
        --source-sha)
            (($# >= 2)) || die "--source-sha requires a value"
            source_sha="$2"
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

[[ -n "$binary" ]] || die "--binary is required"
[[ -n "$config" ]] || die "--config is required"
[[ -n "$output_dir" ]] || die "--output-dir is required"
[[ "$artifact_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] ||
    die "--artifact-name may contain only letters, numbers, dots, underscores, and hyphens"

for command_name in file readelf ldd patchelf strip strings realpath mktemp find grep awk sort tail jq python3 env \
    sha256sum tar cp install wc xargs mv chmod rm dirname basename; do
    require_command "$command_name"
done
[[ -x "$RELEASE_VERIFIER" ]] || die "release verifier is missing or not executable: $RELEASE_VERIFIER"
[[ -x "$LEGAL_VERIFIER" ]] || die "legal verifier is missing or not executable: $LEGAL_VERIFIER"
[[ -f "$CLIENT_SCAN_ALLOWLIST" ]] || die "client release scan allowlist is missing: $CLIENT_SCAN_ALLOWLIST"
legal_args=(--project-root "$PROJECT_ROOT")
[[ -z "$source_sha" ]] || legal_args+=(--source-sha "$source_sha")

binary="$(canonical_file "$binary")"
config="$(canonical_file "$config")"
if [[ -n "$ca_cert" ]]; then
    require_command openssl
    ca_cert="$(canonical_file "$ca_cert")"
    if grep -aEq -- '-----BEGIN ([A-Z0-9]+ )*PRIVATE KEY-----' "$ca_cert"; then
        die "deployment CA certificate file contains private key material"
    fi
    is_single_pem_certificate "$ca_cert" ||
        die "deployment CA certificate file must contain exactly one PEM certificate and no other content"
    openssl x509 -in "$ca_cert" -inform PEM -noout >/dev/null 2>&1 ||
        die "deployment CA certificate is not a valid PEM X.509 certificate: $ca_cert"
fi
[[ -x "$binary" ]] || die "client binary is not executable: $binary"
is_elf "$binary" || die "client binary is not an ELF executable: $binary"
has_native_live2d_dependency "$binary" &&
    die "native Live2D dependency is not redistributable; rebuild with MEMOCHAT_ENABLE_LIVE2D_NATIVE=OFF"
has_release_policy_marker "$binary" ||
    die "client binary is missing the distributable release policy marker; use linux-client-release-gcc16"
if readelf -d "$binary" 2>/dev/null | grep -Eq '\((RPATH|RUNPATH)\)'; then
    die "client build input must not contain RPATH or RUNPATH; use the distributable release preset"
fi

if [[ -z "$qt_root" ]]; then
    require_command qtpaths6
    qt_root="$(qtpaths6 --query QT_INSTALL_PREFIX)"
fi
[[ -d "$qt_root" ]] || die "Qt root does not exist: $qt_root"
qt_root="$(realpath "$qt_root")"
for qt_directory in lib plugins qml; do
    [[ -d "$qt_root/$qt_directory" ]] || die "Qt runtime directory is missing: $qt_root/$qt_directory"
done
[[ -d "$qml_source_root" ]] || die "QML source root does not exist: $qml_source_root"
qml_source_root="$(realpath "$qml_source_root")"
for index in "${!library_dirs[@]}"; do
    [[ -d "${library_dirs[$index]}" ]] || die "runtime library directory is missing: ${library_dirs[$index]}"
    library_dirs[$index]="$(realpath "${library_dirs[$index]}")"
done

python3 - "$config" "$SCRIPT_NAME" "$([[ -n "$ca_cert" ]] && printf 'true' || printf 'false')" <<'PY'
import configparser
import re
import sys
from pathlib import Path

config_path = Path(sys.argv[1])
script_name = sys.argv[2]
ca_cert_requested = sys.argv[3] == "true"
parser = configparser.ConfigParser(interpolation=None, strict=True, empty_lines_in_values=False)
try:
    with config_path.open("r", encoding="utf-8-sig") as stream:
        parser.read_file(stream)
except (OSError, UnicodeError, configparser.Error) as error:
    raise SystemExit(f"{script_name}: invalid public INI config: {error}")

log_sections = [section for section in parser.sections() if section.casefold() == "log"]
if len(log_sections) != 1 or not parser.has_option(log_sections[0], "redact"):
    raise SystemExit(f"{script_name}: public config must explicitly set [Log] Redact=true")
if parser.get(log_sections[0], "redact").strip().casefold() != "true":
    raise SystemExit(f"{script_name}: public config must explicitly set [Log] Redact=true")

gate_sections = [section for section in parser.sections() if section.casefold() == "gateserver"]
if len(gate_sections) > 1:
    raise SystemExit(f"{script_name}: public config contains duplicate [GateServer] sections")
if not ca_cert_requested and gate_sections:
    configured_ca = parser.get(gate_sections[0], "ca_file", raw=True, fallback="").strip()
    if configured_ca:
        raise SystemExit(
            f"{script_name}: public config ca_file must be empty unless --ca-cert supplies the packaged certificate"
        )

sensitive_keys = {
    "accesstoken", "refreshtoken", "authtoken", "token", "authorization",
    "password", "passwd", "pwd", "secret", "session", "sessionid", "cookie",
    "apikey", "jwtkey", "privatekey", "clientsecret", "jwtsecret", "hmackey",
    "accesskey", "secretkey", "loginticket", "verifycode", "turncredential",
    "email", "registrationemail", "useremail",
}
email_pattern = re.compile(r"\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b", re.IGNORECASE)
query_secret_pattern = re.compile(
    r"[?&](?:access[_-]?(?:token|key)|refresh[_-]?token|auth[_-]?token|"
    r"client[_-]?secret|jwt[_-]?secret|hmac[_-]?key|secret[_-]?key|"
    r"login[_-]?ticket|verify[_-]?code|token|authorization|password|passwd|pwd|"
    r"secret|session(?:[_-]?id)?|cookie|api[_-]?key|email)=([^&#\s]+)",
    re.IGNORECASE,
)
developer_path_pattern = re.compile(
    r"/root/code/|/data/(?:Qt|vcpkg|third_party)/|/home/[^/]+/(?:code|src|build)/"
)

for section in parser.sections():
    for key, value in parser.items(section, raw=True):
        normalized_key = re.sub(r"[^a-z0-9]", "", key.casefold())
        stripped_value = value.strip()
        if normalized_key in sensitive_keys and stripped_value:
            raise SystemExit(
                f"{script_name}: public config contains assigned credential or personal-data key "
                f"[{section}] {key}"
            )
        if email_pattern.search(stripped_value) or query_secret_pattern.search(stripped_value):
            raise SystemExit(
                f"{script_name}: public config contains credential or personal data in [{section}] {key}"
            )
        if developer_path_pattern.search(stripped_value):
            raise SystemExit(
                f"{script_name}: public config contains a developer-machine path in [{section}] {key}"
            )
PY

mkdir -p "$output_dir"
output_dir="$(realpath "$output_dir")"
final_dir="$output_dir/$artifact_name"
final_tar="$output_dir/$artifact_name.tar.gz"
final_checksum="$final_tar.sha256"
[[ ! -e "$final_dir" ]] || die "output already exists: $final_dir"
[[ ! -e "$final_tar" ]] || die "output already exists: $final_tar"
[[ ! -e "$final_checksum" ]] || die "output already exists: $final_checksum"

work_root="$(mktemp -d "$output_dir/.${artifact_name}.staging.XXXXXX")"
cleanup() {
    rm -rf -- "$work_root"
}
trap cleanup EXIT

stage="$work_root/$artifact_name"
mkdir -p "$stage/lib" "$stage/plugins" "$stage/qml"
install -m 0755 "$binary" "$stage/MemoChatQml"
install -m 0644 "$config" "$stage/config.ini"
"$LEGAL_VERIFIER" "${legal_args[@]}" --copy-to "$stage/legal"
legal_status="$stage/legal/LEGAL-STATUS.txt"
legal_inventory="$(awk -F= '$1 == "third_party_inventory" { print $2 }' "$legal_status")"
third_party_legal_corpus="$(awk -F= '$1 == "third_party_legal_corpus" { print $2 }' "$legal_status")"
corpus_review_id="$(awk -F= '$1 == "corpus_review_id" { print $2 }' "$legal_status")"
corpus_sha256="$(awk -F= '$1 == "corpus_sha256" { print $2 }' "$legal_status")"
release_source_sha="$(awk -F= '$1 == "release_source_sha" { print $2 }' "$legal_status")"
formal_distribution_ready="$(awk -F= '$1 == "formal_distribution_ready" { print $2 }' "$legal_status")"
legal_status_sha256="$(sha256sum "$legal_status" | awk '{print $1}')"
[[ "$legal_inventory" == complete ]] || die "generated legal inventory status is invalid"
[[ "$third_party_legal_corpus" == complete || "$third_party_legal_corpus" == incomplete ]] \
    || die "generated third-party legal corpus status is invalid"
[[ "$formal_distribution_ready" == true || "$formal_distribution_ready" == false ]] \
    || die "generated formal distribution status is invalid"
deployment_trust_anchor="system trust store"
if [[ -n "$ca_cert" ]]; then
    readonly packaged_ca_path="certs/memochat-local-ca.crt"
    mkdir -p "$stage/certs"
    install -m 0644 "$ca_cert" "$stage/$packaged_ca_path"
    python3 - "$stage/config.ini" "$packaged_ca_path" "$SCRIPT_NAME" <<'PY'
import configparser
import sys
from pathlib import Path

config_path = Path(sys.argv[1])
packaged_ca_path = sys.argv[2]
script_name = sys.argv[3]
parser = configparser.ConfigParser(interpolation=None, strict=True, empty_lines_in_values=False)
parser.optionxform = str
try:
    with config_path.open("r", encoding="utf-8-sig") as stream:
        parser.read_file(stream)
except (OSError, UnicodeError, configparser.Error) as error:
    raise SystemExit(f"{script_name}: invalid staged public INI config: {error}")

gate_sections = [section for section in parser.sections() if section.casefold() == "gateserver"]
if len(gate_sections) != 1:
    raise SystemExit(
        f"{script_name}: --ca-cert requires exactly one [GateServer] section in the public config"
    )
gate_section = gate_sections[0]
ca_options = [option for option in parser.options(gate_section) if option.casefold() == "ca_file"]
if len(ca_options) > 1:
    raise SystemExit(f"{script_name}: public config contains duplicate [GateServer] ca_file options")
ca_option = ca_options[0] if ca_options else "ca_file"
parser.set(gate_section, ca_option, packaged_ca_path)

try:
    with config_path.open("w", encoding="utf-8", newline="\n") as stream:
        parser.write(stream, space_around_delimiters=False)
except OSError as error:
    raise SystemExit(f"{script_name}: could not update staged public INI config: {error}")
PY
    deployment_trust_anchor="./$packaged_ca_path"
fi
dependency_search_path="$stage/lib:$qt_root/lib"
for library_dir in "${library_dirs[@]}"; do
    dependency_search_path="$dependency_search_path:$library_dir"
done

# Qt-owned runtime entries are copied from explicit allowlists. No repository
# runtime directory, user data directory, or previous deployment staging is copied.
if readelf -d "$binary" | grep -Eq 'libQt6(Core|Gui)\.so'; then
    required_qt_plugins=(
        platforms/libqxcb.so
        iconengines/libqsvgicon.so
        imageformats/libqgif.so
        imageformats/libqico.so
        imageformats/libqjpeg.so
        imageformats/libqsvg.so
        multimedia/libffmpegmediaplugin.so
        sqldrivers/libqsqlite.so
        tls/libqcertonlybackend.so
        tls/libqopensslbackend.so
        xcbglintegrations/libqxcb-egl-integration.so
        xcbglintegrations/libqxcb-glx-integration.so
    )
    optional_qt_plugins=(
        platforms/libqoffscreen.so
        platforminputcontexts/libcomposeplatforminputcontextplugin.so
        platforminputcontexts/libibusplatforminputcontextplugin.so
        platformthemes/libqxdgdesktopportal.so
        networkinformation/libqglib.so
        networkinformation/libqnetworkmanager.so
        position/libqtposition_geoclue2.so
        position/libqtposition_positionpoll.so
    )
    for relative_plugin in "${required_qt_plugins[@]}"; do
        plugin_source="$qt_root/plugins/$relative_plugin"
        [[ -f "$plugin_source" ]] || die "required Qt plugin is missing: $plugin_source"
        mkdir -p "$stage/plugins/$(dirname "$relative_plugin")"
        install -m 0644 "$plugin_source" "$stage/plugins/$relative_plugin"
    done
    for relative_plugin in "${optional_qt_plugins[@]}"; do
        plugin_source="$qt_root/plugins/$relative_plugin"
        [[ -f "$plugin_source" ]] || continue
        mkdir -p "$stage/plugins/$(dirname "$relative_plugin")"
        install -m 0644 "$plugin_source" "$stage/plugins/$relative_plugin"
    done
fi
if [[ -n "$(find "$qt_root/qml" -mindepth 1 -print -quit)" ]]; then
    qml_import_scanner="$qt_root/libexec/qmlimportscanner"
    if [[ ! -x "$qml_import_scanner" ]]; then
        qml_import_scanner="$qt_root/bin/qmlimportscanner"
    fi
    [[ -x "$qml_import_scanner" ]] || die "qmlimportscanner is missing under Qt root: $qt_root"
    qml_imports_json="$work_root/qml-imports.json"
    "$qml_import_scanner" \
        -rootPath "$qml_source_root" \
        -importPath "$qt_root/qml" >"$qml_imports_json"
    jq -e 'type == "array"' "$qml_imports_json" >/dev/null ||
        die "qmlimportscanner returned invalid JSON"

    copied_qml_module=false
    while IFS=$'\t' read -r module_path relative_path; do
        [[ -n "$module_path" && -n "$relative_path" ]] || continue
        [[ -d "$module_path" ]] || die "scanned QML module path is missing: $module_path"
        module_path="$(realpath "$module_path")"
        case "$module_path/" in
            "$qt_root/qml/"*) ;;
            *) die "scanned QML module escapes the Qt import root: $module_path" ;;
        esac
        case "$relative_path" in
            /* | .. | ../* | */../*) die "unsafe QML module relative path: $relative_path" ;;
        esac
        mkdir -p "$stage/qml/$relative_path"
        cp -a "$module_path/." "$stage/qml/$relative_path/"
        copied_qml_module=true
    done < <(
        jq -r '
            .[]
            | select(.type == "module" and (.path | type) == "string" and (.relativePath | type) == "string")
            | [.path, .relativePath]
            | @tsv
        ' "$qml_imports_json" | sort -u
    )
    [[ "$copied_qml_module" == true ]] || die "qmlimportscanner found no deployable Qt modules"
fi
if [[ -d "$qt_root/translations" ]]; then
    mkdir -p "$stage/translations"
    cp -a "$qt_root/translations/." "$stage/translations/"
fi

if readelf -d "$binary" | grep -Eq 'libQt6WebEngine(Core|Quick)\.so'; then
    webengine_process="$qt_root/libexec/QtWebEngineProcess"
    [[ -x "$webengine_process" ]] || die "QtWebEngineProcess is missing: $webengine_process"
    [[ -d "$qt_root/resources" ]] || die "Qt WebEngine resources directory is missing: $qt_root/resources"
    mkdir -p "$stage/libexec" "$stage/resources"
    install -m 0755 "$webengine_process" "$stage/libexec/QtWebEngineProcess"
    cp -a "$qt_root/resources/." "$stage/resources/"
fi

cat >"$stage/qt.conf" <<'EOF'
[Paths]
Prefix=.
Libraries=lib
Plugins=plugins
Qml2Imports=qml
Translations=translations
LibraryExecutables=libexec
Data=.
EOF

cat >"$stage/MemoChat" <<'EOF'
#!/bin/sh
set -eu
app_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$app_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$app_dir/plugins"
export QML2_IMPORT_PATH="$app_dir/qml"
if [ -x "$app_dir/libexec/QtWebEngineProcess" ]; then
    export QTWEBENGINEPROCESS_PATH="$app_dir/libexec/QtWebEngineProcess"
    export QTWEBENGINE_RESOURCES_PATH="$app_dir/resources"
    export QTWEBENGINE_LOCALES_PATH="$app_dir/translations/qtwebengine_locales"
fi
exec "$app_dir/MemoChatQml" "$@"
EOF
chmod 0755 "$stage/MemoChat"

declare -A audited_elfs=()

copy_dependency() {
    local dependency="$1"
    local destination="$stage/lib/$(basename "$dependency")"

    is_allowed_dependency_source "$dependency" ||
        die "runtime dependency resolved outside the allowlisted roots: $dependency"
    if has_native_live2d_dependency "$dependency" ||
        [[ "$(basename "$dependency")" == libLive2DCubismCore.so* ]]; then
        die "native Live2D entered the dependency closure: $dependency"
    fi
    if [[ -e "$destination" ]]; then
        if ! cmp -s "$dependency" "$destination"; then
            die "runtime library basename collision: $dependency and $destination"
        fi
        return
    fi
    install -m 0644 "$dependency" "$destination"
}

is_allowed_dependency_source() {
    local dependency
    dependency="$(realpath -e -- "$1")" || return 1
    if is_system_library "$dependency"; then
        return 0
    fi
    case "$dependency" in
        "$qt_root/lib/"* | "$stage/lib/"*)
            return 0
            ;;
    esac
    local allowed_root
    for allowed_root in "${library_dirs[@]}"; do
        case "$dependency" in
            "$allowed_root/"*) return 0 ;;
        esac
    done
    return 1
}

is_allowed_packaged_dependency_resolution() {
    local dependency
    dependency="$(realpath -e -- "$1")" || return 1
    if is_system_library "$dependency"; then
        return 0
    fi
    case "$dependency" in
        "$stage/lib/"*) return 0 ;;
        *) return 1 ;;
    esac
}

clean_ldd() {
    local search_path="$1"
    local candidate="$2"
    /usr/bin/env -i \
        PATH=/usr/bin:/bin \
        LC_ALL=C \
        LANG=C \
        LD_LIBRARY_PATH="$search_path" \
        /usr/bin/ldd "$candidate"
}

while :; do
    copied_before="$(find "$stage/lib" -maxdepth 1 -type f | wc -l)"
    while IFS= read -r -d '' candidate; do
        is_dynamic_elf "$candidate" || continue
        candidate_key="$(realpath "$candidate")"
        [[ -z "${audited_elfs[$candidate_key]:-}" ]] || continue

        ldd_output="$(clean_ldd "$dependency_search_path" "$candidate" 2>&1)" ||
            die "ldd failed for $candidate: $ldd_output"
        if grep -Fq 'not found' <<<"$ldd_output"; then
            die "unresolved runtime dependency in $candidate: $(grep -F 'not found' <<<"$ldd_output")"
        fi

        while IFS= read -r dependency; do
            [[ -n "$dependency" && -f "$dependency" ]] || continue
            if is_qt_library "$dependency" || is_compiler_runtime "$dependency" ||
                ! is_system_library "$dependency"; then
                copy_dependency "$dependency"
            fi
        done < <(
            awk '
                /=> \/[^ ]+/ { print $3 }
                /^[[:space:]]*\// { print $1 }
            ' <<<"$ldd_output"
        )
        audited_elfs[$candidate_key]=1
    done < <(find "$stage" -type f -print0)

    copied_after="$(find "$stage/lib" -maxdepth 1 -type f | wc -l)"
    [[ "$copied_after" == "$copied_before" ]] && break
done

while IFS= read -r -d '' candidate; do
    is_dynamic_elf "$candidate" || continue
    strip --strip-unneeded "$candidate"
    relative_lib="$(realpath --relative-to="$(dirname "$candidate")" "$stage/lib")"
    if [[ "$relative_lib" == "." ]]; then
        new_rpath='$ORIGIN'
    else
        new_rpath="\$ORIGIN/$relative_lib"
    fi
    patchelf --remove-rpath "$candidate"
    patchelf --set-rpath "$new_rpath" "$candidate"
done < <(find "$stage" -type f -print0)

has_release_policy_marker "$stage/MemoChatQml" ||
    die "release policy marker was lost while stripping the client executable"

while IFS= read -r -d '' candidate; do
    is_dynamic_elf "$candidate" || continue
    dynamic_section="$(readelf -d "$candidate" 2>/dev/null)"
    if grep -E '(RPATH|RUNPATH)' <<<"$dynamic_section" | is_developer_path; then
        die "developer RPATH remains in packaged ELF: $candidate"
    fi
    if grep -E '(RPATH|RUNPATH)' <<<"$dynamic_section" | grep -qv '\$ORIGIN'; then
        die "non-relative RPATH remains in packaged ELF: $candidate"
    fi
    if grep -E '(RPATH|RUNPATH)' <<<"$dynamic_section" | grep -Fq '\$ORIGIN'; then
        die "escaped ORIGIN token remains in packaged ELF: $candidate"
    fi

    ldd_output="$(clean_ldd "$stage/lib" "$candidate" 2>&1)" ||
        die "packaged dependency audit failed for $candidate: $ldd_output"
    if grep -Fq 'not found' <<<"$ldd_output"; then
        die "packaged dependency is not found for $candidate: $(grep -F 'not found' <<<"$ldd_output")"
    fi
    while IFS= read -r dependency; do
        [[ -n "$dependency" ]] || continue
        is_allowed_packaged_dependency_resolution "$dependency" ||
            die "packaged dependency resolves outside the package: $candidate -> $dependency"
    done < <(
        awk '
            /=> \/[^ ]+/ { print $3 }
            /^[[:space:]]*\// { print $1 }
        ' <<<"$ldd_output"
    )
done < <(find "$stage" -type f -print0)

while IFS= read -r -d '' candidate; do
    if contains_developer_path "$candidate"; then
        die "packaged file contains an absolute developer path: $candidate"
    fi
    if grep -aEiq 'KafuuChino|Kafuuchino-voice|resources/live2d/KafuuChino|src/KafuuChino' "$candidate"; then
        die "restricted local Live2D/model/voice asset marker entered the package: $candidate"
    fi
    if is_project_owned_release_file "$candidate" "$stage" && contains_email_address "$candidate"; then
        die "email address entered the package: $candidate"
    fi
done < <(find "$stage" -type f -print0)

suspicious_file="$(find "$stage" -type f \( \
    -iname '*.log' -o -iname '*.key' -o -iname '*.pem' -o -iname '*.p12' -o -iname '*.pfx' -o \
    -iname '*credential*' -o -iname '*cookie*' -o -iname '*token*' -o \
    -iname '*password*' -o -iname '*secret*' \
    \) -print -quit)"
[[ -z "$suspicious_file" ]] || die "sensitive filename entered the package: $suspicious_file"
developer_artifact="$(find "$stage" -type f \( \
    -name '*.o' -o -name '*.a' -o -name '*.prl' -o -name 'CMakeCache.txt' -o -name 'build.ninja' \
    \) -print -quit)"
[[ -z "$developer_artifact" ]] || die "Qt development artifact entered the package: $developer_artifact"
if grep -RIlE -- '-----BEGIN ([A-Z ]+ )?PRIVATE KEY-----' "$stage" 2>/dev/null | grep -q .; then
    die "BEGIN PRIVATE KEY material entered the package"
fi

minimum_glibc="$({
    while IFS= read -r -d '' candidate; do
        is_dynamic_elf "$candidate" || continue
        readelf --version-info "$candidate" 2>/dev/null || true
    done < <(find "$stage" -type f -print0)
} | sed -n 's/.*Name: \(GLIBC_[0-9.]*\).*/\1/p' | sort -Vu | tail -n 1)"
[[ "$minimum_glibc" =~ ^GLIBC_[0-9]+\.[0-9]+$ ]] || die "could not determine the packaged glibc requirement"

cat >"$stage/RELEASE-INFO.txt" <<EOF
Artifact: $artifact_name
Architecture: x86_64 Linux
Minimum glibc: $minimum_glibc
Client entrypoint: ./MemoChat
Public configuration: ./config.ini
Legal notices: ./legal/
Legal inventory: $legal_inventory
Third-party legal corpus: $third_party_legal_corpus
Legal corpus review: $corpus_review_id
Legal corpus SHA-256: $corpus_sha256
Release source SHA: $release_source_sha
Legal status SHA-256: $legal_status_sha256
Formal distribution ready: $formal_distribution_ready
Deployment trust anchor: $deployment_trust_anchor
Native Live2D SDK: not included
Build preset: linux-client-release-gcc16
EOF

if [[ -f "$stage/qml/QtWebEngine/ControlsDelegates/AuthenticationDialog.qml" ]]; then
    "$RELEASE_VERIFIER" --allow-file "$CLIENT_SCAN_ALLOWLIST" "$stage"
else
    "$RELEASE_VERIFIER" "$stage"
fi

(
    cd "$stage"
    find . -type f ! -name MANIFEST.sha256 -print0 |
        sort -z |
        xargs -0 sha256sum >MANIFEST.sha256
    sha256sum --check MANIFEST.sha256 >/dev/null
)

archive="$work_root/$artifact_name.tar.gz"
tar --sort=name \
    --mtime='UTC 1970-01-01' \
    --owner=0 --group=0 --numeric-owner \
    -czf "$archive" \
    -C "$work_root" "$artifact_name"

mv "$stage" "$final_dir"
mv "$archive" "$final_tar"
(
    cd "$output_dir"
    sha256sum "$(basename "$final_tar")" >"$(basename "$final_checksum")"
)

printf 'Portable directory: %s\n' "$final_dir"
printf 'Archive: %s\n' "$final_tar"
printf 'Archive checksum: %s\n' "$final_checksum"
