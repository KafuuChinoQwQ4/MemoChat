#include "EmailTaskBus.hpp"
#include "EmailDeliveryTaskCodec.hpp"
#include "EmailSender.hpp"
#include "VarifyServiceImpl.hpp"
#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"

#include <charconv>

import memochat.varify.email_task_bus_algorithms;

#ifndef MEMOCHAT_ENABLE_RABBITMQ
#define MEMOCHAT_ENABLE_RABBITMQ 0
#endif

#if MEMOCHAT_ENABLE_RABBITMQ
#include "../common/WinsockCompat.hpp"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>

#include <cctype>
#include <cstring>
#endif

namespace
{

int ParseIntOrDefault(const std::string& raw, int fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    int value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return ec == std::errc{} && ptr == raw.data() + raw.size() ? value : fallback;
}

#if MEMOCHAT_ENABLE_RABBITMQ
bool RpcReplyOk(amqp_rpc_reply_t reply)
{
    return reply.reply_type == AMQP_RESPONSE_NORMAL;
}

std::string BytesToString(amqp_bytes_t bytes)
{
    return std::string(static_cast<const char*>(bytes.bytes), bytes.len);
}
#endif

} // anonymous namespace

namespace varifyservice
{
namespace email_task_bus_modules = memochat::varify::email_task_bus::modules;

EmailTaskBus::EmailTaskBus()
    : connection_(nullptr)
    , last_envelope_(nullptr)
    , started_(false)
    , rabbitmq_healthy_(false)
    , stop_(false)
{
    auto& cfg = ConfigMgr::Inst();
    config_host_ = cfg["RabbitMQ"]["Host"];
    config_port_ = ParseIntOrDefault(cfg["RabbitMQ"]["Port"], config_port_);
    config_username_ = cfg["RabbitMQ"]["Username"];
    config_password_ = cfg["RabbitMQ"]["Password"];
    config_vhost_ = cfg["RabbitMQ"]["VHost"];
    if (config_vhost_.empty())
        config_vhost_ = email_task_bus_modules::DefaultVHost();
    config_exchange_direct_ = cfg["RabbitMQ"]["ExchangeDirect"];
    if (config_exchange_direct_.empty())
        config_exchange_direct_ = email_task_bus_modules::DefaultDirectExchange();
    config_exchange_dlx_ = cfg["RabbitMQ"]["ExchangeDlx"];
    if (config_exchange_dlx_.empty())
        config_exchange_dlx_ = email_task_bus_modules::DefaultDlxExchange();
    config_verify_delivery_routing_key_ = cfg["RabbitMQ"]["VerifyDeliveryRoutingKey"];
    if (config_verify_delivery_routing_key_.empty())
        config_verify_delivery_routing_key_ = email_task_bus_modules::DefaultVerifyDeliveryRoutingKey();
    config_queue_ = cfg["RabbitMQ"]["VerifyDeliveryQueue"];
    if (config_queue_.empty())
        config_queue_ = email_task_bus_modules::DefaultVerifyDeliveryQueue();
    config_retry_queue_ = cfg["RabbitMQ"]["VerifyDeliveryRetryQueue"];
    if (config_retry_queue_.empty())
        config_retry_queue_ = email_task_bus_modules::DefaultVerifyDeliveryRetryQueue();
    config_dlq_queue_ = cfg["RabbitMQ"]["VerifyDeliveryDlqQueue"];
    if (config_dlq_queue_.empty())
        config_dlq_queue_ = email_task_bus_modules::DefaultVerifyDeliveryDlqQueue();
    config_retry_routing_key_ = cfg["RabbitMQ"]["RetryRoutingKey"];
    if (config_retry_routing_key_.empty())
        config_retry_routing_key_ = email_task_bus_modules::DefaultRetryRoutingKey();
    config_dlq_routing_key_ = cfg["RabbitMQ"]["DlqRoutingKey"];
    if (config_dlq_routing_key_.empty())
        config_dlq_routing_key_ = email_task_bus_modules::DefaultDlqRoutingKey();
    config_retry_delay_ms_ = email_task_bus_modules::NormalizeRetryDelayMs(
        ParseIntOrDefault(cfg["RabbitMQ"]["RetryDelayMs"], config_retry_delay_ms_));
    config_max_retries_ = email_task_bus_modules::NormalizeMaxRetries(
        ParseIntOrDefault(cfg["RabbitMQ"]["MaxRetries"], config_max_retries_));

#if MEMOCHAT_ENABLE_RABBITMQ
    std::string err;
    if (!Connect())
    {
        memolog::LogWarn("varify.emailtaskbus.init_failed",
                         "EmailTaskBus init failed",
                         {{"host", config_host_}, {"error", err}});
    }
    else
    {
        rabbitmq_healthy_.store(true, std::memory_order_release);
        memolog::LogInfo("varify.emailtaskbus.init_ok",
                         "EmailTaskBus connected",
                         {{"host", config_host_}, {"port", std::to_string(config_port_)}});
    }
#else
    memolog::LogInfo("varify.emailtaskbus.init_skipped", "RabbitMQ disabled at build time");
#endif
}

EmailTaskBus::~EmailTaskBus()
{
    StopWorker();
    Close();
}

EmailTaskBus& EmailTaskBus::Instance()
{
    static EmailTaskBus bus;
    return bus;
}

bool EmailTaskBus::Connect()
{
#if !MEMOCHAT_ENABLE_RABBITMQ
    (void) config_port_;
    return false;
#else
    Close();
    amqp_connection_state_t conn = amqp_new_connection();
    if (!conn)
        return false;
    auto* socket = amqp_tcp_socket_new(conn);
    if (!socket)
    {
        amqp_destroy_connection(conn);
        return false;
    }
    if (amqp_socket_open(socket, config_host_.c_str(), config_port_) != AMQP_STATUS_OK)
    {
        amqp_destroy_connection(conn);
        return false;
    }
    amqp_rpc_reply_t reply = amqp_login(conn,
                                        config_vhost_.c_str(),
                                        0,
                                        131072,
                                        0,
                                        AMQP_SASL_METHOD_PLAIN,
                                        config_username_.c_str(),
                                        config_password_.c_str());
    if (!RpcReplyOk(reply))
    {
        amqp_destroy_connection(conn);
        return false;
    }
    amqp_channel_open(conn, 1);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
    {
        amqp_destroy_connection(conn);
        return false;
    }
    connection_ = conn;
    if (!EnsureTopology())
    {
        Close();
        return false;
    }
    return true;
#endif
}

void EmailTaskBus::Close()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    ClearLastConsumed();
    if (connection_)
    {
        amqp_channel_close(static_cast<amqp_connection_state_t>(connection_), 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(static_cast<amqp_connection_state_t>(connection_), AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(static_cast<amqp_connection_state_t>(connection_));
        connection_ = nullptr;
    }
#endif
}

bool EmailTaskBus::EnsureTopology()
{
#if !MEMOCHAT_ENABLE_RABBITMQ
    return false;
#else
    auto* conn = static_cast<amqp_connection_state_t>(connection_);
    amqp_exchange_declare(conn,
                          1,
                          amqp_cstring_bytes(config_exchange_direct_.c_str()),
                          amqp_cstring_bytes("direct"),
                          0,
                          1,
                          0,
                          0,
                          amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_exchange_declare(conn,
                          1,
                          amqp_cstring_bytes(config_exchange_dlx_.c_str()),
                          amqp_cstring_bytes("direct"),
                          0,
                          1,
                          0,
                          0,
                          amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_queue_declare(conn, 1, amqp_cstring_bytes(config_queue_.c_str()), 0, 1, 0, 0, amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_queue_bind(conn,
                    1,
                    amqp_cstring_bytes(config_queue_.c_str()),
                    amqp_cstring_bytes(config_exchange_direct_.c_str()),
                    amqp_cstring_bytes(config_verify_delivery_routing_key_.c_str()),
                    amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_table_entry_t entries[3];
    entries[0].key = amqp_cstring_bytes(const_cast<char*>("x-dead-letter-exchange"));
    entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
    entries[0].value.value.bytes = amqp_cstring_bytes(config_exchange_direct_.c_str());
    entries[1].key = amqp_cstring_bytes(const_cast<char*>("x-dead-letter-routing-key"));
    entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
    entries[1].value.value.bytes = amqp_cstring_bytes(config_verify_delivery_routing_key_.c_str());
    entries[2].key = amqp_cstring_bytes(const_cast<char*>("x-message-ttl"));
    entries[2].value.kind = AMQP_FIELD_KIND_I32;
    entries[2].value.value.i32 = config_retry_delay_ms_;
    amqp_table_t args;
    args.num_entries = 3;
    args.entries = entries;
    amqp_queue_declare(conn, 1, amqp_cstring_bytes(config_retry_queue_.c_str()), 0, 1, 0, 0, args);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_queue_bind(conn,
                    1,
                    amqp_cstring_bytes(config_retry_queue_.c_str()),
                    amqp_cstring_bytes(config_exchange_direct_.c_str()),
                    amqp_cstring_bytes(config_retry_routing_key_.c_str()),
                    amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_queue_declare(conn, 1, amqp_cstring_bytes(config_dlq_queue_.c_str()), 0, 1, 0, 0, amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    amqp_queue_bind(conn,
                    1,
                    amqp_cstring_bytes(config_dlq_queue_.c_str()),
                    amqp_cstring_bytes(config_exchange_dlx_.c_str()),
                    amqp_cstring_bytes(config_dlq_routing_key_.c_str()),
                    amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(conn)))
        return false;

    return true;
#endif
}

bool EmailTaskBus::PublishVerifyDeliveryTask(const std::string& email,
                                             const std::string& code,
                                             const std::string& trace_id)
{
    std::string body = SerializeEmailTask(EmailDeliveryTaskPayload{email, code, trace_id, 0});
    bool ok = PublishToQueue(config_exchange_direct_, config_verify_delivery_routing_key_, body);

    if (!ok)
    {
        rabbitmq_healthy_.store(false, std::memory_order_release);
        memolog::LogWarn("varify.emailtaskbus.publish_failed",
                         "publish to RabbitMQ failed",
                         {{"email", email}, {"trace_id", trace_id}});
    }
    else
    {
        memolog::LogInfo("varify.emailtaskbus.publish_ok",
                         "published email delivery task",
                         {{"email", email}, {"trace_id", trace_id}});
    }
    return ok;
}

bool EmailTaskBus::PublishToQueue(const std::string& exchange, const std::string& routing_key, const std::string& body)
{
#if !MEMOCHAT_ENABLE_RABBITMQ
    (void) exchange;
    (void) routing_key;
    (void) body;
    return false;
#else
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_ && !Connect())
    {
        return false;
    }
    auto* conn = static_cast<amqp_connection_state_t>(connection_);

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;

    int status = amqp_basic_publish(conn,
                                    1,
                                    amqp_cstring_bytes(exchange.c_str()),
                                    amqp_cstring_bytes(routing_key.c_str()),
                                    0,
                                    0,
                                    &props,
                                    amqp_cstring_bytes(body.c_str()));

    if (status == AMQP_STATUS_UNEXPECTED_STATE)
    {
        amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn);
        if (!RpcReplyOk(reply))
        {
            Close();
            return false;
        }
    }
    else if (status != AMQP_STATUS_OK)
    {
        return false;
    }
    return true;
#endif
}

bool EmailTaskBus::StartWorker(EmailSender* sender)
{
    (void) sender;
    if (started_.load(std::memory_order_acquire))
        return true;
#if MEMOCHAT_ENABLE_RABBITMQ
    if (!rabbitmq_healthy_.load(std::memory_order_acquire))
    {
        memolog::LogWarn("varify.emailtaskbus.worker_skip", "RabbitMQ not healthy, skipping worker");
        return true;
    }
    stop_.store(false, std::memory_order_release);
    std::string thread_error;
    if (!worker_thread_.Start(
            [this]()
            {
                WorkerLoop(nullptr);
            },
            &thread_error))
    {
        stop_.store(true, std::memory_order_release);
        memolog::LogError("varify.emailtaskbus.worker_start_failed",
                          "failed to start EmailTaskBus worker",
                          {{"error", thread_error}});
        return false;
    }
    started_.store(true, std::memory_order_release);
#endif
    return true;
}

void EmailTaskBus::StopWorker()
{
    if (!started_.load(std::memory_order_acquire) && !worker_thread_.Joinable())
        return;
    stop_.store(true, std::memory_order_release);
    queue_cv_.notify_all();
    if (worker_thread_.Joinable())
    {
        std::string thread_error;
        if (!worker_thread_.Join(&thread_error))
        {
            memolog::LogError("varify.emailtaskbus.worker_join_failed",
                              "failed to join EmailTaskBus worker",
                              {{"error", thread_error}});
        }
    }
    started_.store(false, std::memory_order_release);
    memolog::LogInfo("varify.emailtaskbus.worker_stopped", "EmailTaskBus worker stopped");
}

bool EmailTaskBus::ConsumeOnce(EmailDeliveryTask& task)
{
#if !MEMOCHAT_ENABLE_RABBITMQ
    return false;
#else
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_)
        return false;

    auto* conn = static_cast<amqp_connection_state_t>(connection_);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;

    amqp_maybe_release_buffers(conn);
    auto* envelope = new amqp_envelope_t();
    memset(envelope, 0, sizeof(amqp_envelope_t));

    amqp_rpc_reply_t reply = amqp_consume_message(conn, envelope, &timeout, 0);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        delete envelope;
        return false;
    }

    std::string body = BytesToString(envelope->message.body);
    std::string routing_key = BytesToString(envelope->routing_key);
    bool from_retry = (routing_key == config_retry_routing_key_);

    EmailDeliveryTaskPayload parsed;
    if (!ParseEmailTask(body, &parsed))
    {
        amqp_basic_ack(conn, 1, envelope->delivery_tag, false);
        amqp_destroy_envelope(envelope);
        delete envelope;
        return false;
    }

    if (from_retry)
    {
        parsed.retry_count++;
    }

    task = EmailDeliveryTask{parsed.email, parsed.code, parsed.trace_id, parsed.retry_count};

    ClearLastConsumed();
    last_envelope_ = envelope;
    return true;
#endif
}

void EmailTaskBus::ClearLastConsumed()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    if (last_envelope_)
    {
        auto* env = static_cast<amqp_envelope_t*>(last_envelope_);
        amqp_destroy_envelope(env);
        delete env;
        last_envelope_ = nullptr;
    }
#endif
}

void EmailTaskBus::WorkerLoop(EmailSender* /*sender*/)
{
    started_.store(true, std::memory_order_release);
    memolog::LogInfo("varify.emailtaskbus.worker_started", "EmailTaskBus worker started", {{"queue", config_queue_}});

#if !MEMOCHAT_ENABLE_RABBITMQ
    std::unique_lock<std::mutex> lock(queue_mutex_);
    while (!stop_.load(std::memory_order_acquire))
    {
        queue_cv_.wait_for(lock,
                           std::chrono::milliseconds(100),
                           [this]()
                           {
                               return stop_.load(std::memory_order_acquire) || !pending_queue_.empty();
                           });
        if (stop_.load(std::memory_order_acquire))
            break;
        if (!pending_queue_.empty())
        {
            EmailDeliveryTask task = pending_queue_.front();
            pending_queue_.pop();
            lock.unlock();
            bool ok = EmailSender::Send(task.email, task.code);
            if (ok)
            {
                g_metrics.email_sent_async.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                g_metrics.email_failed.fetch_add(1, std::memory_order_relaxed);
            }
            lock.lock();
        }
    }
#else
    while (!stop_.load(std::memory_order_acquire))
    {
        EmailDeliveryTask task;
        if (ConsumeOnce(task))
        {
            memolog::LogInfo(
                "varify.emailtaskbus.processing",
                "processing email delivery task",
                {{"email", task.email}, {"trace_id", task.trace_id}, {"retry", std::to_string(task.retry_count)}});

            bool sent = EmailSender::Send(task.email, task.code);
            if (sent)
            {
                g_metrics.email_sent_async.fetch_add(1, std::memory_order_relaxed);
                memolog::LogInfo("varify.emailtaskbus.sent_ok", "email sent successfully", {{"email", task.email}});
                std::lock_guard<std::mutex> ack_lock(mutex_);
                AckLastConsumed();
            }
            else
            {
                g_metrics.email_failed.fetch_add(1, std::memory_order_relaxed);
                memolog::LogWarn("varify.emailtaskbus.send_failed",
                                 "email send failed",
                                 {{"email", task.email},
                                  {"retry", std::to_string(task.retry_count)},
                                  {"max_retries", std::to_string(config_max_retries_)}});
                if (email_task_bus_modules::ShouldRetryTask(task.retry_count, config_max_retries_))
                {
                    const int next_retry = email_task_bus_modules::NextRetryCount(task.retry_count);
                    std::string body =
                        SerializeEmailTask(EmailDeliveryTaskPayload{task.email, task.code, task.trace_id, next_retry});
                    PublishToQueue(config_exchange_direct_, config_retry_routing_key_, body);
                    memolog::LogInfo("varify.emailtaskbus.requeued",
                                     "task requeued for retry",
                                     {{"email", task.email}, {"retry", std::to_string(next_retry)}});
                }
                else
                {
                    std::string dlq_body = SerializeEmailTask(
                        EmailDeliveryTaskPayload{task.email, task.code, task.trace_id, task.retry_count});
                    PublishToQueue(config_exchange_dlx_, config_dlq_routing_key_, dlq_body);
                    memolog::LogError("varify.emailtaskbus.dlq",
                                      "task moved to DLQ after max retries",
                                      {{"email", task.email}, {"retry", std::to_string(task.retry_count)}});
                }
                std::lock_guard<std::mutex> ack_lock2(mutex_);
                AckLastConsumed();
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
#endif

    memolog::LogInfo("varify.emailtaskbus.worker_exit", "EmailTaskBus worker exiting");
}

void EmailTaskBus::AckLastConsumed()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    if (connection_ && last_envelope_)
    {
        auto* env = static_cast<amqp_envelope_t*>(last_envelope_);
        amqp_basic_ack(static_cast<amqp_connection_state_t>(connection_), 1, env->delivery_tag, false);
    }
    ClearLastConsumed();
#endif
}

} // namespace varifyservice
