# MemoChat AI Agent 端到端测试指南

## 测试前准备

### 1. 服务启动检查

验证以下服务已启动并可访问：

| 服务 | 端口 | 检查命令 |
|------|------|---------|
| GateServer | 8080 | `curl http://localhost:8080/health` |
| AIServer | 8095 | `telnet localhost 8095` |
| AIOrchestrator | 8096 | `curl http://localhost:8096/health` |
| PostgreSQL | 5432 | `psql` 连接 |
| Redis | 6379 | `redis-cli ping` |
| Ollama | 11434 | `curl http://localhost:11434/api/tags` |
| Qdrant | 6333 | `curl http://localhost:6333/collections` |

### 2. 依赖安装

```bash
# Python 依赖
cd server/AIOrchestrator
pip install -r requirements.txt

# 拉取 Ollama 模型
ollama pull qwen2.5:7b
ollama pull nomic-embed-text
```

---

## 测试用例

### TC-AI-001: 普通对话

**步骤：**
1. 启动客户端并登录
2. 点击侧边栏「AI 助手」Tab
3. 输入 "你好，请介绍一下自己"
4. 等待 AI 回复

**预期结果：**
- AI 在 30 秒内回复
- 回复内容为中文
- 消息显示在对话区域

### TC-AI-002: 会话管理

**步骤：**
1. 在 AI 助手页面点击「新会话」
2. 发送一条消息
3. 再点击「新会话」创建第二个会话
4. 切换回第一个会话

**预期结果：**
- 两个会话独立存在
- 切换后显示正确的历史消息
- 会话列表正确显示

### TC-AI-003: 聊天摘要

**步骤：**
1. 在聊天页面（普通聊天，非 AI）连续发送 10+ 条消息
2. 点击智能工具栏的「摘要」按钮
3. 等待处理

**预期结果：**
- 返回 50 字以内的摘要
- 摘要准确反映对话主题

### TC-AI-004: 智能回复建议

**步骤：**
1. 在私聊页面点击「💡 建议回复」按钮
2. 等待 AI 生成建议

**预期结果：**
- 返回 3 条回复建议
- 每条建议不超过 20 字
- 建议内容与对话上下文相关

### TC-AI-005: 消息翻译

**步骤：**
1. 在聊天中长按任意一条英文消息
2. 选择「翻译」选项
3. 选择目标语言「中文」

**预期结果：**
- 返回中文翻译结果
- 显示在原消息下方

### TC-AI-006: 知识库上传

**步骤：**
1. 进入 AI 助手 → 知识库
2. 点击「上传文档」
3. 选择一个 PDF/TXT/MD 文件
4. 等待上传完成

**预期结果：**
- 上传成功
- 文档显示在知识库列表
- 状态为「已就绪」

### TC-AI-007: 知识库检索

**步骤：**
1. 在知识库页面搜索框输入查询
2. 点击搜索

**预期结果：**
- 返回相关文档片段
- 显示来源和相关性分数

### TC-AI-008: 模型切换

**步骤：**
1. 在 AI 助手页面点击「设置」
2. 选择不同的模型后端
3. 发送消息验证

**预期结果：**
- 模型成功切换
- 新模型生成的回复

### TC-AI-009: 错误处理

**步骤：**
1. 断开 Ollama 服务
2. 尝试发送消息

**预期结果：**
- 系统显示友好错误提示
- 不崩溃
3. 重新启动 Ollama 后功能恢复

---

## 性能基准

| 指标 | 目标值 |
|------|--------|
| TTFT（首 token 时间）| < 3s |
| 端到端延迟（非流式）| < 10s |
| 知识库检索延迟 | < 500ms |

---

## 常见问题排查

### Q1: AI 回复为空
- 检查 Ollama 是否运行：`ollama list`
- 检查模型是否已下载：`ollama pull qwen2.5:7b`
- 检查 AIOrchestrator 日志

### Q2: 知识库上传失败
- 检查 Qdrant 是否运行：`docker ps | grep qdrant`
- 检查文件大小（限制 10MB）
- 检查文件格式（支持 pdf/txt/md/docx）

### Q3: 流式输出无响应
- 检查 SSE 端点：`curl -X POST http://localhost:8096/chat/stream`
- 检查 GateServer 日志

---

## 测试报告模板

```
测试日期：_______
测试人员：_______
环境版本：_______

| 用例ID | 用例名称 | 结果 | 备注 |
|--------|----------|------|------|
| TC-AI-001 | 普通对话 | PASS/FAIL | |
| TC-AI-002 | 会话管理 | PASS/FAIL | |
| TC-AI-003 | 聊天摘要 | PASS/FAIL | |
| TC-AI-004 | 智能回复建议 | PASS/FAIL | |
| TC-AI-005 | 消息翻译 | PASS/FAIL | |
| TC-AI-006 | 知识库上传 | PASS/FAIL | |
| TC-AI-007 | 知识库检索 | PASS/FAIL | |
| TC-AI-008 | 模型切换 | PASS/FAIL | |
| TC-AI-009 | 错误处理 | PASS/FAIL | |
```
