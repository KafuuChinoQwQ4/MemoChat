const Redis = require('ioredis');

const config_module = require('./config');
const { childSpan, exportZipkinSpan } = require('./telemetry');
const { logger } = require('./logger');

const RedisCli = new Redis({
  host: config_module.redis_host,
  port: config_module.redis_port,
  password: config_module.redis_passwd,
  enableOfflineQueue: false,
  enableReadyCheck: true,
  maxRetriesPerRequest: 2,
  retryStrategy(times) {
    return Math.min(times * 200, 3000);
  },
});

RedisCli.on('error', (err) => {
  logger.error({ event: 'redis.error', error: err.message || String(err) }, 'redis connection error');
});

RedisCli.on('end', () => {
  logger.warn({ event: 'redis.closed' }, 'redis connection closed');
});

const heartbeatTimer = setInterval(() => {
  RedisCli.set('heartbeat', Date.now()).catch((error) => {
    logger.warn({ event: 'redis.heartbeat.failed', error: error.message || String(error) }, 'redis heartbeat set failed');
  });
}, 60000);
if (typeof heartbeatTimer.unref === 'function') {
  heartbeatTimer.unref();
}

async function GetRedis(key, spanContext = null) {
  const span = childSpan(spanContext, 'Redis GET', 'CLIENT', {
    'db.system': 'redis',
    'db.operation': 'GET',
    'db.statement.name': 'GET',
    key,
  });
  try {
    const result = await RedisCli.get(key);
    if (result === null) {
      logger.info({ event: 'redis.get.miss', key, trace_id: span.trace_id, request_id: span.request_id, span_id: span.span_id, module: 'redis' }, 'redis key not found');
      exportZipkinSpan(span, { 'db.redis.result': 'miss' });
      return null;
    }
    exportZipkinSpan(span, { 'db.redis.result': 'hit' });
    return result;
  } catch (error) {
    logger.error({ event: 'redis.get.error', key, error: error.message || String(error), trace_id: span.trace_id, request_id: span.request_id, span_id: span.span_id, module: 'redis', error_type: 'redis' }, 'redis get error');
    exportZipkinSpan(span, { error: error.message || String(error), 'otel.status_code': 'ERROR' });
    return null;
  }
}

async function GetRedisTTL(key) {
  try {
    return await RedisCli.ttl(key);
  } catch (error) {
    logger.error({ event: 'redis.ttl.error', key, error: error.message || String(error), module: 'redis', error_type: 'redis' }, 'redis ttl error');
    return -2;
  }
}

async function QueryRedis(key, spanContext = null) {
  const span = childSpan(spanContext, 'Redis EXISTS', 'CLIENT', {
    'db.system': 'redis',
    'db.operation': 'EXISTS',
    'db.statement.name': 'EXISTS',
    key,
  });
  try {
    const result = await RedisCli.exists(key);
    if (result === 0) {
      logger.info({ event: 'redis.exists.miss', key, trace_id: span.trace_id, request_id: span.request_id, span_id: span.span_id, module: 'redis' }, 'redis key not found by exists');
      exportZipkinSpan(span, { 'db.redis.result': 'miss' });
      return null;
    }
    exportZipkinSpan(span, { 'db.redis.result': 'hit' });
    return result;
  } catch (error) {
    logger.error({ event: 'redis.exists.error', key, error: error.message || String(error), trace_id: span.trace_id, request_id: span.request_id, span_id: span.span_id, module: 'redis', error_type: 'redis' }, 'redis exists error');
    exportZipkinSpan(span, { error: error.message || String(error), 'otel.status_code': 'ERROR' });
    return null;
  }
}

async function SetRedisExpire(key, value, exptime, spanContext = null) {
  const span = childSpan(spanContext, 'Redis SETEX', 'CLIENT', {
    'db.system': 'redis',
    'db.operation': 'SETEX',
    'db.statement.name': 'SETEX',
    key,
  });
  try {
    await RedisCli.set(key, value, 'EX', exptime);
    exportZipkinSpan(span, { exptime });
    return true;
  } catch (error) {
    logger.error(
      { event: 'redis.set_expire.error', key, exptime, error: error.message || String(error), trace_id: span.trace_id, request_id: span.request_id, span_id: span.span_id, module: 'redis', error_type: 'redis' },
      'redis set+expire error'
    );
    exportZipkinSpan(span, { error: error.message || String(error), exptime, 'otel.status_code': 'ERROR' });
    return false;
  }
}

async function closeRedis() {
  clearInterval(heartbeatTimer);
  try {
    await RedisCli.quit();
  } catch (error) {
    logger.warn({ event: 'redis.quit.failed', error: error.message || String(error) }, 'redis quit failed, disconnecting');
    RedisCli.disconnect();
  }
}

module.exports = {
  GetRedis,
  GetRedisTTL,
  QueryRedis,
  SetRedisExpire,
  closeRedis,
};
