# MemoOps-qml/ 目录树

> MemoOps QML 桌面运维客户端工程：C++ API 客户端/传输层 + QML 界面，提供概览、监控、日志、压测、配置与系统管理。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`qml/`](qml/_TREE.md) | 客户端 QML 界面（主窗口与各功能页）。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 客户端工程 CMake 构建配置。 |
| `OpsApiClient.cpp` | 运维 API 客户端主实现。 |
| `OpsApiClient.h` | 运维 API 客户端接口声明。 |
| `OpsApiClientAdmin.cpp` | 管理类接口的 API 客户端实现。 |
| `OpsApiClientLoadtests.cpp` | 压测相关接口的 API 客户端实现。 |
| `OpsApiClientLogs.cpp` | 日志相关接口的 API 客户端实现。 |
| `OpsApiClientMetrics.cpp` | 指标监控接口的 API 客户端实现。 |
| `OpsApiClientOverview.cpp` | 概览接口的 API 客户端实现。 |
| `OpsApiTransport.cpp` | HTTP 传输层实现。 |
| `OpsApiTransport.h` | HTTP 传输层接口声明。 |
| `OpsLogQueryBuilder.cpp` | 日志查询条件构造器实现。 |
| `OpsLogQueryBuilder.h` | 日志查询条件构造器声明。 |
| `main.cpp` | 客户端程序入口。 |
| `qml.qrc` | QML 资源打包清单。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
