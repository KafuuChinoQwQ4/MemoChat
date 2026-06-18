# services/ 目录树

> 测试客户端 features/chat/services 的聊天业务服务群（协议路由、私聊收发/历史、消息变更、附件分发等）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `ChatDialogListResponseServiceTest.cpp` | 验证会话列表响应服务处理 |
| `ChatProtocolRouterTest.cpp` | 验证聊天协议按消息 ID 路由到对应职责 |
| `DialogListServiceTest.cpp` | 验证会话列表服务的拉取/合并逻辑 |
| `GlobalUrlPrefixStub.cpp` | 全局 URL 前缀依赖的测试桩 |
| `IncomingMessageRouterTest.cpp` | 验证入站消息路由分发 |
| `MessageMutationCommandServiceTest.cpp` | 验证消息编辑/撤回等变更命令服务 |
| `PrivateChatEventServiceTest.cpp` | 验证私聊事件服务处理 |
| `PrivateChatHistoryRequestServiceTest.cpp` | 验证私聊历史请求服务 |
| `PrivateChatHistoryServiceTest.cpp` | 验证私聊历史合并/落库服务 |
| `PrivateChatSendServiceTest.cpp` | 验证私聊发送服务流程 |
| `UploadedAttachmentDispatchServiceTest.cpp` | 验证已上传附件的分发服务 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
