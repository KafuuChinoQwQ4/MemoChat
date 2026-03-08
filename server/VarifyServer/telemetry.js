const crypto = require('crypto');
const http = require('http');
const https = require('https');

const config = require('./config');

function randomHex(size) {
  return crypto.randomBytes(size).toString('hex');
}

function newTraceId() {
  return randomHex(16);
}

function newRequestId() {
  return newTraceId();
}

function newSpanId() {
  return randomHex(8);
}

function metadataValue(metadata, key) {
  if (!metadata || typeof metadata.get !== 'function') {
    return '';
  }
  const values = metadata.get(key);
  if (!values || values.length === 0 || !values[0]) {
    return '';
  }
  return String(values[0]).trim();
}

function startSpan({ name, kind = 'SERVER', trace_id = '', request_id = '', parent_span_id = '', attrs = {} }) {
  return {
    name,
    kind,
    trace_id: trace_id || newTraceId(),
    request_id: request_id || newRequestId(),
    parent_span_id: parent_span_id || '',
    span_id: newSpanId(),
    start_ms: Date.now(),
    attrs: { ...attrs },
  };
}

function startServerSpan(name, call, attrs = {}) {
  const metadata = call && call.metadata;
  return startSpan({
    name,
    kind: 'SERVER',
    trace_id: metadataValue(metadata, 'x-trace-id'),
    request_id: metadataValue(metadata, 'x-request-id'),
    parent_span_id: metadataValue(metadata, 'x-span-id'),
    attrs,
  });
}

function childSpan(parent, name, kind = 'CLIENT', attrs = {}) {
  return startSpan({
    name,
    kind,
    trace_id: parent && parent.trace_id ? parent.trace_id : '',
    request_id: parent && parent.request_id ? parent.request_id : '',
    parent_span_id: parent && parent.span_id ? parent.span_id : '',
    attrs,
  });
}

function exportZipkinSpan(span, extraAttrs = {}) {
  if (!config.telemetry_enabled || !config.telemetry_export_traces || !config.telemetry_endpoint || !span || !span.trace_id || !span.span_id) {
    return;
  }

  const endpoint = new URL(config.telemetry_endpoint);
  const body = JSON.stringify([
    {
      traceId: span.trace_id,
      id: span.span_id,
      parentId: span.parent_span_id || undefined,
      name: span.name,
      kind: span.kind,
      timestamp: String(span.start_ms * 1000),
      duration: String(Math.max(0, Date.now() - span.start_ms) * 1000),
      localEndpoint: {
        serviceName: config.telemetry_service_name || 'VarifyServer',
      },
      tags: {
        'service.instance.id': config.service_instance,
        'service.namespace': config.telemetry_service_namespace || 'memochat',
        ...span.attrs,
        ...extraAttrs,
      },
    },
  ]);

  const client = endpoint.protocol === 'https:' ? https : http;
  const req = client.request(
    endpoint,
    {
      method: 'POST',
      headers: {
        'content-type': 'application/json',
        'content-length': Buffer.byteLength(body),
      },
      timeout: 1000,
    },
    (res) => {
      res.resume();
    }
  );
  req.on('error', () => {});
  req.on('timeout', () => req.destroy());
  req.write(body);
  req.end();
}

module.exports = {
  childSpan,
  exportZipkinSpan,
  newRequestId,
  newSpanId,
  newTraceId,
  startServerSpan,
  startSpan,
};
