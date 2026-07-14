# bootstrap/ 目录树

> 登录后启动、会话恢复与登出清理。

## 文件

| 文件 | 作用 |
| --- | --- |
| `postLoginBootstrap.ts` | 密码登录与 refresh 恢复后的 chat 连接 / 列表 bootstrap。 |
| `postLoginBootstrap.test.ts` | 验证浏览器聊天端点选择与安全 URL 构造。 |
| `sessionRestore.ts` | 启动时用 refresh token 恢复 access session。 |
| `sessionRestore.test.ts` | 验证启动恢复不依赖 JavaScript 可见的 refresh token。 |
| `SessionRestoreBoot.tsx` | 首屏触发 session restore 的 React 包装。 |
| `connectionCoordinator.ts` | WS 断开重连状态协调。 |
| `logoutTeardown.ts` | 登出清理 session / 实体 / gateway。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
