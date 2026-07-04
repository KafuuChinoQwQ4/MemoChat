# auth/ 目录树

> 认证领域服务，封装登录校验、token/票据签发等认证业务逻辑。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthService.cpp` | 认证领域服务实现（import auth_service_algorithms module 消费响应元数据/uid 守卫/传输选择） |
| `AuthService.hpp` | 认证领域服务声明 |
| `modules/` | 认证领域服务的项目自有 C++ module interface 子目录 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
