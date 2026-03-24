
//

#include "GateHttp3Listener.h"
#include "LogicSystem.h"
#include "SnowflakeUtil.h"
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include <hiredis/hiredis.h>
#include "RedisMgr.h"
#include "PostgresMgr.h"
#include "MongoMgr.h"
#include "GateAsyncSideEffects.h"
#include "GateWorkerPool.h"
#include "GateGlobals.h"
#include "AsioIOServicePool.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"
#include <chrono>
#include <thread>

void TestRedis() {


	redisContext* c = redisConnect("127.0.0.1", 6380);
	if (c->err)
	{
		printf("Connect to redisServer faile:%s\n", c->errstr);
		redisFree(c);        return;
	}
	printf("Connect to redisServer Success\n");

	std::string redis_password = "123456";
	redisReply* r = (redisReply*)redisCommand(c, "AUTH %s", redis_password.c_str());
	 if (r->type == REDIS_REPLY_ERROR) {
		 printf("Redis auth failed!\n");
	}else {
		printf("Redis auth success!\n");
		 }


	const char* command1 = "set stest1 value1";


    r = (redisReply*)redisCommand(c, command1);


	if (NULL == r)
	{
		printf("Execut command1 failure\n");
		redisFree(c);        return;
	}


	if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
	{
		printf("Failed to execute command[%s]\n", command1);
		freeReplyObject(r);
		redisFree(c);        return;
	}


	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command1);

	const char* command2 = "strlen stest1";
	r = (redisReply*)redisCommand(c, command2);


	if (r->type != REDIS_REPLY_INTEGER)
	{
		printf("Failed to execute command[%s]\n", command2);
		freeReplyObject(r);
		redisFree(c);        return;
	}


	int length = r->integer;
	freeReplyObject(r);
	printf("The length of 'stest1' is %d.\n", length);
	printf("Succeed to execute command[%s]\n", command2);


	const char* command3 = "get stest1";
	r = (redisReply*)redisCommand(c, command3);
	if (r->type != REDIS_REPLY_STRING)
	{
		printf("Failed to execute command[%s]\n", command3);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	printf("The value of 'stest1' is %s\n", r->str);
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command3);

	const char* command4 = "get stest2";
	r = (redisReply*)redisCommand(c, command4);
	if (r->type != REDIS_REPLY_NIL)
	{
		printf("Failed to execute command[%s]\n", command4);
		freeReplyObject(r);
		redisFree(c);        return;
	}
	freeReplyObject(r);
	printf("Succeed to execute command[%s]\n", command4);


	redisFree(c);

}

void TestRedisMgr() {
	assert(RedisMgr::GetInstance()->Set("blogwebsite","llfc.club"));
	std::string value="";
	assert(RedisMgr::GetInstance()->Get("blogwebsite", value) );
	assert(RedisMgr::GetInstance()->Get("nonekey", value) == false);
	assert(RedisMgr::GetInstance()->HSet("bloginfo","blogwebsite", "llfc.club"));
	assert(RedisMgr::GetInstance()->HGet("bloginfo","blogwebsite") != "");
	assert(RedisMgr::GetInstance()->ExistsKey("bloginfo"));
	assert(RedisMgr::GetInstance()->Del("bloginfo"));
	assert(RedisMgr::GetInstance()->Del("bloginfo"));
	assert(RedisMgr::GetInstance()->ExistsKey("bloginfo") == false);
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue1"));
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue2"));
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue3"));
	assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->LPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->LPop("lpushkey2", value)==false);
}

void TestPostgresMgr() {
	int id = PostgresMgr::GetInstance()->RegUser("wwc","secondtonone1@163.com","123456",": / res / head_1.jpg");
	std::cout << "id  is " << id << std::endl;
}

int main()
{
	try
	{
		auto & gCfgMgr = ConfigMgr::Inst();
		auto log_cfg = memolog::LogConfig::FromGetter(
			[&gCfgMgr](const std::string& section, const std::string& key) {
				return gCfgMgr.GetValue(section, key);
			});
		auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
			[&gCfgMgr](const std::string& section, const std::string& key) {
				return gCfgMgr.GetValue(section, key);
			});
		memolog::Logger::Init("GateServer", log_cfg);
		memolog::Telemetry::Init("GateServer", telemetry_cfg);
		const auto postgres_init_start = std::chrono::steady_clock::now();
		PostgresMgr::GetInstance();
		memolog::LogInfo("service.postgres_ready", "GateServer postgres ready",
			{
				{"postgres_init_ms", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - postgres_init_start).count())}
			});
	RedisMgr::GetInstance();
	MongoMgr::GetInstance();
	{
		auto worker_id_str = gCfgMgr.GetValue("Snowflake", "WorkerId");
		auto datacenter_id_str = gCfgMgr.GetValue("Snowflake", "DatacenterId");
		int64_t worker_id = worker_id_str.empty() ? 0 : std::stoll(worker_id_str);
		int64_t datacenter_id = datacenter_id_str.empty() ? 0 : std::stoll(datacenter_id_str);
		SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
	}
	GateAsyncSideEffects::Instance().Start();
		std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
		unsigned short gate_port = atoi(gate_port_str.c_str());
		unsigned int num_threads = std::thread::hardware_concurrency();
		if (num_threads < 2) num_threads = 4;
		auto worker_threads_str = gCfgMgr.GetValue("GateServer", "WorkerThreads");
		unsigned int worker_threads = worker_threads_str.empty() ? num_threads : std::atoi(worker_threads_str.c_str());
		if (worker_threads < 1) worker_threads = num_threads;
		gateglobals::g_worker_threads = worker_threads;
		gateglobals::g_main_ioc = nullptr;
		GateWorkerPool::GetInstance(worker_threads);
		memolog::LogInfo("gate.thread_pool", "GateServer thread pool configured",
			{ {"num_threads", std::to_string(num_threads)}, {"worker_threads", std::to_string(worker_threads)} });
		net::io_context ioc{ static_cast<int>(num_threads) };
		gateglobals::g_main_ioc = &ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

			if (error) {
				return;
			}
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, gate_port)->Start();
		memolog::LogInfo("service.start", "GateServer listening", { {"port", std::to_string(gate_port)} });

		// Start HTTP/3 listener on alternate port (port + 1)
#if defined(MEMOCHAT_ENABLE_HTTP3)
		int http3_port = gate_port + 1;
		std::string http3_error;
		auto http3_listener = std::make_shared<GateHttp3Listener>(ioc, *LogicSystem::GetInstance(), http3_port);
		if (http3_listener->Start(http3_error)) {
			memolog::LogInfo("service.http3.start", "GateServer HTTP/3 listener started",
				{{"port", std::to_string(http3_port)}});
		} else {
			memolog::LogWarn("service.http3.start.fail", "GateServer HTTP/3 listener failed to start",
				{{"error", http3_error}});
		}
#endif

		ioc.run();
		memolog::LogInfo("service.stop", "GateServer stopped");
		GateAsyncSideEffects::Instance().Stop();
		GateWorkerPool::GetInstance()->Stop();
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
		return EXIT_SUCCESS;
	}
	catch (std::exception const& e)
	{
		memolog::LogError("service.fatal", "GateServer crashed", { {"error", e.what()} });
		GateAsyncSideEffects::Instance().Stop();
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
		return EXIT_FAILURE;
	}

}


