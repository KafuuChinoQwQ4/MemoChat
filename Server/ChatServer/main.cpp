#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h" // 新增
#include <grpcpp/grpcpp.h>   // 新增
#include "ChatGrpcClient.h"

using namespace std;

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
    auto& cfg = ConfigMgr::Inst();
    auto server_name = cfg["SelfServer"]["Name"];

    try {
        auto pool = AsioIOServicePool::GetInstance();

        // [重点] 启动时重置本服登录计数为 0
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");

        // [重点] 定义并启动 gRPC Server
        std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        // 监听端口和添加服务
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        // 构建并启动 gRPC 服务器
        std::unique_ptr<grpc::Server> grpc_server(builder.BuildAndStart());
        std::cout << "RPC Server listening on " << server_address << std::endl;

        // 单独启动一个线程运行 gRPC 服务，防止阻塞主线程
        std::thread grpc_server_thread([&grpc_server]() {
            grpc_server->Wait();
        });

        ChatGrpcClient::GetInstance()->Initialize();

        // 启动 ASIO 服务 (处理客户端长连接)
        boost::asio::io_context  io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context, pool, &grpc_server](auto, auto) {
            io_context.stop();
            pool->Stop();
            grpc_server->Shutdown(); // 收到退出信号时关闭 gRPC
        });

        auto port_str = cfg["SelfServer"]["Port"];
        CServer s(io_context, atoi(port_str.c_str()));
        io_context.run();

        // [重点] 退出时清理 Redis 计数
        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
        
        grpc_server_thread.join(); // 等待 gRPC 线程结束
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << endl;
    }
}