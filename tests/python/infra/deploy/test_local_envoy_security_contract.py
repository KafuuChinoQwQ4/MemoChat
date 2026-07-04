import unittest
from pathlib import Path

import yaml

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
LOCAL_COMPOSE = REPO_ROOT / "infra/deploy/local/docker-compose.yml"
LOCAL_ENVOY = REPO_ROOT / "infra/deploy/local/compose/envoy.yaml"
LOCAL_README = REPO_ROOT / "infra/deploy/local/README.md"
PROMETHEUS_CONFIG = REPO_ROOT / "infra/deploy/local/observability/prometheus/prometheus.yml"
K8S_ENVOY = REPO_ROOT / "infra/deploy/kubernetes/charts/memochat/templates/lb/envoy.yaml"


def load_envoy() -> dict:
    return yaml.safe_load(LOCAL_ENVOY.read_text(encoding="utf-8"))


def listener(config: dict, name: str) -> dict:
    for item in config["static_resources"]["listeners"]:
        if item["name"] == name:
            return item
    raise AssertionError(f"missing listener {name}")


def route_config_for_listener(config: dict, name: str) -> dict:
    filters = listener(config, name)["filter_chains"][0]["filters"]
    hcm = next(item for item in filters if item["name"] == "envoy.filters.network.http_connection_manager")
    return hcm["typed_config"]["route_config"]


def virtual_host(config: dict, listener_name: str, host_name: str) -> dict:
    for host in route_config_for_listener(config, listener_name)["virtual_hosts"]:
        if host["name"] == host_name:
            return host
    raise AssertionError(f"missing virtual host {host_name} in {listener_name}")


def response_header_values(host: dict) -> dict[str, str]:
    values = {}
    for item in host.get("response_headers_to_add", []):
        header = item.get("header", {})
        values[header.get("key", "")] = header.get("value", "")
    return values


