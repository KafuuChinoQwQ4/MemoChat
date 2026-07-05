# dev/ 目录树

> 开发期辅助脚本：源码格式化、桌宠冒烟与整链路聊天运行时冒烟。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `browser_chat_smoke_common.mjs` | 浏览器聊天 smoke 脚本共用的 Gate 登录、endpoint、Playwright 和证书 hash helper |
| `browser_websocket_smoke.mjs` | 使用 Playwright 浏览器验证 WebSocket 聊天登录、心跳、关系拉取和可选私聊链路 |
| `browser_webtransport_smoke.mjs` | 使用 Playwright 浏览器预检和验证 WebTransport 聊天帧链路 |
| `format_sources.py` | 批量格式化源码文件的脚本 |
| `pet_smoke.ps1` | 桌宠功能冒烟测试脚本 |
| `runtime_smoke_full_chat.py` | 全链路聊天运行时冒烟测试脚本 |
| `webtransport_provider_preflight.mjs` | 检查可选 libwebsockets WebTransport provider 的 vcpkg、安装、CMake 和 overlay patch 门禁状态 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
