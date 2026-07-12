# auth/ 目录树

> 认证领域服务，封装登录校验、token/票据签发及按 uid 串行化的登录/改密/刷新凭据变更。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthService.cpp` | 认证领域服务实现，凭据锁内完成密码校验、改密撤销、refresh 活跃复查与 token 签发 |
| `AuthService.hpp` | 认证领域服务声明 |
| `modules/` | 认证领域服务的项目自有 C++ module interface 子目录 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
