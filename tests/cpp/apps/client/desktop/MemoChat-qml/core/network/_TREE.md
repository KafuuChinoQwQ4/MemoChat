# network/ 目录树

> 测试客户端 core/network 的协议帧编解码与聊天消息分发路由（含群历史/群负载分支）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `ChatFrameCodecTest.cpp` | 验证聊天帧的编码/解码 |
| `ChatMessageDispatcherGroupHistoryTest.cpp` | 验证消息分发器对群历史的处理 |
| `ChatMessageDispatcherGroupPayloadTest.cpp` | 验证消息分发器对群消息负载的解析 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
