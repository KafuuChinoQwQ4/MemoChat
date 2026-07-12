# account/ 目录树

> 账号数据持久化服务（D-ACCOUNT 落地），统一封装 memo_account 用户、密码、R18 访问策略与 refresh token 数据读写。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AccountPersistence.cpp` | 账号数据持久化实现，封装用户资料、密码校验、R18 成人确认/撤销不变量、refresh token 生命周期与轮换后活跃复查 |
| `AccountPersistence.hpp` | 账号持久化声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
