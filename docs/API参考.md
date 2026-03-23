# MemoChat API 参考文档

> 文档版本：2026-03-23

---

## 一、API 概览

### 1.1 Base URL

| 环境 | Base URL |
|------|----------|
| 本地开发 | `http://127.0.0.1:8080` |
| 开发环境 | `https://api-dev.memochat.example.com` |
| 预发布环境 | `https://api-staging.memochat.example.com` |
| 生产环境 | `https://api.memochat.example.com` |

### 1.2 认证方式

MemoChat API 使用 Token 认证：

| 认证方式 | 说明 | 适用场景 |
|---------|------|----------|
| Login Ticket | HTTP 登录后获取的票据 | 初始化会话 |
| Token | 登录响应中返回的 Token | 后续请求验证 |

### 1.3 请求格式

- **Content-Type**: `application/json`
- **Character Encoding**: UTF-8
- **Request Body**: JSON 格式

### 1.4 响应格式

```json
{
  "error": 0,
  "message": "success",
  "data": { }
}
```

### 1.5 错误码

| 错误码 | 说明 |
|--------|------|
| 0 | 成功 |
| 1001 | 参数错误 |
| 1002 | 验证码错误 |
| 1003 | 验证码过期 |
| 1004 | 用户不存在 |
| 1005 | 密码错误 |
| 1006 | 用户已存在 |
| 1007 | 登录票据无效 |
| 1008 | 登录票据过期 |
| 2001 | ChatServer 不可达 |
| 2002 | 消息发送失败 |
| 2003 | 好友关系不存在 |
| 2004 | 群组不存在 |
| 2005 | 权限不足 |
| 3001 | 媒体文件过大 |
| 3002 | 媒体格式不支持 |
| 4001 | 通话房间不存在 |
| 4002 | 通话已结束 |
| 5001 | 服务器内部错误 |

---

## 二、认证 API

### 2.1 获取验证码

获取邮箱验证码，用于注册或重置密码。

**请求**

```
POST /get_varifycode
Content-Type: application/json

{
  "email": "user@example.com"
}
```

**响应**

```json
{
  "error": 0,
  "message": "验证码已发送到邮箱"
}
```

**错误响应**

```json
{
  "error": 1006,
  "message": "用户已存在"
}
```

### 2.2 用户注册

注册新用户账号。

**请求**

```
POST /user_register
Content-Type: application/json

{
  "email": "user@example.com",
  "verify_code": "123456",
  "password": "password123",
  "name": "johndoe"
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| email | string | 是 | 邮箱地址 |
| verify_code | string | 是 | 验证码（6 位数字） |
| password | string | 是 | 密码（6-20 位） |
| name | string | 是 | 用户名（4-20 位，字母数字下划线） |

**响应**

```json
{
  "error": 0,
  "message": "注册成功",
  "data": {
    "uid": 12345,
    "user_id": "U12345"
  }
}
```

### 2.3 用户登录

登录用户账号。

**请求**

```
POST /user_login
Content-Type: application/json

{
  "email": "user@example.com",
  "password": "password123"
}
```

**响应**

```json
{
  "error": 0,
  "message": "登录成功",
  "data": {
    "protocol_version": 3,
    "preferred_transport": "quic",
    "fallback_transport": "tcp",
    "uid": 12345,
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    "login_ticket": "eyJ0b2tlbl90eXBlIjoiY2hhdCIsInVpZCI6MTIzNDUsImV4cCI6MTcx...",
    "ticket_expire_ms": 1711180800000,
    "chat_endpoints": [
      {
        "transport": "quic",
        "host": "127.0.0.1",
        "port": 8190,
        "server_name": "chatserver1",
        "priority": 0
      },
      {
        "transport": "tcp",
        "host": "127.0.0.1",
        "port": 8090,
        "server_name": "chatserver1",
        "priority": 1
      }
    ],
    "user_profile": {
      "uid": 12345,
      "user_id": "U12345",
      "name": "johndoe",
      "nick": "John Doe",
      "icon": "",
      "desc": "",
      "email": "user@example.com",
      "sex": 0
    }
  }
}
```

### 2.4 重置密码

通过验证码重置密码。

**请求**

```
POST /reset_pwd
Content-Type: application/json

{
  "email": "user@example.com",
  "verify_code": "123456",
  "new_password": "newpassword123"
}
```

**响应**

```json
{
  "error": 0,
  "message": "密码重置成功"
}
```

---

## 三、用户资料 API

### 3.1 更新用户资料

更新当前用户的个人资料。

**请求**

```
POST /user_update_profile
Content-Type: application/json
Authorization: Bearer {token}

