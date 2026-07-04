# model/ 目录树

> 联系人数据模型：好友列表、好友申请、搜索结果的状态与读写逻辑。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ApplyRequestModel.cpp` | 好友申请列表模型实现 |
| `ApplyRequestModel.h` | 好友申请列表模型声明 |
| `FriendListModel.cpp` | 好友列表模型主实现（Qt model 基础接口） |
| `FriendListModel.h` | 好友列表模型声明 |
| `FriendListModelOrdering.cpp` | 好友列表条目合并、分组排序与派生字段逻辑 |
| `FriendListModelMutations.cpp` | 好友列表的增删改变更逻辑 |
| `FriendListModelQueries.cpp` | 好友列表的查询/检索逻辑 |
| `FriendListModelState.cpp` | 好友列表内部状态管理 |
| `SearchResultModel.cpp` | 用户搜索结果模型实现 |
| `SearchResultModel.h` | 用户搜索结果模型声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
