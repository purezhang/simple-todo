# Bug备忘：ReBar工具栏遮挡任务列表表头

## 问题描述

窗口大小调整后，ReBar工具栏遮挡了任务列表的表头行，导致表头不可见。

## 问题症状

- 任务列表的列标题行消失
- 被上方的工具栏覆盖
- 仅在窗口调整大小后出现

## 根本原因

### 变量遮蔽（Variable Shadowing）

在 `CMainFrame::OnSize()` 函数中，错误地在 if 块内重新声明了 `toolbarHeight` 变量：

```cpp
// 错误代码
int toolbarHeight = 0;  // 外层变量声明
if (m_rebar.IsWindow()) {
    m_rebar.MoveWindow(0, 0, rcClient.right, 30);  // 硬编码30px

    RECT rcRebar;
    m_rebar.GetClientRect(&rcRebar);
    int toolbarHeight = rcRebar.bottom - rcRebar.top;  // ⚠️ 创建了新的局部变量！
    // 外层的 toolbarHeight 始终为 0
}

if (m_mainSplitter.IsWindow()) {
    int topOffset = toolbarHeight;  // 这里 toolbarHeight = 0
    m_mainSplitter.MoveWindow(0, topOffset, ...);  // Splitter 从 y=0 开始
    // 被 ReBar 遮挡！
}
```

### 问题分析

| 问题 | 影响 |
|------|------|
| `int toolbarHeight = ...` | 在 if 块内声明了新的局部变量 |
| 外层变量未被赋值 | `topOffset` 始终为 0 |
| Splitter 从 y=0 开始 | 与 ReBar 位置重叠 |
| 硬编码 `30` | 实际 ReBar 高度可能是 28px |

## 解决方案

### 修复代码

```cpp
// 正确代码
int toolbarHeight = 0;  // 外层变量
if (m_rebar.IsWindow()) {
    RECT rcRebarClient;
    m_rebar.GetClientRect(&rcRebarClient);
    toolbarHeight = rcRebarClient.bottom - rcRebarClient.top;  // ✅ 赋值给外层变量

    m_rebar.MoveWindow(0, 0, rcClient.right, toolbarHeight);  // 使用实际高度
}

if (m_mainSplitter.IsWindow()) {
    int topOffset = toolbarHeight;  // ✅ 正确获取 ReBar 高度
    m_mainSplitter.MoveWindow(0, topOffset, clientWidth, clientHeight);
}
```

### 关键修改

1. **移除 `int`**：`toolbarHeight = ...` 不再声明新变量
2. **使用实际高度**：`MoveWindow()` 使用计算出的实际高度
3. **获取正确高度**：使用 `GetClientRect()` 获取 ReBar 的实际客户区高度

## 教训

### 1. 变量声明要小心

在 WTL/ATL 开发中，变量遮蔽是常见错误：

```cpp
// 危险写法
int x = 0;
if (condition) {
    int x = 10;  // 创建了新变量，外层 x 保持为 0
}

// 安全写法
int x = 0;
if (condition) {
    x = 10;  // 正确修改外层变量
}
```

### 2. 硬编码高度不可靠

不同 Windows 版本、主题、DPI 下，控件高度可能不同：

```cpp
// 不可靠
m_rebar.MoveWindow(0, 0, rcClient.right, 30);

// 可靠
RECT rcClientRect;
m_rebar.GetClientRect(&rcClientRect);
int height = rcClientRect.bottom - rcClientRect.top;
m_rebar.MoveWindow(0, 0, rcClient.right, height);
```

### 3. 调试方法

使用 `OutputDebugString` 或 DebugView 调试布局问题：

```cpp
#ifdef _DEBUG
RECT rcRebar;
m_rebar.GetClientRect(&rcRebar);
TCHAR szDebug[256];
_stprintf_s(szDebug, _T("[OnSize] ReBar高度=%d\n"), rcRebar.bottom - rcRebar.top);
::OutputDebugString(szDebug);
#endif
```

## 相关文件

- `src/MainFrm.cpp` - `CMainFrame::OnSize()` 函数
- `src/MainFrm.h` - 类声明

## 发生记录

| 日期 | 分支 | 提交 | 描述 |
|------|------|------|------|
| 2026-02-06 | v0.4.0 | fe80807 | 首次正式修复 |

## 预防措施

1. **代码审查**：变量声明后避免在嵌套块内重新声明同名变量
2. **使用静态分析工具**：Visual Studio 的 /W4 警告级别可捕获变量遮蔽
3. **单元测试**：添加布局相关的集成测试
4. **调试日志**：在 OnSize 等布局函数中添加调试输出

## 参考

- [MSDN: RECT Structure](https://docs.microsoft.com/en-us/windows/win32/api/windef/ns-windef-rect)
- [MSDN: GetClientRect Function](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect)
