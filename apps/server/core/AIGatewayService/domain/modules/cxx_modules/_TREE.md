# cxx_modules/ 目录树

> AI 网关路由层的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIRouteModule.cppm` | AI 路由代理目标和超时归一化算法 module。 |
| `AIRouteRegistration.cppm` | AI RouteRegistry 注册使用的 HTTP method 与 `/ai/*` path 字面量 module。 |
| `AIRouteSchema.cppm` | AI 网关 route schema 的 method、path、route name 与 DTO 类型名字面量 module。 |
| `AIGatewayClient.cppm` | AI 网关 gRPC 客户端 AIServer 目标默认值、空字段回退守卫与错误 payload 字面量 module（仅 gtest 消费，生产 TU 因 grpc/protobuf 头 ICE 不作 module consumer）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
