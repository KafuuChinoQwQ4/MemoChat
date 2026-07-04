# app/ 目录树

> AI 网关服务的进程入口层，负责启动 HTTP 服务并装配路由。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | AI 网关入口层使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIGatewayRuntime.cpp` | AI 网关入口运行时配置 helper，导入 module 选择端口和线程数 |
| `AIGatewayRuntime.h` | AI 网关入口运行时配置 helper 声明 |
| `AIGatewayServer.cpp` | AIGatewayService 主程序：初始化配置、调用入口运行时 helper、注册 AI 路由并启动服务。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
