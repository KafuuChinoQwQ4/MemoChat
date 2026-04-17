# MCP 工具桥接使用指南

## 概述

MCP (Model Context Protocol) 桥接层允许 MemoChat AI Agent 接入社区生态的预置 MCP Server，扩展工具能力。

## 启用 MCP

在 `config.yaml` 中设置：

```yaml
mcp:
  enabled: true
  servers:
    - name: filesystem
      command: ["npx", "-y", "@modelcontextprotocol/server-filesystem"]
      args: ["/tmp/memochat"]
      enabled: true
```

## 协议说明

MCP 协议基于 JSON-RPC 2.0，通过 stdio 与 MCP Server 通信：

1. **初始化**: 发送 `initialize` 请求，接收 `protocolVersion` 确认
2. **工具发现**: 调用 `tools/list` 获取可用工具
3. **工具调用**: 调用 `tools/call` 并传入 `name` 和 `arguments`
4. **结果返回**: MCP Server 返回 `{"content": [{"text": "..."}]}` 格式结果

## 可用 MCP Server

| Server | NPM 包 | 用途 | 启用条件 |
|--------|--------|------|---------|
| filesystem | @modelcontextprotocol/server-filesystem | 本地文件访问 | 需指定允许目录 |
| github | @modelcontextprotocol/server-github | GitHub 操作 | 需配置 GITHUB_TOKEN |
| slack | @modelcontextprotocol/server-slack | Slack 消息 | 需配置 SLACK_TOKEN |
| Brave Search | @modelcontextprotocol/server-brave-search | Web 搜索 | 需配置 BRAVE_API_KEY |

## 安装 MCP Server

```bash
# 安装 filesystem server
npx -y @modelcontextprotocol/server-filesystem /tmp/memochat

# 安装 github server
export GITHUB_TOKEN=your_github_token_here
npx -y @modelcontextprotocol/server-github

# 安装 brave search server
export BRAVE_API_KEY=your_brave_api_key_here
npx -y @modelcontextprotocol/server-brave-search
```

## 测试 MCP 连接

```bash
# 手动测试 filesystem
npx -y @modelcontextprotocol/server-filesystem /tmp

# 检查 AIOrchestrator 日志
# 看到 "mcp.server.ready" 表示连接成功
# 看到 "mcp.tool.registered" 表示工具已注册
```

## 工具使用示例

启用后，AI Agent 可以使用以下工具：

### filesystem

- `read_file`: 读取文件内容
- `write_file`: 写入文件内容
- `list_directory`: 列出目录内容

### github

- `get_repository`: 获取仓库信息
- `create_issue`: 创建 Issue
- `search_code`: 搜索代码

### brave_search

- `brave_web_search`: 搜索网页

## 故障排查

### Q1: MCP Server 无法启动

检查 Node.js 和 npm 是否安装：
```bash
node --version
npm --version
```

### Q2: "MCP Server 已断开连接"

MCP Server 进程可能崩溃，查看日志定位原因。

### Q3: 工具注册数量为 0

检查 config.yaml 中 `enabled: true` 是否正确设置。

## 性能注意事项

- MCP Server 作为子进程运行，启用的服务器越多，资源消耗越大
- 建议仅启用必要的 MCP Server
- 可以通过环境变量传递敏感信息（如 API Token）

## 安全注意事项

- filesystem server 务必指定受限的允许目录
- 不要在 config.yaml 中明文存储 API Token
- 建议使用环境变量方式传递敏感信息