# qml/ 目录树

> 测试客户端 qml/ 界面层的运行时与边界契约（大面板边界、滚动窗口交接、启动性能等），并下分各界面分区。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | 主窗口/聊天外壳运行时契约测试 |
| [`auth/`](auth/_TREE.md) | 登录注册界面契约测试 |
| [`chat/`](chat/_TREE.md) | 聊天界面（含群组）契约测试 |
| [`pet/`](pet/_TREE.md) | 桌宠界面契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `test_qml_large_pane_boundary.py` | 校验大型面板的拆分边界契约 |
| `test_qml_scroll_window_handoff.py` | 校验滚动窗口的交接行为契约 |
| `test_ui_startup_performance_contract.py` | 校验 UI 启动性能契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
