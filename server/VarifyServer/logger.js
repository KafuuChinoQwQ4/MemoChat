const fs = require('fs');
const path = require('path');
const pino = require('pino');

const config = require('./config');

function ensureDir(dir) {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }
}

function dateTag() {
  const d = new Date();
  const y = d.getUTCFullYear();
  const m = String(d.getUTCMonth() + 1).padStart(2, '0');
  const day = String(d.getUTCDate()).padStart(2, '0');
  return `${y}${m}${day}`;
}

const logDir = path.resolve(__dirname, config.log_dir || './logs');
ensureDir(logDir);
const filePath = path.join(logDir, `VarifyServer_${dateTag()}.json`);
const streams = [];
if (config.log_to_console !== false) {
  streams.push({ stream: process.stdout });
}
streams.push({ stream: pino.destination({ dest: filePath, sync: false }) });

const logger = pino(
  {
    level: config.log_level || 'info',
    messageKey: 'message',
    base: {
      service: config.telemetry_service_name || 'VarifyServer',
      service_instance: config.service_instance,
      env: config.log_env || 'local',
    },
    timestamp: () => `,"ts":"${new Date().toISOString()}"`,
    formatters: {
      level(label) {
        return { level: label };
      },
      log(object) {
        const attrs = {};
        const normalized = {
          event: object.event || 'varify.log',
          trace_id: object.trace_id || '',
          request_id: object.request_id || '',
          span_id: object.span_id || '',
          uid: object.uid || '',
          session_id: object.session_id || '',
          module: object.module || '',
          peer_service: object.peer_service || '',
          error_code: object.error_code || '',
          error_type: object.error_type || '',
          duration_ms: object.duration_ms || 0,
        };
        for (const [key, value] of Object.entries(object)) {
          if (Object.prototype.hasOwnProperty.call(normalized, key) || key === 'event') {
            if (key === 'event') {
              normalized.event = value;
            }
            continue;
          }
          attrs[key] = value;
        }
        normalized.attrs = attrs;
        return normalized;
      },
    },
  },
  pino.multistream(streams)
);

function newTraceId() {
  return require('./telemetry').newTraceId();
}

function redactEmail(email) {
  if (!email || typeof email !== 'string') {
    return '****';
  }
  const at = email.indexOf('@');
  if (at <= 0) {
    return '****';
  }
  const local = email.slice(0, at);
  const domain = email.slice(at);
  if (local.length <= 2) {
    return `${local.slice(0, 1)}***${domain}`;
  }
  return `${local.slice(0, 2)}***${domain}`;
}

module.exports = {
  logger,
  newTraceId,
  redactEmail,
};
