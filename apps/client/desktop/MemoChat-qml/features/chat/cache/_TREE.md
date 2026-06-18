# cache/ 目录树

> 聊天消息的本地缓存层：私聊/群聊缓存存储及其行数据编解码。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatCacheMessageCodec.cpp` | 缓存行（PrivateChatCacheRow 等）与消息结构互相编解码 |
| `ChatCacheMessageCodec.h` | 缓存消息编解码接口与缓存行结构定义 |
| `GroupChatCacheStore.cpp` | 群聊消息本地缓存存储实现 |
| `GroupChatCacheStore.h` | 群聊缓存存储 GroupChatCacheStore 类声明 |
| `PrivateChatCacheStore.cpp` | 私聊消息本地缓存存储实现 |
| `PrivateChatCacheStore.h` | 私聊缓存存储 PrivateChatCacheStore 类声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
