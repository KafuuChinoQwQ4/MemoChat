# sse/ 目录树

> 浏览器端 SSE 流式请求客户端与解析辅助。

## 文件

| 文件 | 作用 |
| --- | --- |
| `SseStreamClient.js` | `SseStreamClient.ts` 的生成 JS companion。 |
| `SseStreamClient.test.ts` | 覆盖 AI SSE 请求认证头、请求体和流解析契约。 |
| `SseStreamClient.ts` | 通过 fetch POST 消费 text/event-stream，并携带用户认证头。 |
| `parseSseLine.js` | `parseSseLine.ts` 的生成 JS companion。 |
| `parseSseLine.ts` | 解析单行 SSE 字段和值。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
