
//

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
#include "AsioIOServicePool.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"
#include <chrono>

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
		std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
		unsigned short gate_port = atoi(gate_port_str.c_str());
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

			if (error) {
				return;
			}
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, gate_port)->Start();
		memolog::LogInfo("service.start", "GateServer listening", { {"port", std::to_string(gate_port)} });
		ioc.run();
		memolog::LogInfo("service.stop", "GateServer stopped");
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
		return EXIT_SUCCESS;
	}
	catch (std::exception const& e)
	{
		memolog::LogError("service.fatal", "GateServer crashed", { {"error", e.what()} });
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
		return EXIT_FAILURE;
	}

}


