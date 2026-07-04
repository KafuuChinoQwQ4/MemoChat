# legacy_routes/ 目录树

> HTTP/3 兼容路由装配，将遗留 H3 路径转接到统一 RouteRegistry。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | H3 legacy 路由表和未命中响应元数据的 C++ modules。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateHttp3ServiceRoutes.cpp` | H3 兼容路由装配与 module-backed 统一路由转发实现。 |
| `GateHttp3ServiceRoutes.h` | H3 兼容路由装配声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
