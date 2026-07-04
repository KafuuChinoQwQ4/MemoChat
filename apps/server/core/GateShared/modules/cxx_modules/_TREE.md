# cxx_modules/ 目录树

> GateShared 的 C++ modules 接口单元与消费边界，用于验证并逐步迁移主项目自定义 CMI 构建链。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateDomainRuntime.cppm` | 导出单域网关进程启动使用的监听端口和线程数选择算法模块。 |
| `GateHttpJsonSupport.cppm` | 导出 Gate HTTP JSON 支持使用的 content-type、空 trace 与解析/trace guard 算法模块。 |
| `GateModuleProbe.cppm` | 导出 `memochat.gate.module_probe` 模块的最小接口单元。 |
| `GateModuleProbeConsumer.cpp` | 在普通 C++ 翻译单元中 `import` 探针模块，证明主构建可消费 CMI。 |
| `GateModuleProbeConsumer.h` | 暴露探针消费者函数给常规 C++ 代码或测试使用。 |
| `GateRouting.cppm` | 导出 Gate 路由注册表使用的 method 规范化、route key 与前缀匹配算法模块。 |
| `GateWorkerPool.cppm` | 导出 Gate worker 线程池线程数选择和 join/stop guard 算法模块。 |
| `HealthRoute.cppm` | 导出 Gate 健康检查/就绪检查路由使用的路径、状态与响应元数据算法模块。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
