const grpc = require('@grpc/grpc-js');
const message_proto = require('./proto');
const const_module = require('./const');
const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email');
const config_module = require('./config');
const redis_module = require('./redis');
const { logger, newTraceId, redactEmail } = require('./logger');
const { exportZipkinSpan, startServerSpan, childSpan } = require('./telemetry');

let grpcServer = null;
let shuttingDown = false;

function traceIdFromCall(call) {
  const md = call && call.metadata;
  if (md && typeof md.get === 'function') {
    const values = md.get('x-trace-id');
    if (values && values.length > 0 && values[0]) {
      return String(values[0]);
    }
  }
  return newTraceId();
}

async function GetVarifyCode(call, callback) {
  const span = startServerSpan('VarifyService.GetVarifyCode', call, {
    'rpc.system': 'grpc',
    'rpc.service': 'VarifyService',
    'rpc.method': 'GetVarifyCode',
  });
  const trace_id = span.trace_id;
  const email = call.request.email;
  logger.info({ event: 'varify.get_code.request', trace_id, request_id: span.request_id, span_id: span.span_id, email: redactEmail(email), module: 'grpc' }, 'get verify code request');

  try {
    let query_res = await redis_module.GetRedis(const_module.code_prefix + email, span);
    let uniqueId = query_res;

    if (query_res == null) {
      uniqueId = uuidv4();
      if (uniqueId.length > 4) {
        uniqueId = uniqueId.substring(0, 4);
      }
      const bres = await redis_module.SetRedisExpire(const_module.code_prefix + email, uniqueId, 600, span);
      if (!bres) {
        logger.warn({ event: 'varify.get_code.redis_failed', trace_id, request_id: span.request_id, span_id: span.span_id, email: redactEmail(email), module: 'grpc', error_type: 'redis' }, 'set redis verify code failed');
        exportZipkinSpan(span, { email: redactEmail(email), 'otel.status_code': 'ERROR', 'error.type': 'redis' });
        callback(null, {
          email,
          error: const_module.Errors.RedisErr,
        });
        return;
      }
    }

    const text_str = `您的验证码为${uniqueId}请三分钟内完成注册`;
    const mailOptions = {
      from: config_module.email_from,
      to: email,
      subject: '验证码',
      text: text_str,
    };

    const send_res = await emailModule.SendMail(mailOptions);
    logger.info(
      {
        event: 'varify.get_code.sent',
        trace_id,
        request_id: span.request_id,
        span_id: span.span_id,
        email: redactEmail(email),
        smtp_message: send_res,
        module: 'grpc',
      },
      'verify code sent'
    );
    exportZipkinSpan(span, { email: redactEmail(email), peer_service: 'smtp' });

    callback(null, {
      email,
      error: const_module.Errors.Success,
    });
  } catch (error) {
    logger.error(
      {
        event: 'varify.get_code.exception',
        trace_id,
        request_id: span.request_id,
        span_id: span.span_id,
        email: redactEmail(email),
        error: error && error.message ? error.message : String(error),
        module: 'grpc',
        error_type: 'exception',
      },
      'get verify code exception'
    );
    exportZipkinSpan(span, { email: redactEmail(email), error: error && error.message ? error.message : String(error), 'otel.status_code': 'ERROR' });

    callback(null, {
      email,
      error: const_module.Errors.Exception,
    });
  }
}

function main() {
  grpcServer = new grpc.Server();
  const bindAddress = '0.0.0.0:50051';
  grpcServer.addService(message_proto.VarifyService.service, { GetVarifyCode });

  grpcServer.bindAsync(bindAddress, grpc.ServerCredentials.createInsecure(), (err, port) => {
    if (err) {
      logger.error({ event: 'service.bind.failed', address: bindAddress, error: err.message || String(err), module: 'grpc', error_type: 'bind' }, 'failed to bind gRPC server');
      process.exit(1);
      return;
    }

    if (!port) {
      logger.error({ event: 'service.bind.failed', address: bindAddress, module: 'grpc', error_type: 'bind' }, 'gRPC server did not bind any port');
      process.exit(1);
      return;
    }

    logger.info({ event: 'service.start', address: bindAddress, module: 'grpc' }, 'VarifyServer started');
  });
}

function stopGrpcServer() {
  return new Promise((resolve) => {
    if (!grpcServer) {
      resolve();
      return;
    }
    grpcServer.tryShutdown(() => resolve());
  });
}

async function shutdown(signal) {
  if (shuttingDown) {
    return;
  }
  shuttingDown = true;
  logger.info({ event: 'service.stop', signal, module: 'grpc' }, 'shutting down VarifyServer');
  await stopGrpcServer();
  await redis_module.closeRedis();
  process.exit(0);
}

process.on('SIGINT', () => {
  shutdown('SIGINT').catch((err) => logger.error({ event: 'service.stop.error', error: err.message || String(err) }));
});
process.on('SIGTERM', () => {
  shutdown('SIGTERM').catch((err) => logger.error({ event: 'service.stop.error', error: err.message || String(err) }));
});

main();
