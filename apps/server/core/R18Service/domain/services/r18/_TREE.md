# r18/ 目录树

> R18 业务服务：实现带独立运维鉴权的全局内容源控制面与受限官方源数据面。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | R18 内容源服务 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `R18AdapterUtils.cpp` | R18 官方适配器共享的 HTTP、加密、图片代理与占位图工具实现 |
| `R18AdapterUtils.hpp` | R18 官方适配器共享工具声明 |
| `R18JmAdapter.cpp` | JMComic 官方源搜索、详情、章节、图片代理及章节图反打乱实现 |
| `R18JmAdapter.hpp` | JMComic 官方源适配与图片反打乱参数接口声明 |
| `R18PicacgAdapter.cpp` | Picacg 官方源适配及精确域名、公网解析、固定端点和响应边界保护的图片代理实现 |
| `R18PicacgAdapter.hpp` | Picacg 官方源适配接口声明 |
| `R18NhentaiAdapter.cpp` | nHentai 官方源搜索（sort/tag）、详情与图片代理适配实现 |
| `R18NhentaiAdapter.hpp` | nHentai 官方源适配接口声明 |
| `R18EhentaiAdapter.cpp` | e-hentai / exhentai 官方源分类（f_cats）/ 附加 tag 搜索、详情、论坛账密登录与 Cookie 校验实现 |
| `R18EhentaiAdapter.hpp` | e-hentai / exhentai 官方源适配接口声明 |
| `R18Hanime1Adapter.cpp` | Hanime1 官方源搜索、详情、章节封面及短期签名视频源严格解析实现 |
| `R18Hanime1Adapter.hpp` | Hanime1 官方源适配与短期视频描述符解析接口声明 |
| `R18PublicDtos.cpp` | R18 公开接口请求 DTO 解析及视频解析必填字段校验实现 |
| `R18PublicDtos.hpp` | R18 公开接口请求 DTO（含视频解析请求）声明 |
| `R18Service.cpp` | R18 业务服务实现；全局源变更要求 Bearer 与独立 source-admin key，并为视频解析复用 Bearer/R18 门禁与禁缓存响应 |
| `R18Service.hpp` | R18 业务服务及视频解析处理器声明 |
| `R18SourceRecordCodec.cpp` | R18 内容源记录持久化编解码及隐藏内部路径的公开 DTO 实现 |
| `R18SourceRecordCodec.hpp` | R18 内容源记录 JSON DTO 声明 |
| `R18SourceService.cpp` | 有界 JavaScript 源暂存、全局源状态持久化、启用状态调度保护及 Hanime1 用户级视频解析分发实现 |
| `R18SourceService.hpp` | R18 内容源接入服务、图片获取及用户级视频解析接口声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
