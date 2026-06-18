# LoginService/ 目录树

> 登录鉴权服务，从 GateServer 剥离；经 account-core 读账号数据，签发 HTTP 会话 token 与 HMAC 登录票据，对外提供 /user_login。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | LoginServer 进程入口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | LoginService 构建脚本 |
| `login.ini` | LoginServer 运行配置 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
