# coordinators/ 目录树

> 测试客户端 app/coordinators 的媒体协调器、挂起附件发送执行器、聊天入口和关系引导协调器。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `MediaCoordinatorTest.cpp` | 验证媒体协调器的调度与状态流转 |
| `MediaPendingAttachmentRunnerTest.cpp` | 验证挂起附件上传/发送执行器逻辑 |
| `SessionChatEntryCoordinatorTest.cpp` | 验证聊天登录完成后才启动联系人与关系引导 |
| `SessionRelationBootstrapTest.cpp` | 验证关系引导刷新联系人、会话和申请模型的协调顺序 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
