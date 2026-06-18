# controller/ 目录树

> 测试客户端 features/call/controller 的通话控制器及请求负载，附测试桩（网关/URL 前缀）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `CallControllerOpaqueTypes.h` | 测试用的不透明类型/前置声明辅助头 |
| `CallControllerTest.cpp` | 验证通话控制器的状态与流程 |
| `CallRequestPayloadsTest.cpp` | 验证通话请求负载的构造/序列化 |
| `ClientGatewayStub.cpp` | 客户端网关依赖的测试桩 |
| `GlobalUrlPrefixStub.cpp` | 全局 URL 前缀依赖的测试桩 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
