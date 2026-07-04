# app/ 目录树

> 聚焦域网关进程（Media/Moments/Call/R18 等）的共享入口骨架，统一各域 exe 的启动流程。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateDomainServer.cpp` | 单域网关进程共享入口实现。 |
| `GateDomainServer.h` | 单域网关共享入口骨架声明。 |
| `GateDomainRuntime.cpp` | 导入 GateDomain runtime C++ module 并暴露端口/线程选择函数。 |
| `GateDomainRuntime.h` | 单域网关 runtime 端口与线程选择函数声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
