# components/ 目录树

> R18 页面级 React 组件。

## 文件

| 文件 | 作用 |
| --- | --- |
| `R18ShellContent.module.css` | R18 页面窄屏布局、筛选条与主题适配弹层（章节/账号/图片阅读器/视频播放器）样式。 |
| `R18ShellContent.test.ts` | 锁定可操作源、图片失败态、Hanime1 站内播放交互与 Web 媒体安全策略。 |
| `R18ShellContent.tsx` | R18 页面源码，提供成人授权 gate、内容源搜索/筛选、无限加载、章节/账号弹层及图片/视频阅读入口。 |
| `R18VideoPlayerOverlay.tsx` | Hanime1 全屏站内播放器，负责临时地址解析、原生播放、画质切换与错误兜底。 |
| `r18SourceAvailability.ts` | 统一判断源是否可操作以及账号管理交互类型。 |
| `r18SearchPagination.ts` | 漫画源列表无限滚动：按上游单页加载、触底判定、分页去重合并。 |
| `r18SearchPagination.test.ts` | 锁定上游单页无限滚动启发式与去重合并。 |
| `r18SourceFilters.ts` | 各官方源排序 / 分类 / 扩展官方 tag 筛选配置，及本地 tag 搜索过滤。 |
| `r18SourceFilters.test.ts` | 锁定各源筛选选项映射。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
