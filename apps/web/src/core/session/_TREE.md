# session/ 目录树

> Web 短期登录会话状态；长期 refresh token 仅存在于 HttpOnly Cookie。

## 文件

| 文件 | 作用 |
| --- | --- |
| `sessionStore.ts` | 登录态 Zustand store；仅保存内存态 access token 与 login ticket。 |
| `sessionTypes.ts` | 会话相关类型定义。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
