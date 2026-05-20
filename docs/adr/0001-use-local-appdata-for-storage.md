# ADR 0001: Use LocalAppData for persistent storage

## Status
Accepted

## Context
ServoLink 需要保存用户的项目参数（机构尺寸、角度范围）到本地文件，便于下次启动时恢复。

在 Windows 上，两种候选路径：
- `QStandardPaths::AppDataLocation` → `%APPDATA%\ServoLink\`（Roaming）
- `QStandardPaths::AppLocalDataLocation` → `%LOCALAPPDATA%\ServoLink\`（Local）

## Decision
使用 `AppLocalDataLocation`（Local 目录）。

## Reasons
1. MindPath 的 ADR 0003 经充分讨论后选择了 Local 路径，ServoLink 沿用此策略
2. 机构设计数据为个人项目数据，无需域账号漫游
3. Local 路径下数据不会因域策略问题被意外清空或同步冲突

## Consequences
1. 存储路径：`C:/Users/<用户>/AppData/Local/ServoLink/ServoLink/servolink.json`
2. 不实现跨机同步（用户如需迁移手动拷贝即可）
3. 无 Roaming → Local 的迁移任务（新项目，无历史数据）
