# 媒体访问控制威胁模型

本文档记录 MediaService 媒体下载的访问控制设计与威胁模型,对应安全审计中
“媒体下载越权”的修复边界。

## 当前模型:认证用户 + owner/grant 授权

媒体资源下载不再把 `media_key` 当作访问能力令牌。`media_key` 仍使用 UUIDv4 生成,
只作为不可枚举的对象定位符;真正的访问控制由登录 token 与数据库授权共同决定。

- 下载必须提供有效 `uid/token`。
- 旧版 `file=` 下载路径已禁用;下载必须使用 `asset=<media_key>` 并命中 active metadata。
- `chat_media_asset.owner_uid == uid` 时允许下载。
- `owner_uid != uid` 时,必须存在未撤销的 `chat_media_access_grant` 授权:单用户
  `grantee_uid=<viewer>`、公开 `grantee_uid=0/grant_scope='public'`、好友
  `grantee_uid=0/grant_scope='friends'` 或群聊 `grantee_uid=0/grant_scope='group:<id>'`。
- 未授权跨属主下载返回 `media access denied`,并只记录 viewer uid、owner_uid 与 media_id。
- 下载日志不得记录 `media_key`、`storage_path`、S3 object key 或本地绝对路径。

## Grant 发放

`chat_media_access_grant` 是跨用户共享的唯一服务端授权来源。私聊上传写入对端 uid grant;
朋友圈公开/好友可见上传写入 public/friends audience grant;群聊上传写入 `group:<id>` audience
grant。下载时仍会校验登录 token,并在关系表可用时校验好友或群成员关系,不能依赖裸
`media_key` 泄漏给客户端后自然可读。

当前 owner 读取是隐式授权;显式 grant 用于跨属主读取,并支持通过 `revoked_at_ms` 撤销。
拆分后的 media schema 如果没有好友或群成员投影表,friends/group audience 会保守拒绝,
需要由关系服务同步成员投影或改为服务端签发的短期下载 token。

## 纵深防御

- JSON/base64 上传在 decode 前检查请求 body 和 encoded payload 长度。超出 simple JSON
  上限的对象必须走分片上传。
- 本地存储拒绝绝对路径、`..` 段和 canonical 后逃逸 upload root 的路径。
- MinIO/S3 `PublicUrl` tokenless redirect 默认关闭。只有显式配置
  `AllowPublicRedirect=true` 或 `1` 时,storage adapter 才会返回 public URL。
- 下载响应设置 `X-Content-Type-Options: nosniff`、`Content-Disposition: attachment`
  和保守 content type。
- 存储后端错误只写服务端日志;客户端只收到通用 `file not found`。

## 剩余产品工作

严格 owner/grant 模型会要求所有跨用户媒体展示路径同步发放 grant。私聊和朋友圈发布链路
已经在上传时发放 grant;群聊和好友可见下载在拆分部署下还需要关系/成员投影,或由关系服务
在消息持久化、群成员变更和动态可见性变更时补齐/撤销授权。
