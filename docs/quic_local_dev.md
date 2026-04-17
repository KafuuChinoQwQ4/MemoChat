# QUIC 本地开发指南

当需要实际的 `msquic` 支持时，使用动态 vcpkg 预设。

## 构建

仅服务端：

```powershell
cmake --preset msvc2022-vcpkg-server-msquic
cmake --build --preset msvc2022-vcpkg-server-msquic-release
```

客户端和服务端：

```powershell
cmake --preset msvc2022-all-msquic
cmake --build --preset msvc2022-msquic-release
```

## 开发证书

生成本地 PEM 证书和私钥：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\new_dev_quic_cert.ps1
```

然后将生成的路径复制到以下文件的 `[Quic]` 部分：

- `server/ChatServer/config.ini`
- `server/ChatServer/chatserver1.ini`
- `server/ChatServer/chatserver2.ini`
- `server/ChatServer/chatserver3.ini`
- `server/ChatServer/chatserver4.ini`

必需的配置项：

```ini
[Quic]
Alpn=memochat-chat
CertFile=D:\MemoChat-Qml-Drogon\artifacts\certs\quic-dev\server-cert.pem
PrivateKeyFile=D:\MemoChat-Qml-Drogon\artifacts\certs\quic-dev\server-key.pem
```

## 启动

```powershell
scripts\windows\start_test_services.bat --enable-quic
```

如果 QUIC 握手或流设置失败，客户端将自动回退到 TCP。
