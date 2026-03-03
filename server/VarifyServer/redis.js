const Redis = require('ioredis');

const config_module = require('./config');
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

async function GetRedis(key) {
  try {
    const result = await RedisCli.get(key);
    if (result === null) {
      logger.info({ event: 'redis.get.miss', key }, 'redis key not found');
      return null;
    }
    return result;
  } catch (error) {
    logger.error({ event: 'redis.get.error', key, error: error.message || String(error) }, 'redis get error');
    return null;
  }
}

async function QueryRedis(key) {
  try {
    const result = await RedisCli.exists(key);
    if (result === 0) {
      logger.info({ event: 'redis.exists.miss', key }, 'redis key not found by exists');
      return null;
    }
    return result;
  } catch (error) {
    logger.error({ event: 'redis.exists.error', key, error: error.message || String(error) }, 'redis exists error');
    return null;
  }
}

async function SetRedisExpire(key, value, exptime) {
  try {
    await RedisCli.set(key, value);
    await RedisCli.expire(key, exptime);
    return true;
  } catch (error) {
    logger.error(
      { event: 'redis.set_expire.error', key, exptime, error: error.message || String(error) },
      'redis set+expire error'
    );
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
  QueryRedis,
  SetRedisExpire,
  closeRedis,
};
