# AccountShared/ 目录树

> Account/Login/Register 三个域服务共享的领域库（D-ACCOUNT 落地点）：账号/认证业务逻辑、资料持久化、缓存、异步副作用与路由 Profile，供各服务以 account-core 形式复用。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`core/`](core/_TREE.md) | 异步副作用、缓存与登录/资料支撑等核心层 |
| [`domain/`](domain/_TREE.md) | 认证/资料领域模块、服务与路由 Profile |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | AccountShared 库构建脚本 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
