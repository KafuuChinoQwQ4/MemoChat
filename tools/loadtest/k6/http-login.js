import http from "k6/http";
import { check } from "k6";
import { SharedArray } from "k6/data";
import exec from "k6/execution";

const gateUrls = (__ENV.GATE_URLS || __ENV.GATE_URL || "http://127.0.0.1:8080")
  .split(",")
  .map((value) => value.trim().replace(/\/+$/, ""))
  .filter((value) => value.length > 0);
const loginPath = __ENV.LOGIN_PATH || "/user_login";
const clientVer = __ENV.CLIENT_VER || "3.0.0";
const scenarioName = __ENV.SCENARIO || "login";
const totalRequests = Number.parseInt(__ENV.TOTAL || "100", 10);
const concurrency = Number.parseInt(__ENV.CONCURRENCY || "10", 10);
const useXorPassword = (__ENV.USE_XOR_PASSWD || "true").toLowerCase() !== "false";
const summaryPath = __ENV.SUMMARY_PATH || "k6_summary.json";

const accounts = new SharedArray("accounts", () => {
  const csv = open(__ENV.ACCOUNTS_CSV || "../python-loadtest/accounts.local.csv");
  const lines = csv.split(/\r?\n/).filter((line) => line.trim().length > 0);
  const headers = lines.shift().split(",").map((header) => header.trim());
  const emailIndex = headers.indexOf("email");
  const passwordIndex = headers.indexOf("password");
  const lastPasswordIndex = headers.indexOf("last_password");
  return lines
    .map((line) => line.split(","))
    .map((cols) => ({
      email: (cols[emailIndex] || "").trim(),
      password: ((lastPasswordIndex >= 0 && cols[lastPasswordIndex]) || cols[passwordIndex] || "").trim(),
    }))
    .filter((account) => account.email && account.password);
});

export const options = {
  summaryTrendStats: ["min", "avg", "med", "p(75)", "p(90)", "p(95)", "p(99)", "max"],
  scenarios: {
    fixed_iterations: {
      executor: "shared-iterations",
      vus: concurrency,
      iterations: totalRequests,
      maxDuration: __ENV.MAX_DURATION || "5m",
    },
  },
  thresholds: {
    http_req_failed: ["rate<0.05"],
  },
};

function xorEncode(raw) {
  const key = raw.length % 255;
  let out = "";
  for (let i = 0; i < raw.length; i += 1) {
    out += String.fromCharCode(raw.charCodeAt(i) ^ key);
  }
  return out;
}

function gateUrlFor(iteration) {
  return gateUrls[iteration % gateUrls.length];
}

export default function () {
  const iteration = exec.scenario.iterationInTest;
  if (scenarioName === "health") {
    const url = `${gateUrlFor(iteration)}/healthz`;
    const response = http.get(url, {
      tags: { scenario_name: "health", route: "/healthz" },
      timeout: __ENV.HTTP_TIMEOUT || "8s",
    });
    check(response, {
      "health status is 200": (r) => r.status === 200,
    });
    return;
  }

  const account = accounts[iteration % accounts.length];
  const password = useXorPassword ? xorEncode(account.password) : account.password;
  const payload = JSON.stringify({
    email: account.email,
    passwd: password,
    client_ver: clientVer,
  });
  const response = http.post(`${gateUrlFor(iteration)}${loginPath}`, payload, {
    headers: {
      "Content-Type": "application/json",
      "X-Trace-Id": `${__VU}-${iteration}`,
    },
    tags: { scenario_name: "login", route: loginPath },
    timeout: __ENV.HTTP_TIMEOUT || "8s",
  });
  check(response, {
    "login status is 200": (r) => r.status === 200,
    "login business success": (r) => {
      try {
        return r.json("error") === 0;
      } catch (_) {
        return false;
      }
    },
  });
}

function metricValue(data, name, field, fallback = 0) {
  const metric = data.metrics[name];
  if (!metric || !metric.values) {
    return fallback;
  }
  return Object.prototype.hasOwnProperty.call(metric.values, field) ? metric.values[field] : fallback;
}

export function handleSummary(data) {
  const checks = metricValue(data, "checks", "passes", 0);
  const checkFails = metricValue(data, "checks", "fails", 0);
  const total = checks + checkFails > 0 && scenarioName === "login" ? Math.floor((checks + checkFails) / 2) : totalRequests;
  const failedRate = metricValue(data, "http_req_failed", "rate", 0);
  const failedByHttp = Math.round(total * failedRate);
  const failedByChecks = scenarioName === "login" ? Math.ceil(checkFails / 2) : checkFails;
  const failed = Math.min(total, Math.max(failedByHttp, failedByChecks));
  const success = Math.max(0, total - failed);
  const durationMs = data.state && data.state.testRunDurationMs
    ? data.state.testRunDurationMs
    : metricValue(data, "iteration_duration", "avg", 0) * total / Math.max(1, concurrency);
  const durationSec = Math.max(0.001, durationMs / 1000);
  const report = {
    scenario: scenarioName === "health" ? "http_health_k6" : "http_login_k6",
    started_at: new Date().toISOString(),
    finished_at: new Date().toISOString(),
    summary: {
      total,
      success,
      failed,
      success_rate: total > 0 ? Number((success / total).toFixed(4)) : 0,
      wall_elapsed_sec: Number(durationSec.toFixed(4)),
      throughput_rps: Number(metricValue(data, "http_reqs", "rate", 0).toFixed(3)),
      latency_ms: {
        min_ms: Number(metricValue(data, "http_req_duration", "min", 0).toFixed(3)),
        avg_ms: Number(metricValue(data, "http_req_duration", "avg", 0).toFixed(3)),
        p50_ms: Number(metricValue(data, "http_req_duration", "med", 0).toFixed(3)),
        p75_ms: Number(metricValue(data, "http_req_duration", "p(75)", 0).toFixed(3)),
        p90_ms: Number(metricValue(data, "http_req_duration", "p(90)", 0).toFixed(3)),
        p95_ms: Number(metricValue(data, "http_req_duration", "p(95)", 0).toFixed(3)),
        p99_ms: Number(metricValue(data, "http_req_duration", "p(99)", 0).toFixed(3)),
        max_ms: Number(metricValue(data, "http_req_duration", "max", 0).toFixed(3)),
      },
    },
    errors: {
      http_req_failed_rate: failedRate,
      check_fails: checkFails,
    },
    quality: {},
    details: {
      runner: "k6",
      gate_urls: gateUrls,
      login_path: loginPath,
      concurrency,
      total_requested: totalRequests,
      scenario_name: scenarioName,
      metrics: data.metrics,
    },
  };
  return {
    stdout: JSON.stringify(report.summary, null, 2),
    [summaryPath]: JSON.stringify(report, null, 2),
  };
}
