# business/ 目录树

> 业务库 PostgreSQL 迁移序列：按编号演进用户/好友、聊天事件外发、朋友圈、AI Agent 与 A2A 对局，以及媒体/朋友圈/通话/账号的库拆分 schema。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `001_baseline.sql` | 基线建库：用户、好友等核心表与 schema |
| `002_chat_event_outbox.sql` | 聊天事件外发表（outbox 模式） |
| `003_moments.sql` | 朋友圈相关表（动态/点赞/评论） |
| `004_ai_agent.sql` | AI Agent 会话/消息/知识库/记忆等表 |
| `005_ai_agent_harness.sql` | AI Agent 运行 trace/step/反馈等观测表 |
| `006_a2a_game_persistence.sql` | A2A 对局房间状态等持久化表 |
| `007_a2a_game_templates.sql` | A2A 对局模板表 |
| `008_db_split_media_moments_call.sql` | 媒体/朋友圈/通话库拆分迁移 |
| `008_db_split_media_moments_call_rollback.sql` | 上述库拆分的回滚脚本 |
| `008_memo_call_schema.sql` | memo 通话会话等 schema/表 |
| `008_memo_media_schema.sql` | memo 聊天媒体资产 schema/表 |
| `008_memo_moments_schema.sql` | memo 朋友圈相关 schema/表 |
| `009_memo_account_schema.sql` | memo 账号（user/user_id）schema/表 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
