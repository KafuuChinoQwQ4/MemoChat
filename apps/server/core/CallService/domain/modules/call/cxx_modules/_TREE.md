# cxx_modules/ 目录树

> Call 路由模块 C++ modules 接口单元，承载路由层轻量纯算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallRouteModule.cppm` | 导出 Call route registration 的方法/路径常量，以及 uid 查询解析与 trace_id 追加边界判定算法。 |
| `CallRouteResponse.cppm` | 导出 Call 路由响应的 HTTP 状态码、JSON/token content type 与 trace_id 探测字面量算法。 |
| `CallRouteSchema.cppm` | 导出 Call 路由 schema 使用的 method、path、route name 与 DTO type-name 字面量算法。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
