# QUIC Local Dev

Use the dynamic vcpkg presets when you want actual `msquic` support.

## Build

Server only:

```powershell
cmake --preset msvc2022-vcpkg-server-msquic
cmake --build --preset msvc2022-vcpkg-server-msquic-release
```

Client and server:

```powershell
cmake --preset msvc2022-all-msquic
cmake --build --preset msvc2022-msquic-release
```

## Dev Certificate

Generate a local PEM certificate and private key:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\windows\new_dev_quic_cert.ps1
```

Then copy the generated paths into the `[Quic]` section of:

- `server/ChatServer/config.ini`
- `server/ChatServer/chatserver1.ini`
- `server/ChatServer/chatserver2.ini`
- `server/ChatServer/chatserver3.ini`
- `server/ChatServer/chatserver4.ini`

Required keys:

```ini
[Quic]
Alpn=memochat-chat
CertFile=D:\MemoChat-Qml-Drogon\artifacts\certs\quic-dev\server-cert.pem
PrivateKeyFile=D:\MemoChat-Qml-Drogon\artifacts\certs\quic-dev\server-key.pem
```

## Start

```powershell
scripts\windows\start_test_services.bat --enable-quic
```

If QUIC handshake or stream setup fails, the client will fall back to TCP automatically.
