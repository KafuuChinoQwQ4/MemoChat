# features/ 目录树

> 桌面客户端的全部业务功能模块，按 feature 分层（controller/model/view/service 等）组织。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`agent/`](agent/_TREE.md) | AI 智能体对话、多模型、知识库与文字游戏模块 |
| [`auth/`](auth/_TREE.md) | 登录、注册、找回密码与凭据管理模块 |
| [`call/`](call/_TREE.md) | 音视频通话（LiveKit）模块 |
| [`chat/`](chat/_TREE.md) | 单聊会话与消息收发模块 |
| [`contact/`](contact/_TREE.md) | 联系人列表与好友关系模块 |
| [`group/`](group/_TREE.md) | 群组会话与群成员管理模块 |
| [`moments/`](moments/_TREE.md) | 朋友圈/动态信息流模块 |
| [`pet/`](pet/_TREE.md) | 桌面宠物模块 |
| [`profile/`](profile/_TREE.md) | 个人资料与账户设置模块 |
| [`r18/`](r18/_TREE.md) | R18 内容相关模块 |
| [`settings/`](settings/_TREE.md) | 客户端设置模块 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | 汇总各 feature 的源码与资源到构建系统 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
