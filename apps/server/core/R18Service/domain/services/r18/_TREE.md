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
| `R18EhentaiAdapter.cpp` | e-hentai 官方源分类（f_cats）/附加 tag 搜索与详情适配实现 |
| `R18EhentaiAdapter.hpp` | e-hentai 官方源适配接口声明 |
| `R18PublicDtos.cpp` | R18 公开接口请求 DTO 的兼容解析实现 |
| `R18PublicDtos.hpp` | R18 公开接口请求 DTO 声明 |
| `R18Service.cpp` | R18 业务服务实现；全局源变更要求 Bearer 与独立 source-admin key，并转发 JM 图片反打乱元数据 |
| `R18Service.hpp` | R18 业务服务声明 |
| `R18SourceRecordCodec.cpp` | R18 内容源记录持久化编解码及隐藏内部路径的公开 DTO 实现 |
| `R18SourceRecordCodec.hpp` | R18 内容源记录 JSON DTO 声明 |
| `R18SourceService.cpp` | 有界 JavaScript 源暂存、导入大小合同、全局源状态持久化、启用状态调度保护及 JM 图片元数据转发实现 |
| `R18SourceService.hpp` | R18 内容源接入服务、跨 target 导入大小及 JM 图片元数据接口声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