class LocalEnvoySecurityContractTests(unittest.TestCase):
    def test_envoy_admin_is_loopback_only_and_not_exposed_as_a_named_container_port(self):
        config = load_envoy()
        admin_socket = config["admin"]["address"]["socket_address"]
        self.assertEqual(admin_socket["address"], "127.0.0.1")
        self.assertEqual(admin_socket["port_value"], 9901)

        compose = yaml.safe_load(LOCAL_COMPOSE.read_text(encoding="utf-8"))
        gateway_ports = compose["services"]["memochat-envoy-gateway"]["ports"]
        for port in gateway_ports:
            with self.subTest(port=port):
                self.assertNotIn("9901", str(port))

        k8s_template = K8S_ENVOY.read_text(encoding="utf-8")
        self.assertIn("address: 127.0.0.1", k8s_template)
        self.assertIn("port_value: 9901", k8s_template)
        self.assertIn("host: 127.0.0.1", k8s_template)
        self.assertIn("path: /ready", k8s_template)
        self.assertIn("port: 9901", k8s_template)
        self.assertNotIn("containerPort: 9901", k8s_template)
        self.assertNotIn("port: admin", k8s_template)

    def test_envoy_admin_docs_and_prometheus_do_not_scrape_shared_network_admin(self):
        readme = LOCAL_README.read_text(encoding="utf-8")
        prometheus = PROMETHEUS_CONFIG.read_text(encoding="utf-8")

        self.assertNotIn("http://memochat-envoy-gateway:9901", readme)
        self.assertNotIn("docker run --rm --network memochat_default", readme)
        self.assertNotIn("memochat-envoy-gateway:9901", prometheus)
        self.assertNotIn("job_name: envoy_gateway", prometheus)
        self.assertNotIn("envoy_server_live", readme)
        self.assertIn("docker exec memochat-envoy-gateway", readme)
        self.assertIn("127.0.0.1:9901", readme)

    def test_kubernetes_envoy_loadbalancer_requires_tls_for_public_ports(self):
        k8s_template = K8S_ENVOY.read_text(encoding="utf-8")
        service_section = k8s_template[k8s_template.index("kind: Service") :]

        self.assertIn(
            'required "envoy.tls.secretName is required for the production Envoy LoadBalancer"',
            k8s_template,
        )
        self.assertIn("secretName: {{ $envoyTlsSecretName | quote }}", k8s_template)
        self.assertIn("mountPath: /etc/envoy/tls", k8s_template)

        self.assertIn("- name: listener_https", k8s_template)
        self.assertIn("port_value: 8443", k8s_template)
        self.assertIn(
            '"@type": type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.DownstreamTlsContext', k8s_template
        )
        self.assertIn("filename: /etc/envoy/tls/tls.crt", k8s_template)
        self.assertIn("filename: /etc/envoy/tls/tls.key", k8s_template)

        for name in ("listener_https", "listener_chat_8090", "listener_chat_8091"):
            with self.subTest(listener=name):
                listener_block = k8s_template[k8s_template.index(f"- name: {name}") :]
                listener_block = listener_block[: listener_block.find("\n        - name:", 1)]
                self.assertIn("envoy.transport_sockets.tls", listener_block)
                self.assertIn("DownstreamTlsContext", listener_block)

        self.assertIn("- name: https", service_section)
        self.assertIn("port: 443", service_section)
        self.assertIn("targetPort: 8443", service_section)
        self.assertNotRegex(service_section, r"(?m)^\s+port:\s+80\s*$")
        self.assertNotRegex(service_section, r"(?m)^\s+targetPort:\s+80\s*$")

    def test_http_keeps_health_and_probe_routes_before_https_redirect(self):
        config = load_envoy()
        host = virtual_host(config, "listener_http", "local_service")
        routes = host["routes"]
        route_names = [route["name"] for route in routes]

        self.assertLess(route_names.index("health"), route_names.index("http_to_https"))
        self.assertLess(route_names.index("gateway_probe"), route_names.index("http_to_https"))
        self.assertLess(route_names.index("gateway_failover_probe"), route_names.index("http_to_https"))
        self.assertLess(route_names.index("http_to_https"), route_names.index("auth"))

        redirect = routes[route_names.index("http_to_https")]
        self.assertEqual(redirect["match"], {"prefix": "/"})
        self.assertEqual(redirect["redirect"]["scheme_redirect"], "https")
        self.assertEqual(redirect["redirect"]["port_redirect"], 8443)
        self.assertEqual(redirect["redirect"]["response_code"], "PERMANENT_REDIRECT")

    def test_tls_and_h3_emit_hsts_and_existing_security_headers(self):
        config = load_envoy()
        for listener_name, host_name in (
            ("listener_tls", "local_service_https"),
            ("listener_h3", "local_service_h3"),
        ):
            with self.subTest(listener=listener_name):
                headers = response_header_values(virtual_host(config, listener_name, host_name))
                self.assertEqual(headers["Strict-Transport-Security"], "max-age=31536000; includeSubDomains")
                self.assertEqual(headers["X-Content-Type-Options"], "nosniff")
                self.assertEqual(headers["X-Frame-Options"], "SAMEORIGIN")
                self.assertEqual(headers["Referrer-Policy"], "no-referrer")
                self.assertIn("geolocation=()", headers["Permissions-Policy"])

        tls_headers = response_header_values(virtual_host(config, "listener_tls", "local_service_https"))
        self.assertIn('h3=":8443"', tls_headers["alt-svc"])

    def test_auth_routes_keep_envoy_local_rate_limit_on_secure_listeners(self):
        config = load_envoy()
        for listener_name, host_name, prefix in (
            ("listener_tls", "local_service_https", "auth_local_ratelimiter_https"),
            ("listener_h3", "local_service_h3", "auth_local_ratelimiter_h3"),
        ):
            with self.subTest(listener=listener_name):
                host = virtual_host(config, listener_name, host_name)
                auth_routes = [
                    route
                    for route in host["routes"]
                    if route["name"] == "auth"
                    and route["match"].get("path")
                    in {"/user_login", "/auth/refresh", "/auth/logout", "/get_varifycode"}
                ]
                self.assertGreaterEqual(len(auth_routes), 3)
                for route in auth_routes:
                    limiter = route["typed_per_filter_config"]["envoy.filters.http.local_ratelimit"]
                    self.assertEqual(limiter["stat_prefix"], prefix)
                    self.assertEqual(limiter["token_bucket"]["max_tokens"], 10)

    def test_refresh_and_logout_routes_use_login_backend_on_secure_listeners_and_k8s_template(self):
        config = load_envoy()
        for listener_name, host_name, path in (
            ("listener_tls", "local_service_https", "/auth/refresh"),
            ("listener_tls", "local_service_https", "/auth/logout"),
            ("listener_h3", "local_service_h3", "/auth/refresh"),
            ("listener_h3", "local_service_h3", "/auth/logout"),
        ):
            with self.subTest(listener=listener_name, path=path):
                host = virtual_host(config, listener_name, host_name)
                auth_routes = [
                    route for route in host["routes"] if route["name"] == "auth" and route["match"].get("path") == path
                ]
                self.assertEqual(len(auth_routes), 1)
                self.assertEqual(auth_routes[0]["route"]["cluster"], "login_backend")
                self.assertEqual(auth_routes[0]["match"]["headers"][0]["exact_match"], "POST")

        k8s_template = K8S_ENVOY.read_text(encoding="utf-8")
        self.assertIn("path: /auth/refresh", k8s_template)
        self.assertIn("path: /auth/logout", k8s_template)
        self.assertIn("cluster: login_backend", k8s_template)
