# pet/ 目录树

> 客户端桌面宠物 feature 模块：Live2D 角色的渲染、聊天、语音、视觉与窗口控制。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`assets/`](assets/_TREE.md) | 宠物头像/资源设置与路径解析 |
| [`controller/`](controller/_TREE.md) | 宠物控制器：会话、网络、状态管理 |
| [`model/`](model/_TREE.md) | 宠物数据模型 |
| [`platform/`](platform/_TREE.md) | 平台相关桥接（Windows 窗口） |
| [`resources/`](resources/_TREE.md) | 宠物模块 QML 资源清单 |
| [`runtime/`](runtime/_TREE.md) | Live2D/聊天/窗口运行时 JS 逻辑 |
| [`speech/`](speech/_TREE.md) | 语音合成与语音训练运行时 |
| [`view/`](view/_TREE.md) | 宠物窗口、聊天、Live2D 设置 QML 视图 |
| [`vision/`](vision/_TREE.md) | 摄像头视觉帧编码与采集 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | 汇总本模块源码、资源到构建系统 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
