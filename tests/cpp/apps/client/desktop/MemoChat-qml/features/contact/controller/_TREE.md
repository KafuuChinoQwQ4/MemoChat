# controller/ 目录树

> 测试客户端 features/contact/controller 的联系人控制器、事件服务与请求负载，附测试桩。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `ClientGatewayStub.cpp` | 客户端网关依赖的测试桩 |
| `ContactControllerOpaqueTypes.h` | 测试用的不透明类型/前置声明辅助头 |
| `ContactControllerTest.cpp` | 验证联系人控制器的编排逻辑 |
| `ContactEventServiceTest.cpp` | 验证联系人事件服务处理 |
| `ContactRequestPayloadsTest.cpp` | 验证联系人请求负载的构造/序列化 |
| `GlobalUrlPrefixStub.cpp` | 全局 URL 前缀依赖的测试桩 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
