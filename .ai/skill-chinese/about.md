# Skill 中文化

MemoChat 项目中的 agent 说明和 skill 文档已经统一为中文说明，同时保留原有命令、路径、端口、front matter 字段和工作流语义。后续又补充了 Agent ACID 原则，用于约束原子化提交、一致性验证、并发隔离和关键知识持久化；并补充文档同步规则，要求代码、配置、运行时、接口、UI、部署或 agent/skill 行为变化时同步更新相关文档。

该 skill 体系已经融合 Learn Harness Engineering 的核心方法：仓库作为唯一事实来源、五子系统 harness、冷启动可恢复、WIP=1、外部化完成定义、三层终止闸门、失败归因、任务轨迹和双层可观测性。后续 agent 处理 MemoChat 任务时，应以 `.ai` 产物承载状态和证据，以 Docker/MCP 和构建/测试/运行时信号证明完成，而不是依赖聊天记录或主观判断。
