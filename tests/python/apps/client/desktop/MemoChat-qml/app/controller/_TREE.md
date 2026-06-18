# controller/ 目录树

> 测试客户端 app/controller 的应用控制器职责拆分契约（门面瘦身、特性上下文、消息缓冲、历史刷新与头像等边界）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_app_controller_auth_boundary.py` | 校验应用控制器与认证层的边界 |
| `test_appcontroller_facade_pruning_contract.py` | 校验 AppController 门面瘦身契约 |
| `test_appcontroller_shell_guardrails.py` | 校验 AppController 与外壳的护栏约束 |
| `test_chat_history_initial_refresh.py` | 校验聊天历史初始刷新行为契约 |
| `test_direct_feature_context_contract.py` | 校验功能直连上下文的契约 |
| `test_incoming_message_buffer_contract.py` | 校验入站消息缓冲契约 |
| `test_offline_history_bootstrap.py` | 校验离线历史引导加载契约 |
| `test_user_avatar_profile_contract.py` | 校验用户头像/资料关联契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
