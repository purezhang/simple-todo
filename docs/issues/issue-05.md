# 实现置顶按钮状态显示执行计划

## 问题分析

当前的置顶按钮没有状态显示，用户无法直观地知道当前窗口是否处于置顶状态。需要实现置顶按钮的状态显示，区分置顶和非置顶状态。

## UI设计

使用图钉图标，简洁直观：

| 状态 | 按钮显示 |
|------|----------|
| **未置顶** | `📌 窗口置顶` |
| **已置顶** | `📌 取消置顶` |

## 实现步骤

### 步骤1：定义按钮文本

在 `MainFrm.cpp` 顶部或 `OnCreate` 前添加：

```cpp
// 置顶按钮文本
#define TOPMOST_TEXT_NORMAL    _T("📌 窗口置顶")
#define TOPMOST_TEXT_CHECKED   _T("📌 取消置顶")
```

### 步骤2：修改OnToggleTopmost方法

修改 `MainFrm.cpp` 中的 `OnToggleTopmost` 方法：

```cpp
LRESULT CMainFrame::OnToggleTopmost(WORD, WORD, HWND, BOOL&)
{
    m_bTopmost = !m_bTopmost;

    if (m_bTopmost) {
        SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 更新菜单项
        HMENU hMenu = GetMenu();
        if (hMenu) {
            ModifyMenu(hMenu, ID_WINDOW_TOPMOST, MF_BYCOMMAND | MF_STRING,
                ID_WINDOW_TOPMOST, _T("取消置顶(&T)"));
            DrawMenuBar();
        }

        // 更新工具栏按钮
        TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_TEXT, 0, 0,
            (LPTSTR)TOPMOST_TEXT_CHECKED, {0}, 0, 0 };
        m_toolbar.SetButtonInfo(0, &tbbi);
    } else {
        SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 更新菜单项
        HMENU hMenu = GetMenu();
        if (hMenu) {
            ModifyMenu(hMenu, ID_WINDOW_TOPMOST, MF_BYCOMMAND | MF_STRING,
                ID_WINDOW_TOPMOST, _T("窗口置顶(&T)"));
            DrawMenuBar();
        }

        // 更新工具栏按钮
        TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_TEXT, 0, 0,
            (LPTSTR)TOPMOST_TEXT_NORMAL, {0}, 0, 0 };
        m_toolbar.SetButtonInfo(0, &tbbi);
    }

    return 0;
}
```

### 步骤3：初始化时设置按钮状态

在 `MainFrm.cpp` 的 `OnCreate` 方法中，工具栏按钮创建完成后添加：

```cpp
// 在 m_rebar.InsertBand(-1, &rbbi); 之后添加：

// 初始化置顶按钮状态
TBBUTTONINFO tbbiInit = { sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_TEXT, 0, 0,
    (LPTSTR)(m_bTopmost ? TOPMOST_TEXT_CHECKED : TOPMOST_TEXT_NORMAL), {0}, 0, 0 };
m_toolbar.SetButtonInfo(0, &tbbiInit);
```

### 步骤4：更新初始按钮定义（可选）

也可以在初始的 `buttons` 数组中直接设置正确的文本：

```cpp
// 初始时使用 m_bTopmost 判断
LPCTSTR initialText = m_bTopmost ? TOPMOST_TEXT_CHECKED : TOPMOST_TEXT_NORMAL;

TBBUTTON buttons[] = {
    { I_IMAGENONE, ID_WINDOW_TOPMOST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)initialText },
    // ... 其他按钮
};
```

## 预期效果

```
点击前: [📌 窗口置顶] → 点击后: [📌 取消置顶]
点击前: [📌 取消置顶] → 点击后: [📌 窗口置顶]
```

图钉图标直观表示"置顶"概念。
