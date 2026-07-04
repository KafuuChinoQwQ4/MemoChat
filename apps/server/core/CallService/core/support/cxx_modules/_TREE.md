# cxx_modules/ 目录树

> CallService 核心支撑的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallPublicDto.cppm` | 通话公共请求 DTO 使用的可选字段读取判断算法 module |
| `CallService.cppm` | 通话服务门面使用的配置、状态、事件与 TTL 判断算法 module |
| `CallSessionCache.cppm` | 通话会话缓存 DTO 使用的缓存身份有效性判断算法 module |
| `CallSessionMath.cppm` | 通话会话时间/房间短 id/时长计算与通知消息 id 等原始算法 module |
| `CallToken.cppm` | 通话 LiveKit/JWT token 使用的 Base64URL 编码算法 module |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
