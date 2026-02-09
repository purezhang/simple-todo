# Issues List

SimpleTodo 项目的任务和 issue 列表。

## 编号规则

- **已完成**: issues-01 ~ issues-08
- **待处理**: issues-09 ~ issues-11, issues-18

## Issues 列表

| ID | 标题 | 状态 | 优先级 | 相关文档 |
|----|------|------|--------|----------|
| [issues-01](./issue-01.md) | 修复删除任务时总是删除第一个任务的bug | 已完成 | - | - |
| [issues-02](./issue-02.md) | 编译0.1版本的Release版本 | 已完成 | - | - |
| [issues-03](./issue-03.md) | 创建v0.2.0分支用于开发新功能 | 已完成 | - | - |
| [issues-04](./issue-04.md) | 修复Release版本中工具栏按钮不显示的问题 | 已完成 | 高 | [issue-04.md](./issue-04.md) |
| [issues-05](./issue-05.md) | 实现置顶按钮状态显示 | 已完成 | 中 | [issue-05.md](./issue-05.md) |
| [issues-06](./issue-06.md) | 实现弹出层详情面板 | 已完成 | 中 | [issue-06.md](./issue-06.md) |
| [issues-07](./issue-07.md) | 为TodoList空白处添加双击和右键点击功能 | 待处理 | 中 | [issue-07.md](./issue-07.md) |
| [issues-08](../memos/toolbar-fix.md) | 修复Release版本中工具栏不显示的问题 | 已完成 | 高 | |
| [issues-09](./issue-09.md) | 实现删除任务备份功能 | 待处理 | 中 | - |
| [issues-10](./issue-10.md) | 实现存单任务清单功能 | 待处理 | 中 | - |
| [issues-11](./issue-11.md) | 实现任务分类（Project）功能 | 待处理 | 中 | [issue-11.md](./issue-11.md) |
| [issues-18](./issue-18.md) | 截止时间和P0优先级颜色显示 | 待处理 | 中 | [issue-18.md](./issue-18.md) |
| [issues-19](./issue-19.md) | 完整国际化（中英文切换）重构 | 待处理 | 高 | [issue-19.md](./issue-19.md) |

## 新建 Issue

新建 issue 时：

1. 在 `docs/issues/` 目录下创建 `issue-XX.md` 文件
2. 在 `issues-list.md` 中添加条目
3. 编号递增，保持连续性

## Issue 模板

```markdown
# Issue-XX: [标题]

## 问题描述

[描述问题或需求]

## 解决方案

[解决方案概述]

## 实现步骤

### 步骤1：xxx

[具体步骤]

### 步骤2：xxx

[具体步骤]

## 预期效果

[描述实现后的效果]

## 备注

[其他说明]
```

---

最后更新: 2026-02-09 (v0.6.0 分支已创建，国际化重构开始)
