# auth/ 目录树

> 跨服务共享的认证基础定义：服务端密钥、密码哈希与 ChatServer 本地校验的 HMAC 登录票据（D-TOKEN 契约）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthSecret.hpp` | 认证用共享密钥定义、开发默认识别与生产启动硬失败检查 |
| `ChatLoginTicket.hpp` | HMAC 登录票据结构，供 ChatServer 本地校验 |
| `PasswordHasher.cpp` | libsodium 密码哈希、验证与 rehash 判定实现 |
| `PasswordHasher.hpp` | 密码哈希深模块接口 |
| `RefreshToken.cpp` | refresh token 生成、拆分、哈希与常量时间验证实现 |
| `RefreshToken.hpp` | refresh token 深模块接口 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
