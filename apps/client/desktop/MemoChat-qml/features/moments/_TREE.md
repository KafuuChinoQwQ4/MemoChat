# moments/ 目录树

> 客户端朋友圈 feature 模块：动态发布、信息流、评论与媒体展示的完整 MVC + 解析层。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`controller/`](controller/_TREE.md) | 朋友圈控制器：信息流、发布、评论的请求与响应 |
| [`model/`](model/_TREE.md) | 朋友圈动态数据模型与读写逻辑 |
| [`parsing/`](parsing/_TREE.md) | 动态条目/响应/发布载荷的解析与序列化 |
| [`resources/`](resources/_TREE.md) | 朋友圈模块 QML 资源清单 |
| [`runtime/`](runtime/_TREE.md) | 朋友圈视图运行时 JS 逻辑 |
| [`view/`](view/_TREE.md) | 朋友圈信息流、详情、发布等 QML 视图 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | 汇总本模块源码、资源到构建系统 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
