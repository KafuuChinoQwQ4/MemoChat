# MemoChat-qml/ 目录树

> MemoChat QML 客户端的 Python 契约测试根，镜像客户端分层并校验整体源码布局。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | 应用层（连接/控制器/外壳）契约测试 |
| [`features/`](features/_TREE.md) | 各业务功能模块契约测试 |
| [`live2d/`](live2d/_TREE.md) | Live2D 资源契约测试 |
| [`qml/`](qml/_TREE.md) | QML 界面运行时/边界契约测试 |
| [`shared/`](shared/_TREE.md) | 客户端共享组件（媒体）契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_memochat_qml_core_layout.py` | 校验客户端核心源码与测试目录的物理布局契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
