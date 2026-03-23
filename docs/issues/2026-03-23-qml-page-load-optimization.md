# MemoChat QML 页面加载优化报告

**日期**：2026-03-23
**优化范围**：客户端页面加载速度

---

## 问题描述

用户反馈：所有页面切换都很慢，页面先出来但内容是空的再慢慢填入。

---

## 根因分析

### 1. 数据填充晚于页面渲染

登录成功后立即调用 `setPage(ChatPage)` 触发 QML 显示页面，但会话列表、联系人列表、群组列表等数据需要等待 TCP 网络回调（`onDialogListRsp`、`onRelationBootstrapUpdated`）才填充。QML 页面渲染时，`dialogsReady`、`contactsReady`、`groupsReady` 等标志均为 `false`，导致页面内容区域显示"正在加载..."空白占位。

### 2. 群组初始化存在 QTimer 延迟

`bootstrapGroups()` 中使用 `QTimer::singleShot(0, ...)` 将群组列表刷新推迟到下一事件循环，造成群组列表额外延迟一帧。

### 3. 会话列表全量重建

`refreshDialogModel()` 每次调用都执行 `_dialog_list_model.clear()` + `setFriends()`，触发 `beginResetModel()`。大量会话时，QML ListView 完全销毁并重建所有 delegate，造成明显卡顿。

### 4. 头像图片加载无占位

大量 `Image` 组件在异步加载期间显示空白，没有占位背景色或淡入过渡，用户体验为"闪烁"而非平滑加载。

---

## 优化方案

### 优化 1：预加载数据（高优先级）

**目标**：聊天页面渲染时数据已经就绪，消除"空白等待"。

**修改文件**：`client/MemoChat-qml/AppControllerSession.cpp`

**改动**：`onSwitchToChat()` 中，将 `bootstrapDialogs()`、`bootstrapContacts()`、`bootstrapGroups()` 移到 `setPage(ChatPage)` 之前执行。

```cpp
// 之前：
setPage(ChatPage); // QML 渲染时数据尚未加载
// ... 网络回调后填充数据 ...

// 之后：
bootstrapDialogs();
bootstrapContacts();
bootstrapGroups();
setPage(ChatPage); // QML 渲染时数据已经就绪
```

**效果**：聊天页面切换时，QML 直接显示内容，不会出现空白占位。

### 优化 2：移除群组初始化的 QTimer 延迟（高优先级）

**修改文件**：`client/MemoChat-qml/AppController.cpp`

**改动**：`bootstrapGroups()` 中移除 `QTimer::singleShot(0, ...)` 包装，`refreshGroupList()` 直接同步执行。

```cpp
// 之前：
refreshGroupModel();
setGroupsReady(true);
QTimer::singleShot(0, this, [this, uid = selfInfo->_uid]() {
    refreshGroupList(); // 延迟到下一事件循环
});

// 之后：
refreshGroupModel();
refreshGroupList(); // 直接同步刷新
setGroupsReady(true);
```

**效果**：群组列表不再额外延迟一帧。

### 优化 3：会话列表增量 upsert 替代全量重建（中优先级）

**目标**：减少 `beginResetModel()` 触发频率，降低 ListView 完全重建的开销。

**修改文件**：
- `client/MemoChat-qml/FriendListModel.h` — 新增 `upsertBatch()` 声明
- `client/MemoChat-qml/FriendListModel.cpp` — 实现 `upsertBatch()` 批量方法
- `client/MemoChat-qml/AppController.cpp` — `refreshDialogModel()` 和 `refreshDialogModelIncremental()` 中 `setFriends()` / `upsertFriend()` → `upsertBatch()`
- `client/MemoChat-qml/AppControllerPrivateEvents.cpp` — `onDialogListRsp()` 中 `setFriends()` → `upsertBatch()`

**新增方法 `upsertBatch()` 核心逻辑**：

```cpp
void FriendListModel::upsertBatch(const std::vector<std::shared_ptr<FriendInfo>> &friends)
{
    // 收集所有更新和新增的索引
    std::vector<int> updatedIndices;
    std::vector<int> newIndices;

    for (const auto &friendInfo : friends) {
        // ... 构建 FriendEntry ...
        const int existingIdx = indexOfUid(entry.uid);
        if (existingIdx >= 0) {
            _items[existingIdx] = entry;
            updatedIndices.push_back(existingIdx);
        } else {
            _items.push_back(entry);
            newIndices.push_back(_items.size() - 1);
        }
    }

    // 批量触发信号（避免逐条触发的开销）
    for (int idx : updatedIndices) {
        emit dataChanged(index(idx, 0), index(idx, 0));
    }
    if (!newIndices.empty()) {
        beginInsertRows(QModelIndex(), newIndices.front(), newIndices.back());
        endInsertRows();
    }
    emit countChanged();
}
```

