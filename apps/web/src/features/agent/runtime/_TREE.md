# runtime/ 目录树

> Web AI 对话的非 UI 运行时辅助逻辑。

## 文件

| 文件 | 作用 |
| --- | --- |
| `agentConversationPersistence.js` | `agentConversationPersistence.ts` 的生成 JS companion。 |
| `agentConversationPersistence.test.js` | `agentConversationPersistence.test.ts` 的生成 JS companion。 |
| `agentConversationPersistence.test.ts` | 覆盖 AI 本地对话按用户持久化、恢复和裁剪逻辑。 |
| `agentConversationPersistence.ts` | 提供 AI 本地对话类型、清洗、裁剪和按用户 localStorage 读写。 |
| `agentProviderRuntime.js` | `agentProviderRuntime.ts` 的生成 JS companion。 |
| `agentProviderRuntime.test.js` | `agentProviderRuntime.test.ts` 的生成 JS companion。 |
| `agentProviderRuntime.test.ts` | 覆盖模型供应商别名和头像 data URL 规则。 |
| `agentProviderRuntime.ts` | 提供模型供应商显示名、别名匹配、配色和头像 data URL。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
