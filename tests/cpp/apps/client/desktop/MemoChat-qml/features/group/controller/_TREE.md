# controller/ 目录树

> 测试客户端 features/group/controller 的群组控制器与群管理事件/响应/效果应用链路，附测试桩。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `ConversationSyncServiceStub.cpp` | 会话同步服务依赖的测试桩 |
| `GlobalUrlPrefixStub.cpp` | 全局 URL 前缀依赖的测试桩 |
| `GroupControllerTest.cpp` | 验证群组控制器的编排逻辑 |
| `GroupManagementEffectApplierTest.cpp` | 验证群管理结果对本地状态的效果应用 |
| `GroupManagementEventServiceTest.cpp` | 验证群管理事件服务处理 |
| `GroupManagementResponseServiceTest.cpp` | 验证群管理响应服务处理 |
| `GroupRequestPayloadsTest.cpp` | 验证群组请求负载的构造/序列化 |
| `GroupResponseErrorServiceTest.cpp` | 验证群响应错误处理服务 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
