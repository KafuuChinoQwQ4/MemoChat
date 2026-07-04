# h3/ 目录树

> 将 HTTP/3 请求与响应适配到 GateShared 统一路由注册表的适配器。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`modules/`](cxx_modules/H3RouteAdapter.cppm) | H3 路由适配器 guard/默认 content-type/文件缺失元数据算法 module |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `H3RouteAdapter.cpp` | HTTP/3 路由适配器实现，导入算法 module 处理连接 guard、默认 content-type 与文件体转换。 |
| `H3RouteAdapter.h` | HTTP/3 路由适配器接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
