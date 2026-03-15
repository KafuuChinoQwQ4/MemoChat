const fs = require('fs');
const path = require('path');
const os = require('os');

const configPath = path.join(__dirname, 'config.json');
const config = JSON.parse(fs.readFileSync(configPath, 'utf8'));

const email_host = config.email.host || 'smtp.163.com';
const email_port = config.email.port || 465;
const email_secure = typeof config.email.secure === 'boolean' ? config.email.secure : true;
const email_user = config.email.user;
const email_pass = config.email.pass;
const email_from = config.email.from || email_user;
const mysql_host = config.mysql.host;
const mysql_port = config.mysql.port;
const redis_host = config.redis.host;
const redis_port = config.redis.port;
const redis_passwd = config.redis.passwd;
const code_prefix = 'code_';
const rabbitmq_host = config.rabbitmq ? config.rabbitmq.host || '127.0.0.1' : '127.0.0.1';
const rabbitmq_port = config.rabbitmq ? config.rabbitmq.port || 5672 : 5672;
const rabbitmq_username = config.rabbitmq ? config.rabbitmq.username || 'guest' : 'guest';
const rabbitmq_password = config.rabbitmq ? config.rabbitmq.password || 'guest' : 'guest';
const rabbitmq_vhost = config.rabbitmq ? config.rabbitmq.vhost || '/' : '/';
const rabbitmq_prefetch_count = config.rabbitmq ? Number(config.rabbitmq.prefetchCount || 8) : 8;
const rabbitmq_exchange_direct = config.rabbitmq ? config.rabbitmq.exchangeDirect || 'memochat.direct' : 'memochat.direct';
const rabbitmq_exchange_dlx = config.rabbitmq ? config.rabbitmq.exchangeDlx || 'memochat.dlx' : 'memochat.dlx';
const rabbitmq_verify_delivery_routing_key = config.rabbitmq
  ? config.rabbitmq.verifyDeliveryRoutingKey || 'verify.email.delivery'
  : 'verify.email.delivery';
const rabbitmq_retry_routing_key = config.rabbitmq
  ? config.rabbitmq.retryRoutingKey || 'verify.email.delivery.retry'
  : 'verify.email.delivery.retry';
const rabbitmq_dlq_routing_key = config.rabbitmq
  ? config.rabbitmq.dlqRoutingKey || 'verify.email.delivery.dlq'
  : 'verify.email.delivery.dlq';
const rabbitmq_verify_delivery_queue = config.rabbitmq
  ? config.rabbitmq.queueVerifyDelivery || 'verify.email.delivery.q'
  : 'verify.email.delivery.q';
const rabbitmq_verify_delivery_retry_queue = config.rabbitmq
  ? config.rabbitmq.queueVerifyDeliveryRetry || 'verify.email.delivery.retry.q'
  : 'verify.email.delivery.retry.q';
const rabbitmq_verify_delivery_dlq_queue = config.rabbitmq
  ? config.rabbitmq.queueVerifyDeliveryDlq || 'verify.email.delivery.dlq.q'
  : 'verify.email.delivery.dlq.q';
const rabbitmq_retry_delay_ms = config.rabbitmq ? Number(config.rabbitmq.retryDelayMs || 5000) : 5000;
const rabbitmq_max_retries = config.rabbitmq ? Number(config.rabbitmq.maxRetries || 5) : 5;

const log_level = (config.log && config.log.level) || 'info';
const log_dir = (config.log && config.log.dir) || './logs';
const log_to_console = config.log ? config.log.toConsole !== false : true;
const log_env = (config.log && config.log.env) || 'local';
const telemetry_enabled = config.telemetry ? config.telemetry.enabled !== false : true;
const telemetry_endpoint = (config.telemetry && config.telemetry.otlpEndpoint) || 'http://127.0.0.1:9411/api/v2/spans';
const telemetry_protocol = (config.telemetry && config.telemetry.protocol) || 'zipkin-json';
const telemetry_sample_ratio = config.telemetry && config.telemetry.sampleRatio !== undefined
  ? Number(config.telemetry.sampleRatio)
  : 1.0;
const telemetry_export_logs = config.telemetry ? config.telemetry.exportLogs !== false : true;
const telemetry_export_traces = config.telemetry ? config.telemetry.exportTraces !== false : true;
const telemetry_export_metrics = config.telemetry ? config.telemetry.exportMetrics === true : false;
const telemetry_service_name = (config.telemetry && config.telemetry.serviceName) || 'VarifyServer';
const telemetry_service_namespace = (config.telemetry && config.telemetry.serviceNamespace) || 'memochat';
const service_instance = `${telemetry_service_name}@${os.hostname()}:${process.pid}`;
const verify_async_outbox = config.featureFlags ? config.featureFlags.verify_async_outbox === true : false;

module.exports = {
  email_host,
  email_port,
  email_secure,
  email_user,
  email_pass,
  email_from,
  mysql_host,
  mysql_port,
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
};
