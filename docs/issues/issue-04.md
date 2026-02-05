# 修复Release版本中工具栏按钮不显示的问题执行计划

## 问题分析

用户报告在Release版本中，复制到其他设备上运行时，顶部的置顶按钮都没有了。通过检查代码，发现了以下可能的问题：

1. **ReBar大小设置问题**：在创建工具栏时，ReBar的`cx`参数被硬编码为200，这可能导致工具栏宽度不够，无法显示所有按钮。

2. **坐标计算问题**：在`OnSize`方法中，使用`GetWindowRect`获取ReBar的大小，然后计算`toolbarHeight`。但`GetWindowRect`返回的是屏幕坐标，不是客户端坐标，这可能导致计算出的高度不正确。

3. **DebugLog函数调用**：在Release版本中，`DebugLog`函数仍然会被调用，虽然这不会导致崩溃，但可能会影响性能。

## 解决方案

### 1. 修改ReBar大小设置

将ReBar的`cx`参数从硬编码的200改为更合理的值，或者使用`RBBS_GRIPPERALWAYS`样式让ReBar自动调整大小。

### 2. 修正坐标计算

在`OnSize`方法中，使用`GetClientRect`而不是`GetWindowRect`来获取ReBar的大小，这样可以得到正确的客户端坐标。

### 3. 优化DebugLog函数

在Release版本中，添加条件编译，禁用`DebugLog`函数的控制台输出部分，只保留`OutputDebugString`部分。

## 实现步骤

### 步骤1：修改工具栏创建代码

修改`MainFrm.cpp`中的工具栏创建代码，调整ReBar的大小设置：

```cpp
// 将工具栏加入 ReBar
REBARBANDINFO rbbi = { sizeof(REBARBANDINFO) };
rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
rbbi.hwndChild = m_toolbar;
rbbi.cxMinChild = 0;
rbbi.cyMinChild = 24;
rbbi.cx = -1; // 使用-1让ReBar自动调整宽度
m_rebar.InsertBand(-1, &rbbi);
```

### 步骤2：修正OnSize方法中的坐标计算

修改`MainFrm.cpp`中的`OnSize`方法，使用`GetClientRect`获取ReBar的大小：

```cpp
// 调整工具栏容器 ReBar
int toolbarHeight = 0;
if (m_rebar.IsWindow()) {
    // 让 ReBar 占据顶部
    m_rebar.MoveWindow(0, 0, rcClient.right, 30);
    
    RECT rcRebar;
    m_rebar.GetClientRect(&rcRebar); // 使用GetClientRect获取客户端坐标
    toolbarHeight = rcRebar.bottom - rcRebar.top;
    
    // 调试日志
    TCHAR szDebug[64];
    _stprintf_s(szDebug, _T("OnSize: ToolbarHeight=%d\n"), toolbarHeight);
    ::OutputDebugString(szDebug);
}
```

### 步骤3：优化DebugLog函数

修改`MainFrm.cpp`中的`DebugLog`函数，添加条件编译：

```cpp
void DebugLog(const TCHAR* format, ...) {
    // 输出到DebugView
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), format, args);
    va_end(args);
    OutputDebugString(buffer);
    
#ifdef _DEBUG
    // 只在Debug版本中输出到控制台
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsole(hConsole, buffer, (DWORD)_tcslen(buffer), &written, NULL);
    }
#endif
}
```

## 预期效果

修复后，在Release版本中，工具栏按钮应该能够正常显示，包括置顶按钮。
