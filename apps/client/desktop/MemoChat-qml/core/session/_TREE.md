# session/ 目录树

> 客户端会话数据层：用户、好友、群组、私聊与群聊消息的本地数据结构及统一管理。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `UserContactData.h` | 联系人/好友数据结构定义 |
| `UserGroupData.h` | 群组数据结构定义 |
| `UserMessageData.h` | 消息数据结构定义 |
| `UserMgrApply.cpp` | 好友申请相关管理实现 |
| `UserMgrFriends.cpp` | 好友列表相关管理实现 |
| `UserMgrGroupList.cpp` | 群组列表相关管理实现 |
| `UserMgrGroupMessages.cpp` | 群聊消息相关管理实现 |
| `UserMgrPrivateMessages.cpp` | 私聊消息相关管理实现 |
| `userdata.cpp` | 用户数据实现 |
| `userdata.h` | 用户数据声明 |
| `usermgr.cpp` | 用户管理器核心实现 |
| `usermgr.h` | 用户管理器声明（会话状态聚合入口） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
