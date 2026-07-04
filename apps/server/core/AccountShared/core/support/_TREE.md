# support/ 目录树

> 账号域的支撑工具：登录流程辅助与 HTTP2 资料处理辅助。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 登录支撑使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthLoginCacheProfileDto.cpp` | 登录资料缓存 DTO 编解码实现 |
| `AuthLoginCacheProfileDto.hpp` | 登录资料缓存 DTO 声明 |
| `AuthLoginRateLimiter.cpp` | 登录失败 Redis 限流实现 |
| `AuthLoginRateLimiter.hpp` | 登录失败 Redis 限流声明 |
| `AuthLoginSupport.cpp` | 登录流程支撑逻辑实现 |
| `AuthLoginSupport.hpp` | 登录支撑声明 |
| `AuthPublicDtos.cpp` | 账号公开接口请求/响应 DTO 解析实现，包含登录与 refresh token 契约 |
| `AuthPublicDtos.hpp` | 账号公开接口请求/响应 DTO 声明，包含登录与 refresh token 契约 |
| `AuthUserInfo.hpp` | 登录流程使用的轻量用户资料结构 |
| `Http2ProfileSupport.cpp` | HTTP2 资料处理支撑实现 |
| `Http2ProfileSupport.hpp` | HTTP2 资料支撑声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
