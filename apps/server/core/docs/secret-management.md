# 密钥管理

本文档记录 MemoChat 后端服务的密钥配置与生产注入方式,对应架构评审 finding ⑥
(HmacSecret 跨服务硬编码同一 dev 值)。

## ChatAuth HmacSecret

登录票据(ChatLoginTicket)由 HMAC 签名,签名密钥配置在各服务 ini 的
`[ChatAuth] HmacSecret`。本地开发所有服务共用同一个众所周知的 dev 值
(`memochat-dev-chat-secret`),方便联调。

### 生产环境:环境变量注入

`IniConfig` 支持用环境变量覆盖任意 ini 键,覆盖键名由 `EnvKeyFor(section, key)` 生成,
规则是 `MEMOCHAT_<SECTION>_<KEY>`(大写、非字母数字转下划线)。因此 `[ChatAuth] HmacSecret`
的覆盖环境变量是:

```
MEMOCHAT_CHATAUTH_HMACSECRET
```

生产部署**必须**通过该环境变量注入强随机密钥(由 k8s Secret / 部署管线提供),
不得使用 ini 里的 dev 默认值。

### 运行时防呆

服务启动读取 HmacSecret 时,会用 `memochat::auth::IsWellKnownDevHmacSecret()` 检测是否仍是
dev 默认值;若是,发出一次性 WARN 日志提示"set env MEMOCHAT_CHATAUTH_HMACSECRET"。
此防呆已接入 `AuthLoginSupport`(Account/Register/Login 系)与 `ChatSessionConfig`(ChatServer)
的密钥读取点。

## 注入约定

| 密钥 | ini 位置 | 生产环境变量 |
|------|----------|--------------|
| 登录票据签名 | `[ChatAuth] HmacSecret` | `MEMOCHAT_CHATAUTH_HMACSECRET` |

dev 值保留在 ini 中无妨(仅本地用);所有 ini 的 `HmacSecret=` 行附有注释指向上述环境变量。
