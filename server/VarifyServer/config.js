const fs = require('fs');
const path = require('path');
const os = require('os');

const configPath = path.join(__dirname, 'config.json');
const config = JSON.parse(fs.readFileSync(configPath, 'utf8'));

function envString(name, fallback) {
  const value = process.env[name];
  return value !== undefined && value !== '' ? value : fallback;
}

function envNumber(name, fallback) {
  const value = process.env[name];
  if (value === undefined || value === '') {
    return fallback;
  }
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function envBool(name, fallback) {
  const value = process.env[name];
  if (value === undefined || value === '') {
    return fallback;
  }
  return ['1', 'true', 'yes', 'on'].includes(String(value).toLowerCase());
}

const email_host = envString('MEMOCHAT_EMAIL_HOST', config.email.host || 'smtp.163.com');
const email_port = envNumber('MEMOCHAT_EMAIL_PORT', config.email.port || 465);
const email_secure = envBool('MEMOCHAT_EMAIL_SECURE', typeof config.email.secure === 'boolean' ? config.email.secure : true);
const email_user = envString('MEMOCHAT_EMAIL_USER', config.email.user);
const email_pass = envString('MEMOCHAT_EMAIL_PASS', config.email.pass);
const email_from = envString('MEMOCHAT_EMAIL_FROM', config.email.from || email_user);
const mysql_host = envString('MEMOCHAT_MYSQL_HOST', config.mysql.host);
const mysql_port = envNumber('MEMOCHAT_MYSQL_PORT', config.mysql.port);
const mysql_passwd = envString('MEMOCHAT_MYSQL_PASSWORD', config.mysql.passwd);
const redis_host = envString('MEMOCHAT_REDIS_HOST', config.redis.host);
const redis_port = envNumber('MEMOCHAT_REDIS_PORT', config.redis.port);
const redis_passwd = envString('MEMOCHAT_REDIS_PASSWD', config.redis.passwd);
const code_prefix = 'code_';
const rabbitmq_host = envString('MEMOCHAT_RABBITMQ_HOST', config.rabbitmq ? config.rabbitmq.host || '127.0.0.1' : '127.0.0.1');
const rabbitmq_port = envNumber('MEMOCHAT_RABBITMQ_PORT', config.rabbitmq ? config.rabbitmq.port || 5672 : 5672);
const rabbitmq_username = envString('MEMOCHAT_RABBITMQ_USERNAME', config.rabbitmq ? config.rabbitmq.username || 'guest' : 'guest');
const rabbitmq_password = envString('MEMOCHAT_RABBITMQ_PASSWORD', config.rabbitmq ? config.rabbitmq.password || 'guest' : 'guest');
const rabbitmq_vhost = envString('MEMOCHAT_RABBITMQ_VHOST', config.rabbitmq ? config.rabbitmq.vhost || '/' : '/');
const rabbitmq_prefetch_count = envNumber('MEMOCHAT_RABBITMQ_PREFETCHCOUNT', config.rabbitmq ? Number(config.rabbitmq.prefetchCount || 8) : 8);
const rabbitmq_exchange_direct = envString('MEMOCHAT_RABBITMQ_EXCHANGEDIRECT', config.rabbitmq ? config.rabbitmq.exchangeDirect || 'memochat.direct' : 'memochat.direct');
const rabbitmq_exchange_dlx = envString('MEMOCHAT_RABBITMQ_EXCHANGEDLX', config.rabbitmq ? config.rabbitmq.exchangeDlx || 'memochat.dlx' : 'memochat.dlx');
const rabbitmq_verify_delivery_routing_key = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_VERIFYDELIVERYROUTINGKEY', config.rabbitmq.verifyDeliveryRoutingKey || 'verify.email.delivery')
  : 'verify.email.delivery';
const rabbitmq_retry_routing_key = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_RETRYROUTINGKEY', config.rabbitmq.retryRoutingKey || 'verify.email.delivery.retry')
  : 'verify.email.delivery.retry';
const rabbitmq_dlq_routing_key = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_DLQROUTINGKEY', config.rabbitmq.dlqRoutingKey || 'verify.email.delivery.dlq')
  : 'verify.email.delivery.dlq';
const rabbitmq_verify_delivery_queue = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_QUEUEVERIFYDELIVERY', config.rabbitmq.queueVerifyDelivery || 'verify.email.delivery.q')
  : 'verify.email.delivery.q';
const rabbitmq_verify_delivery_retry_queue = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_QUEUEVERIFYDELIVERYRETRY', config.rabbitmq.queueVerifyDeliveryRetry || 'verify.email.delivery.retry.q')
  : 'verify.email.delivery.retry.q';
const rabbitmq_verify_delivery_dlq_queue = config.rabbitmq
  ? envString('MEMOCHAT_RABBITMQ_QUEUEVERIFYDELIVERYDLQ', config.rabbitmq.queueVerifyDeliveryDlq || 'verify.email.delivery.dlq.q')
  : 'verify.email.delivery.dlq.q';
const rabbitmq_retry_delay_ms = envNumber('MEMOCHAT_RABBITMQ_RETRYDELAYMS', config.rabbitmq ? Number(config.rabbitmq.retryDelayMs || 5000) : 5000);
const rabbitmq_max_retries = envNumber('MEMOCHAT_RABBITMQ_MAXRETRIES', config.rabbitmq ? Number(config.rabbitmq.maxRetries || 5) : 5);

const log_level = envString('MEMOCHAT_LOG_LEVEL', (config.log && config.log.level) || 'info');
const log_dir = envString('MEMOCHAT_LOG_DIR', (config.log && config.log.dir) || './logs');
const log_to_console = envBool('MEMOCHAT_LOG_TOCONSOLE', config.log ? config.log.toConsole !== false : true);
const log_env = envString('MEMOCHAT_LOG_ENV', (config.log && config.log.env) || 'local');
const telemetry_enabled = envBool('MEMOCHAT_TELEMETRY_ENABLED', config.telemetry ? config.telemetry.enabled !== false : true);
const telemetry_endpoint = envString('MEMOCHAT_TELEMETRY_OTLPENDPOINT', (config.telemetry && config.telemetry.otlpEndpoint) || 'http://127.0.0.1:9411/api/v2/spans');
const telemetry_protocol = envString('MEMOCHAT_TELEMETRY_PROTOCOL', (config.telemetry && config.telemetry.protocol) || 'zipkin-json');
const telemetry_sample_ratio = process.env.MEMOCHAT_TELEMETRY_SAMPLERATIO !== undefined
  ? envNumber('MEMOCHAT_TELEMETRY_SAMPLERATIO', 1.0)
  : (config.telemetry && config.telemetry.sampleRatio !== undefined
  ? Number(config.telemetry.sampleRatio)
  : 1.0);
const telemetry_export_logs = envBool('MEMOCHAT_TELEMETRY_EXPORTLOGS', config.telemetry ? config.telemetry.exportLogs !== false : true);
const telemetry_export_traces = envBool('MEMOCHAT_TELEMETRY_EXPORTTRACES', config.telemetry ? config.telemetry.exportTraces !== false : true);
const telemetry_export_metrics = envBool('MEMOCHAT_TELEMETRY_EXPORTMETRICS', config.telemetry ? config.telemetry.exportMetrics === true : false);
const telemetry_service_name = envString('MEMOCHAT_TELEMETRY_SERVICENAME', (config.telemetry && config.telemetry.serviceName) || 'VarifyServer');
const telemetry_service_namespace = envString('MEMOCHAT_TELEMETRY_SERVICENAMESPACE', (config.telemetry && config.telemetry.serviceNamespace) || 'memochat');
const service_instance = `${telemetry_service_name}@${os.hostname()}:${process.pid}`;
const verify_async_outbox = envBool('MEMOCHAT_FEATUREFLAGS_VERIFY_ASYNC_OUTBOX', config.featureFlags ? config.featureFlags.verify_async_outbox === true : false);
const grpc_host = envString('MEMOCHAT_VARIFYSERVER_HOST', '0.0.0.0');
const grpc_port = envNumber('MEMOCHAT_VARIFYSERVER_PORT', 50051);
const health_port = envNumber('MEMOCHAT_HEALTH_PORT', 8081);

module.exports = {
  email_host,
  email_port,
  email_secure,
  email_user,
  email_pass,
  email_from,
  mysql_host,
  mysql_port,
  mysql_passwd,
  redis_host,
  redis_port,
  redis_passwd,
  code_prefix,
  rabbitmq_host,
  rabbitmq_port,
  rabbitmq_username,
  rabbitmq_password,
  rabbitmq_vhost,
  rabbitmq_prefetch_count,
  rabbitmq_exchange_direct,
  rabbitmq_exchange_dlx,
  rabbitmq_verify_delivery_routing_key,
  rabbitmq_retry_routing_key,
  rabbitmq_dlq_routing_key,
  rabbitmq_verify_delivery_queue,
  rabbitmq_verify_delivery_retry_queue,
  rabbitmq_verify_delivery_dlq_queue,
  rabbitmq_retry_delay_ms,
  rabbitmq_max_retries,
  log_level,
  log_dir,
  log_to_console,
  log_env,
  telemetry_enabled,
  telemetry_endpoint,
  telemetry_protocol,
  telemetry_sample_ratio,
  telemetry_export_logs,
  telemetry_export_traces,
  telemetry_export_metrics,
  telemetry_service_name,
  telemetry_service_namespace,
  service_instance,
  verify_async_outbox,
  grpc_host,
  grpc_port,
  health_port,
};
