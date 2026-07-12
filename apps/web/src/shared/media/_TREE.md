# media/ 目录树

> Web 端媒体上传、选择和下载 URL 解析辅助。

## 文件

| 文件 | 作用 |
| --- | --- |
| `MediaUploadService.ts` | 封装媒体上传请求。 |
| `filePicker.ts` | 浏览器文件选择辅助。 |
| `mediaUrl.test.ts` | 覆盖同源认证媒体 URL、UUID v4 media key、外部源拒绝和默认头像解析。 |
| `mediaUrl.ts` | 将服务端媒体引用解析成浏览器 URL，并限制 Bearer 请求到可信媒体端点。 |
| `uploadTypes.ts` | 媒体上传相关类型定义。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
