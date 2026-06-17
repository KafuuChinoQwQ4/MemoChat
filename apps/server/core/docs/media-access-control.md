# 媒体访问控制威胁模型

本文档记录 MediaService 媒体下载的访问控制设计与威胁模型,对应架构评审 finding ②
(媒体下载越权)的纵深防御决策。

## 当前模型:capability URL + 审计

媒体资源的访问控制建立在**不可猜测的 capability key** 之上,而非严格的属主校验:

- 每个媒体资产有一个 `media_key`,采用 **UUIDv4** 生成(122 位随机熵)。下载 URL 形如
  `/media/download?asset=<media_key>`。UUIDv4 的不可枚举性是实际的访问门槛——不持有
  该 key 的第三方无法构造有效下载请求。
- `memo_media.chat_media_asset` 表记录每个资产的 `owner_uid`(上传者)。**但下载路径
  不做 `owner_uid == 调用者 uid` 的硬校验**。

### 为什么不做属主硬校验

聊天/朋友圈媒体本质是**共享**的:接收方用自己的 uid+token 下载发送方的图片,
朋友圈图片对可见范围内所有人开放。`chat_media_asset` 中**没有**分享/授权表
(share/grant table),只有 `owner_uid`。因此 `owner_uid != uid → 403` 会破坏
最常见的合法场景(聊天收图、朋友圈看图)。

### 纵深防御:跨属主下载审计

下载时若 `asset.owner_uid != 调用者 uid`,服务**不拦截**,但发出结构化 WARN 审计日志
(`media.download.cross_owner`,字段含 viewer uid、owner_uid、media_id、media_key),
使跨属主访问可观测,而不破坏共享。

## 未来强化路径(非当前 bug)

若未来需要严格授权,可选:

1. 引入 `media_grant` 表,记录 `(media_id, grantee_uid)` 或 `(media_id, scope)`,下载时校验。
2. 签名下载凭证:对 `media_key|uid|exp` 做 HMAC,下载 URL 带签名,服务端验签 + 校验过期。

这两者都是产品决策,需要客户端配合改造下载 URL 构造逻辑,不在 finding ② 范围内。
当前 capability URL + 审计的组合对实验性平台是合理取舍。