{
  "nick": "John Doe",
  "icon": "https://example.com/avatar/12345.jpg",
  "desc": "Hello, I'm John!"
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| nick | string | 否 | 昵称（最多 50 字符） |
| icon | string | 否 | 头像 URL |
| desc | string | 否 | 个人签名（最多 100 字符） |

**响应**

```json
{
  "error": 0,
  "message": "资料更新成功"
}
```

### 3.2 获取用户资料

获取指定用户的资料。

**请求**

```
POST /user_get_profile
Content-Type: application/json
Authorization: Bearer {token}

{
  "uid": 67890
}
```

**响应**

```json
{
  "error": 0,
  "data": {
    "uid": 67890,
    "user_id": "U67890",
    "name": "janedoe",
    "nick": "Jane Doe",
    "icon": "https://example.com/avatar/67890.jpg",
    "desc": "Hello!",
    "email": "jane@example.com",
    "sex": 2
  }
}
```

---

## 四、好友关系 API

### 4.1 搜索用户

通过用户名或用户 ID 搜索用户。

**请求**

```json
{
  "search_key": "john"
}
```

**响应**

```json
{
  "error": 0,
  "data": {
    "users": [
      {
        "uid": 12345,
        "user_id": "U12345",
        "name": "johndoe",
        "nick": "John Doe",
        "icon": "https://example.com/avatar/12345.jpg",
        "sex": 1
      }
    ]
  }
}
```

### 4.2 申请添加好友

向其他用户发送好友申请。

**请求**

```json
{
  "to_uid": 67890,
  "verify_msg": "我是 John，想加你为好友"
}
```

**响应**

```json
{
  "error": 0,
  "message": "好友申请已发送"
}
```

### 4.3 处理好友申请

同意或拒绝好友申请。

**请求**

```json
{
  "from_uid": 67890,
  "accept": true,
  "back": "Jane"
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| from_uid | integer | 是 | 申请方用户 ID |
| accept | boolean | 是 | 是否同意 |
| back | string | 否 | 备注名 |

**响应**

```json
{
  "error": 0,
  "message": "已同意好友申请"
}
```

### 4.4 获取好友列表

获取当前用户的好友列表。

**响应**

```json
{
  "error": 0,
  "data": {
    "friends": [
      {
        "uid": 67890,
        "user_id": "U67890",
        "name": "janedoe",
        "nick": "Jane Doe",
        "icon": "https://example.com/avatar/67890.jpg",
        "back": "Jane",
        "sex": 2
      }
    ]
  }
}
```

### 4.5 删除好友

删除指定好友。

**请求**

```json
{
  "friend_id": 67890
}
```

**响应**

```json
{
  "error": 0,
  "message": "已删除好友"
}
```

---

## 五、群组 API

### 5.1 创建群组

创建一个新的群组。

**请求**

```json
{
  "name": "Test Group",
  "member_uids": [67890, 11111, 22222],
  "icon": "https://example.com/group/icon.jpg"
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| name | string | 是 | 群名称（2-20 字符） |
| member_uids | array | 是 | 初始成员用户 ID 列表 |
| icon | string | 否 | 群头像 URL |

**响应**

```json
{
  "error": 0,
  "data": {
    "group_id": 10001,
    "group_code": "G10001"
  }
}
```

### 5.2 获取群列表

获取当前用户所在的群组列表。

**响应**

```json
{
  "error": 0,
  "data": {
    "groups": [
      {
        "group_id": 10001,
        "group_code": "G10001",
        "name": "Test Group",
        "icon": "https://example.com/group/icon.jpg",
        "owner_uid": 12345,
        "member_count": 4,
        "role": 3
      }
    ]
  }
}
```

**role 字段说明**

| 值 | 说明 |
|----|------|
| 1 | 普通成员 |
| 2 | 管理员 |
| 3 | 群主 |

### 5.3 获取群成员

获取指定群组的成员列表。

**请求**

```json
{
  "group_id": 10001
}
```

**响应**

```json
{
  "error": 0,
  "data": {
    "group_id": 10001,
    "name": "Test Group",
    "members": [
      {
        "uid": 12345,
        "user_id": "U12345",
        "name": "johndoe",
        "nick": "John Doe",
        "icon": "https://example.com/avatar/12345.jpg",
        "role": 3,
        "join_at": 1711180800000
      }
    ]
  }
}
```

### 5.4 申请加入群组

申请加入指定群组。

**请求**

```json
{
  "group_id": 10001,
  "reason": "想加入这个群"
}
```

**响应**

```json
{
  "error": 0,
  "message": "入群申请已发送"
}
```

### 5.5 处理入群申请

群主或管理员处理入群申请。

**请求**

```json
{
  "group_id": 10001,
  "applicant_uid": 33333,
  "accept": true
}
```

**响应**

```json
{
  "error": 0,
  "message": "已同意入群申请"
}
```

### 5.6 退出群组

退出指定群组。

**请求**

```json
{
  "group_id": 10001
}
```

**响应**

```json
{
  "error": 0,
  "message": "已退出群组"
}
```

### 5.7 解散群组

群主解散指定群组。

**请求**

```json
{
  "group_id": 10001
}
```

**响应**

```json
{
  "error": 0,
  "message": "群组已解散"
}
```

---

## 六、聊天消息 API

### 6.1 获取对话列表

获取当前用户的对话列表。

**响应**

```json
{
  "error": 0,
  "data": {
    "dialogs": [
      {
        "dialog_type": "private",
        "peer_uid": 67890,
        "peer_user": {
          "uid": 67890,
          "nick": "Jane Doe",
          "icon": "https://example.com/avatar/67890.jpg"
        },
        "last_msg_preview": "Hello!",
        "last_msg_ts": 1711180800000,
        "unread_count": 2
      },
      {
        "dialog_type": "group",
        "group_id": 10001,
        "group": {
          "group_id": 10001,
          "name": "Test Group",
          "icon": "https://example.com/group/icon.jpg"
        },
        "last_msg_preview": "John: Hi everyone!",
        "last_msg_ts": 1711180700000,
        "unread_count": 0
      }
    ]
  }
}
```

### 6.2 获取私聊历史

获取与指定用户的私聊消息历史。

**请求**

```json
{
  "peer_uid": 67890,
  "before_msg_id": "",
  "limit": 20
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| peer_uid | integer | 是 | 对方用户 ID |
| before_msg_id | string | 否 | 上一页最后一条消息 ID |
| limit | integer | 否 | 每页消息数（默认 20，最大 100） |

**响应**

```json
{
  "error": 0,
  "data": {
    "messages": [
      {
        "msg_id": "msg-uuid-123",
        "from_uid": 12345,
        "to_uid": 67890,
        "content": "Hello!",
        "msg_type": "text",
        "created_at": 1711180800000,
        "status": 2
      }
    ],
    "has_more": true
  }
}
```

### 6.3 获取群聊历史

获取指定群组的消息历史。

**请求**

```json
{
  "group_id": 10001,
  "before_msg_id": "",
  "limit": 20
}
```

**响应**

```json
{
  "error": 0,
  "data": {
    "messages": [
      {
        "msg_id": "msg-uuid-456",
        "group_id": 10001,
        "from_uid": 12345,
        "from_user": {
          "uid": 12345,
          "nick": "John Doe",
          "icon": "https://example.com/avatar/12345.jpg"
        },
        "content": "Hi everyone!",
        "msg_type": "text",
        "created_at": 1711180800000
      }
    ],
    "has_more": true
  }
}
```

### 6.4 发送私聊消息

通过 TCP/QUIC 连接发送私聊消息（参考协议文档）。

### 6.5 发送群聊消息

通过 TCP/QUIC 连接发送群聊消息（参考协议文档）。

---

## 七、文件上传下载 API

### 7.1 上传媒体文件

上传图片、音频、视频或其他文件。

**请求**

```
POST /api/media/upload
Content-Type: multipart/form-data
Authorization: Bearer {token}

file: [文件数据]
type: image
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| file | file | 是 | 上传的文件 |
| type | string | 是 | 文件类型 image/video/file/audio |

**响应**

```json
{
  "error": 0,
  "data": {
    "media_id": "media-uuid-789",
    "media_key": "2026/03/23/media-uuid-789",
    "url": "https://cdn.example.com/media/2026/03/23/media-uuid-789",
    "size": 102400,
    "mime": "image/jpeg"
  }
}
```

### 7.2 下载媒体文件

下载指定的媒体文件。

**请求**

```
GET /api/media/download/{media_id}
Authorization: Bearer {token}
```

**响应**

```
HTTP/1.1 200 OK
Content-Type: image/jpeg
Content-Length: 102400

[文件二进制数据]
```

---

## 八、音视频通话 API

### 8.1 开始通话

向指定用户发起音视频通话邀请。

**请求**

```json
{
  "callee_uid": 67890,
  "call_type": "video"
}
```

**参数说明**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| callee_uid | integer | 是 | 被叫方用户 ID |
| call_type | string | 是 | 通话类型 audio/video |

**响应**

```json
{
  "error": 0,
  "data": {
    "call_id": "call-uuid-123",
    "room_name": "memochat_call-call-uuid-123",
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    "expires_at": 1711181100000
  }
}
```

### 8.2 接受通话

接受通话邀请。

**请求**

```json
{
  "call_id": "call-uuid-123"
}
```

**响应**

```json
{
  "error": 0,
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    "expires_at": 1711181100000
  }
}
```

### 8.3 拒绝通话

拒绝通话邀请。

**请求**

```json
{
  "call_id": "call-uuid-123"
}
```

**响应**

```json
{
  "error": 0,
  "message": "已拒绝通话"
}
```

### 8.4 取消通话

取消正在响铃的通话。

**请求**

```json
{
  "call_id": "call-uuid-123"
}
```

**响应**

```json
{
  "error": 0,
  "message": "通话已取消"
}
```

### 8.5 挂断通话

结束正在进行的通话。

**请求**

```json
{
  "call_id": "call-uuid-123"
}
```

**响应**

```json
{
  "error": 0,
  "message": "通话已结束",
  "data": {
    "duration_sec": 120
  }
}
```

---

## 九、系统 API

### 9.1 健康检查

检查服务健康状态。

**请求**

```
GET /healthz
```

**响应**

```json
{
  "status": "ok"
}
```

### 9.2 就绪检查

检查服务就绪状态。

**请求**

```
GET /readyz
```

**响应**

```json
{
  "status": "ready",
  "services": {
    "postgres": "ok",
    "redis": "ok",
    "kafka": "ok",
    "mongodb": "ok"
  }
}
```

---

## 十、WebSocket 实时 API

### 10.1 连接建立

```
wss://api.memochat.example.com/ws?token={token}&uid={uid}
```

### 10.2 接收消息格式

```json
{
  "type": "message",
  "data": {
    "msg_id": "msg-uuid-123",
    "from_uid": 67890,
    "content": "Hello!",
    "created_at": 1711180800000
  }
}
```

### 10.3 消息类型

| type | 说明 |
|------|------|
| message | 收到新消息 |
| notice | 系统通知 |
| friend_request | 好友申请 |
| group_invite | 群组邀请 |
| call_invite | 通话邀请 |
| presence | 用户在线状态变更 |

---

## 十一、错误处理

### 11.1 错误响应格式

```json
{
  "error": 1001,
  "message": "参数错误",
  "details": {
    "field": "email",
    "reason": "邮箱格式不正确"
  }
}
```

### 11.2 重试策略

| 错误类型 | 重试次数 | 重试间隔 |
|---------|---------|---------|
| 网络错误 | 3 | 1s, 2s, 4s |
| 服务器错误 (5xx) | 2 | 1s, 2s |
| 限流 (429) | 5 | 1s, 2s, 4s, 8s, 16s |

### 11.3 限流说明

| 接口 | 限制 |
|------|------|
| /get_varifycode | 5 次/分钟 |
| /user_login | 10 次/分钟 |
| 其他 API | 100 次/分钟 |

---

## 十二、SDK 示例

### 12.1 Python SDK 示例

```python
import requests

class MemoChatClient:
    def __init__(self, base_url):
        self.base_url = base_url
        self.token = None
        self.uid = None
    
    def login(self, email, password):
        resp = requests.post(
            f"{self.base_url}/user_login",
            json={"email": email, "password": password}
        )
        data = resp.json()
        if data["error"] == 0:
            self.token = data["data"]["token"]
            self.uid = data["data"]["uid"]
        return data
    
    def get_friends(self):
        headers = {"Authorization": f"Bearer {self.token}"}
        resp = requests.post(
            f"{self.base_url}/friend_list",
            headers=headers
        )
        return resp.json()
    
    def send_message(self, to_uid, content):
        # 使用 TCP/QUIC 连接发送
        pass
```

### 12.2 JavaScript SDK 示例

```javascript
class MemoChatClient {
  constructor(baseUrl) {
    this.baseUrl = baseUrl;
    this.token = null;
    this.uid = null;
  }

  async login(email, password) {
    const resp = await fetch(`${this.baseUrl}/user_login`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password })
    });
    const data = await resp.json();
    if (data.error === 0) {
      this.token = data.data.token;
      this.uid = data.data.uid;
    }
    return data;
  }

  async getFriends() {
    const resp = await fetch(`${this.baseUrl}/friend_list`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${this.token}`
      }
    });
    return resp.json();
  }
}
```
