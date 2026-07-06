# Web 前端模块

Web 前端位于 `apps/web/`，使用 Vite、React、TypeScript、React Router、Zustand、React Query、Dexie 和 Vitest。

## 目录职责

| 目录 | 职责 |
| --- | --- |
| `src/app/` | Provider、bootstrap、组合容器、端口绑定、入站分发 |
| `src/routes/` | Hash Router、鉴权 guard、主 shell 和 tab 路由 |
| `src/features/` | auth、chat、contact、group、moments、agent、profile、settings 等业务 UI 和 store |
| `src/core/` | HTTP、聊天传输、协议编解码、session、entity store、持久化 |
| `src/shared/` | gateway、通用 hooks、UI、媒体上传、工具函数 |
| `src/theme/` | 全局样式、主题 token、亮暗色 |
| `src/test/` | MSW、测试 mock 和测试 setup |

## 运行与网络

Vite dev server 代理 HTTP API 到 `https://localhost:8443`，并把 `/ws` 代理到本地聊天入口 `localhost:8444`。运行时配置在 `src/core/config/runtimeConfig.ts`：

- `VITE_GATE_BASE_URL`
- `VITE_MEDIA_BASE_URL`
- `VITE_AI_BASE_URL`
- `VITE_WS_BRIDGE_URL`
- `VITE_PREFER_WEBTRANSPORT`
- `VITE_USE_MOCK_TRANSPORT`

登录链路在 `src/app/bootstrap/postLoginBootstrap.ts`：HTTP `/user_login` 成功后保存 session，再根据 `chat_endpoints` 建立 WebTransport 或 WebSocket 聊天传输，随后拉取关系 bootstrap。

## 开发边界

- `App` 只组合 Provider 和 Router，不写业务。
- `features/*` 写业务 UI 和局部 store，不跨 feature 导入对方实现。
- `core/network` 只处理传输、协议、HTTP、SSE，不依赖 feature。
- 入站聊天帧通过 dispatcher 按 `ReqId` 分发，再由 app 层注册的路由写入 entity store。
