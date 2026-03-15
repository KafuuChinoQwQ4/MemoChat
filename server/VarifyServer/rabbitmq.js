const amqp = require('amqplib');
const config = require('./config');
const { logger } = require('./logger');

let connection = null;
let publishChannel = null;
let consumeChannel = null;
let workerRunning = false;

function rabbitUrl() {
  const vhost = encodeURIComponent(config.rabbitmq_vhost || '/');
  return `amqp://${encodeURIComponent(config.rabbitmq_username)}:${encodeURIComponent(config.rabbitmq_password)}@${config.rabbitmq_host}:${config.rabbitmq_port}/${vhost}`;
}

async function ensureConnected() {
  if (connection && publishChannel && consumeChannel) {
    return;
  }
  connection = await amqp.connect(rabbitUrl());
  connection.on('close', () => {
    connection = null;
    publishChannel = null;
    consumeChannel = null;
    workerRunning = false;
  });
  connection.on('error', (err) => {
    logger.error(
      { event: 'varify.rabbitmq.connection_error', error: err && err.message ? err.message : String(err), module: 'rabbitmq', error_type: 'amqp' },
      'rabbitmq connection error'
    );
  });

  publishChannel = await connection.createConfirmChannel();
  consumeChannel = await connection.createChannel();
  await publishChannel.prefetch(config.rabbitmq_prefetch_count);
  await consumeChannel.prefetch(config.rabbitmq_prefetch_count);
  await declareTopology(publishChannel);
  await declareTopology(consumeChannel);
}

async function declareTopology(channel) {
  await channel.assertExchange(config.rabbitmq_exchange_direct, 'direct', { durable: true });
  await channel.assertExchange(config.rabbitmq_exchange_dlx, 'direct', { durable: true });

  await channel.assertQueue(config.rabbitmq_verify_delivery_queue, {
    durable: true,
  });
  await channel.assertQueue(config.rabbitmq_verify_delivery_retry_queue, {
    durable: true,
    arguments: {
      'x-dead-letter-exchange': config.rabbitmq_exchange_direct,
      'x-dead-letter-routing-key': config.rabbitmq_verify_delivery_routing_key,
      'x-message-ttl': config.rabbitmq_retry_delay_ms,
    },
  });
  await channel.assertQueue(config.rabbitmq_verify_delivery_dlq_queue, {
    durable: true,
  });

  await channel.bindQueue(
    config.rabbitmq_verify_delivery_queue,
    config.rabbitmq_exchange_direct,
    config.rabbitmq_verify_delivery_routing_key
  );
  await channel.bindQueue(
    config.rabbitmq_verify_delivery_retry_queue,
    config.rabbitmq_exchange_direct,
    config.rabbitmq_retry_routing_key
  );
  await channel.bindQueue(
    config.rabbitmq_verify_delivery_dlq_queue,
    config.rabbitmq_exchange_dlx,
    config.rabbitmq_dlq_routing_key
  );
}

function buildTask(traceId, requestId, email, mailOptions) {
  return {
    task_id: `${Date.now()}-${Math.random().toString(16).slice(2)}`,
    task_type: 'verify_email_delivery',
    trace_id: traceId || '',
    request_id: requestId || '',
    created_at_ms: Date.now(),
    retry_count: 0,
    max_retries: config.rabbitmq_max_retries,
    routing_key: config.rabbitmq_verify_delivery_routing_key,
    payload: {
      email,
      mailOptions,
    },
  };
}

async function publishVerifyDeliveryTask(traceId, requestId, email, mailOptions) {
  await ensureConnected();
  const task = buildTask(traceId, requestId, email, mailOptions);
  publishChannel.publish(
    config.rabbitmq_exchange_direct,
    config.rabbitmq_verify_delivery_routing_key,
    Buffer.from(JSON.stringify(task)),
    {
      contentType: 'application/json',
      deliveryMode: 2,
      persistent: true,
      messageId: task.task_id,
      timestamp: Date.now(),
      headers: {
        trace_id: traceId || '',
        request_id: requestId || '',
        retry_count: 0,
      },
    }
  );
  await publishChannel.waitForConfirms();
  return task.task_id;
}