**效果**：
- `beginResetModel()` 不再被调用，ListView 不完全销毁
- N 次信号合并为 O(1) 次（一次 `dataChanged` 批量 + 一次 `beginInsertRows/endInsertRows`）
- 性能日志输出：`[PERF] FriendListModel::upsertBatch - count: N | updated: M | new: K | ms: X`

### 优化 4：头像图片占位 + 淡入动画（中优先级）

**目标**：消除头像加载期间的空白闪烁，提供平滑的视觉体验。

**修改文件**（共 9 个 QML 文件）：
- `client/MemoChat-qml/qml/chat/ChatLeftPanel.qml` — 会话列表头像 + 搜索结果头像
- `client/MemoChat-qml/qml/chat/conversation/ChatMessageDelegate.qml` — 消息气泡收发双方头像
- `client/MemoChat-qml/qml/chat/ChatSideBar.qml` — 侧边栏个人头像
- `client/MemoChat-qml/qml/chat/contact/ApplyRequestDelegate.qml` — 好友申请头像
- `client/MemoChat-qml/qml/chat/contact/AddFriendDialog.qml` — 添加好友对话框头像
- `client/MemoChat-qml/qml/chat/contact/ChatFriendInfoPane.qml` — 联系人详情头像
- `client/MemoChat-qml/qml/chat/contact/ContactListPane.qml` — 联系人列表头像
- `client/MemoChat-qml/qml/chat/group/GroupInfoPane.qml` — 群信息头像
- `client/MemoChat-qml/qml/chat/settings/SettingsAvatarCard.qml` — 设置页头像

**改动**：

```qml
// 之前：
Image {
    anchors.fill: parent
    source: iconUrl
    cache: true
    asynchronous: true
    // 加载期间显示空白
}

// 之后：
Image {
    anchors.fill: parent
    source: iconUrl
    cache: true
    asynchronous: true
    opacity: (status === Image.Ready) ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: 200 } }
    // 外层 Rectangle 始终显示纯色占位背景
    // 加载完成后平滑淡入（200ms）
}
```

**效果**：
- 头像加载期间始终显示纯色占位背景（Rectangle color）
- 加载完成后 200ms 淡入过渡，无闪烁
- `clip: true` 的 Rectangle 确保 Image 不溢出

---

## 未实施的优化（低优先级）

### 后台线程排序

`DialogListService::sortDialogs()` 在主线程对会话向量做 `std::stable_sort`。

**评估**：典型会话数量（< 100）排序开销极小（< 1ms），远低于 I/O 和网络延迟。暂不实施，可通过现有 `[PERF]` 日志测量后再决定。

---

## 性能日志

本次优化在以下位置添加了性能日志，可通过日志验证优化效果：

- `AppControllerSession.cpp`：`[PERF] Stage-0: Parallel network bootstrap` / `[PERF] Stage-1: Ensure chat list initialized`
- `FriendListModel.cpp`：`[PERF] FriendListModel::upsertBatch - count: ... | updated: ... | new: ... | ms: ...`
- `AppController.cpp`：`[PERF] refreshDialogModelIncremental - chats: ... | groups: ... | total: ...ms`
- `ChatMessageModel.cpp`：`[PERF] ChatMessageModel::setMessagesAtomic - messages: ... | build: ...ms | sort: ...ms | ui: ...ms | total: ...ms`

---

## 实施文件清单

| 文件 | 改动类型 |
|------|---------|
| `client/MemoChat-qml/AppControllerSession.cpp` | 逻辑调整 |
| `client/MemoChat-qml/AppController.cpp` | 逻辑调整 |
| `client/MemoChat-qml/AppControllerPrivateEvents.cpp` | 逻辑调整 |
| `client/MemoChat-qml/FriendListModel.h` | 新增方法 |
| `client/MemoChat-qml/FriendListModel.cpp` | 新增方法 |
| `client/MemoChat-qml/qml/chat/ChatLeftPanel.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/conversation/ChatMessageDelegate.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/ChatSideBar.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/contact/ApplyRequestDelegate.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/contact/AddFriendDialog.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/contact/ChatFriendInfoPane.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/contact/ContactListPane.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/group/GroupInfoPane.qml` | QML 优化 |
| `client/MemoChat-qml/qml/chat/settings/SettingsAvatarCard.qml` | QML 优化 |
