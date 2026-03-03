const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
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
    base: {
      service: 'VarifyServer',
      env: config.log_env || 'local',
    },
    timestamp: pino.stdTimeFunctions.isoTime,
  },
  pino.multistream(streams)
);

function newTraceId() {
  if (typeof crypto.randomUUID === 'function') {
    return crypto.randomUUID();
  }
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
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