async function republishWithRetry(channel, task) {
  const retriedTask = {
    ...task,
    retry_count: Number(task.retry_count || 0) + 1,
  };
  channel.publish(
    config.rabbitmq_exchange_direct,
    config.rabbitmq_retry_routing_key,
    Buffer.from(JSON.stringify(retriedTask)),
    {
      contentType: 'application/json',
      deliveryMode: 2,
      persistent: true,
      messageId: retriedTask.task_id,
      timestamp: Date.now(),
      headers: {
        trace_id: retriedTask.trace_id || '',
        request_id: retriedTask.request_id || '',
        retry_count: retriedTask.retry_count,
      },
    }
  );
}

async function publishDlq(channel, task, errorMessage) {
  const failedTask = {
    ...task,
    failed_at_ms: Date.now(),
    last_error: errorMessage,
  };
  channel.publish(
    config.rabbitmq_exchange_dlx,
    config.rabbitmq_dlq_routing_key,
    Buffer.from(JSON.stringify(failedTask)),
    {
      contentType: 'application/json',
      deliveryMode: 2,
      persistent: true,
      messageId: failedTask.task_id,
      timestamp: Date.now(),
      headers: {
        trace_id: failedTask.trace_id || '',
        request_id: failedTask.request_id || '',
        retry_count: failedTask.retry_count || 0,
      },
    }
  );
}

async function startVerifyDeliveryWorker(deliverFn) {
  await ensureConnected();
  if (workerRunning) {
    return;
  }
  workerRunning = true;
  await consumeChannel.consume(config.rabbitmq_verify_delivery_queue, async (msg) => {
    if (!msg) {
      return;
    }
    let task = null;
    try {
      task = JSON.parse(msg.content.toString('utf8'));
      await deliverFn(task);
      consumeChannel.ack(msg);
    } catch (error) {
      const retryCount = task ? Number(task.retry_count || 0) : 0;
      const maxRetries = task ? Number(task.max_retries || config.rabbitmq_max_retries) : config.rabbitmq_max_retries;
      const errorMessage = error && error.message ? error.message : String(error);
      if (task && retryCount < maxRetries) {
        await republishWithRetry(consumeChannel, task);
        logger.warn(
          {
            event: 'varify.rabbitmq.retry_scheduled',
            trace_id: task.trace_id || '',
            request_id: task.request_id || '',
            task_id: task.task_id || '',
            retry_count: retryCount + 1,
            email: task.payload && task.payload.email ? task.payload.email : '',
            module: 'rabbitmq',
          },
          'verify delivery task scheduled for retry'
        );
      } else if (task) {
        await publishDlq(consumeChannel, task, errorMessage);
        logger.error(
          {
            event: 'varify.rabbitmq.sent_dlq',
            trace_id: task.trace_id || '',
            request_id: task.request_id || '',
            task_id: task.task_id || '',
            retry_count: retryCount,
            email: task.payload && task.payload.email ? task.payload.email : '',
            error: errorMessage,
            module: 'rabbitmq',
            error_type: 'smtp',
          },
          'verify delivery task moved to dlq'
        );
      }
      consumeChannel.ack(msg);
    }
  });
}

async function shutdownRabbitMq() {
  workerRunning = false;
  if (consumeChannel) {
    await consumeChannel.close().catch(() => {});
    consumeChannel = null;
  }
  if (publishChannel) {
    await publishChannel.close().catch(() => {});
    publishChannel = null;
  }
  if (connection) {
    await connection.close().catch(() => {});
    connection = null;
  }
}

module.exports = {
  publishVerifyDeliveryTask,
  startVerifyDeliveryWorker,
  shutdownRabbitMq,
};
