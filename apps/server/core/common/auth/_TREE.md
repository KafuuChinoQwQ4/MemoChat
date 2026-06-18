# auth/ 目录树

> 跨服务共享的认证基础定义：服务端密钥与 ChatServer 本地校验的 HMAC 登录票据（D-TOKEN 契约）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthSecret.h` | 认证用共享密钥定义与读取 |
| `ChatLoginTicket.h` | HMAC 登录票据结构，供 ChatServer 本地校验 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
