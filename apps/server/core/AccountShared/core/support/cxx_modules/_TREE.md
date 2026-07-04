# cxx_modules/ 目录树

> AccountShared 账号支撑的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthLogin.cppm` | 登录版本门槛使用的 SemVer 解析与比较算法 module |
| `AuthLoginCacheProfile.cppm` | 登录资料缓存 DTO 使用的输出指针和缓存身份有效性判定算法 module |
| `AuthPublicDto.cppm` | 账号公开请求 DTO 必填字段组合判断算法 module |
| `ProfileSupport.cppm` | HTTP2 资料支撑成功/错误码、错误信息、Redis key 前缀与 uid 守卫算法 module |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
