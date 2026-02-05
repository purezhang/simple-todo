# 工具栏不显示问题解决备忘

## 问题描述

Release 版本编译后，在本机运行正常，但拷贝到其他设备上时，工具栏完全不显示（窗口置顶、展开全部、收起全部、添加任务 等按钮全部消失）。

## 问题根因

工具栏 (`CToolBarCtrl`) 需要 **comctl32.dll v6.0** 才能正确渲染。问题设备上缺少必要的控件初始化和 DLL 加载，导致工具栏窗口创建成功但内容不可见。

## 解决步骤

### 1. 代码修复 - 核心修复 (`src/SimpleTodo.cpp`)

这是**真正解决问题**的修改：

```cpp
// 初始化 ATL 控件
AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES);

// 强制加载 comctl32.dll v6.0 确保工具栏和 ReBar 在所有设备上正确显示
HMODULE hComCtl = LoadLibrary(L"comctl32.dll");
if (hComCtl) {
    // 不释放库，让程序在整个生命周期中使用 v6.0
}
```

**关键点**：
- `ICC_BAR_CLASSES` - 工具栏和状态栏
- `ICC_WIN95_CLASSES` - 菜单、按钮等 Win95 风格控件
- `ICC_STANDARD_CLASSES` - 标准控件类
- `LoadLibrary(L"comctl32.dll")` - **最关键的修复**，强制加载 v6.0，即使 manifest 未解析也能工作

### 2. 编译脚本增强 - 最佳实践 (`scripts/build.ps1`)

这是**辅助措施**，确保 manifest 正确嵌入：

```powershell
# 嵌入 manifest 以确保 Common Controls 6.0 正确加载
& $mtPath -manifest $manifestPath -outputresource:"$outputExe;#1"
```

**是否必须？** 不是必须，但建议保留，属于 Windows 应用打包的最佳实践。

## 验证方法

1. 编译后在目标设备运行
2. 用记事本打开 exe，搜索 `assemblyIdentity`，确认包含：
   ```xml
   <assemblyIdentity
       type="win32"
       name="Microsoft.Windows.Common-Controls"
       version="6.0.0.0"
       ...
   />
   ```

## 预防措施

1. **确保 manifest 嵌入 exe** - 使用 mt.exe 或 Visual Studio 配置
2. **代码中显式加载 comctl32.dll** - 作为 manifest 的后备方案
3. **正确初始化控件类** - `AtlInitCommonControls` 包含所有需要的类

## 相关文件

- `src/SimpleTodo.cpp` - 添加 LoadLibrary 调用
- `scripts/build.ps1` - 添加 manifest 嵌入步骤
- `src/SimpleTodo.exe.manifest` - 声明 Common Controls 6.0 依赖

## 解决日期

2026-02-04
