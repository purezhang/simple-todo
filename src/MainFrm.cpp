#include "stdafx.h"
#include <commctrl.h>
#include "MainFrm.h"
#include "AddTodoDlg.h"
#include "SQLiteManager.h"

// 调试日志函数
void DebugLog(const TCHAR* format, ...) {
    // 输出到DebugView
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), format, args);
    va_end(args);
    OutputDebugString(buffer);
    
    // 同时输出到控制台
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsole(hConsole, buffer, (DWORD)_tcslen(buffer), &written, NULL);
    }
}

CMainFrame::CMainFrame()
    : m_nSelectedIndex(-1), m_bSelectedIsDone(false), m_bChineseLanguage(true)
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'N' &&
        ::GetKeyState(VK_CONTROL) < 0) {
        // Ctrl+N - 添加任务
        PostMessage(WM_COMMAND, ID_TODO_ADD);
        return TRUE;
    }

    return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    // 不使用工具栏，不需要调用 UIUpdateToolBar
    // UIUpdateToolBar();
    return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
    // 只在 Debug 版本中分配控制台用于调试输出
#ifdef _DEBUG
    // 分配控制台用于调试输出
    AllocConsole();
    SetConsoleTitle(_T("SimpleTodo Debug Console"));
    
    // 重定向标准输出
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    
    // Startup banner - always visible
    DebugLog(_T("===============================================\n"));
    DebugLog(_T("SimpleTodo Application Starting\n"));
    DebugLog(_T("Build: " __DATE__ " " __TIME__ "\n"));
    DebugLog(_T("===============================================\n"));
    DebugLog(_T("Console allocated successfully\n"));
#endif

    // 加载菜单
    HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
    if (hMenu) {
        SetMenu(hMenu);
        DebugLog(_T("Menu loaded successfully\n"));
    } else {
        DebugLog(_T("ERROR: Failed to load menu\n"));
    }

    // 创建 UI 资源（字体和行高控制）
    // 1. 获取系统标准消息字体 (通常是 Segoe UI 9pt)
    // 这样可以确保字号恢复正常，不人为放大
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    m_fontList.CreateFontIndirect(&ncm.lfMessageFont);

    // 2. 创建 ImageList 用于撑开行高 (1px宽, 20px高)
    // 之前是 24px，现在配合标准字号稍微减小一点，保持紧凑但有呼吸感
    m_imgList.Create(1, 20, ILC_COLOR32, 0, 0);

    // 创建工具栏容器 ReBar
    m_rebar.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS);

    // 创建工具栏
    m_toolbar.Create(m_rebar, rcDefault, NULL, 
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
        TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN, 
        0, ATL_IDW_TOOLBAR);
    
    m_toolbar.SetButtonStructSize();
    
    // 添加按钮
    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_WINDOW_TOPMOST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"窗口置顶" },
        { I_IMAGENONE, ID_VIEW_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"展开全部" },
        { I_IMAGENONE, ID_VIEW_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"收起全部" },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_TODO_ADD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"添加任务" }
    };

    m_toolbar.AddButtons(5, buttons);
    m_toolbar.AutoSize();

    // 将工具栏加入 ReBar
    REBARBANDINFO rbbi = { sizeof(REBARBANDINFO) };
    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
    rbbi.hwndChild = m_toolbar;
    rbbi.cxMinChild = 0;
    rbbi.cyMinChild = 24;
    rbbi.cx = 200;
    m_rebar.InsertBand(-1, &rbbi);

    // 创建主分割器（水平分割：上待办，下底部容器）
    // 注意：不要赋值给 m_hWndClient，防止 CFrameWindowImpl 自动布局覆盖我们
    m_mainSplitter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 创建底部分割器（垂直分割：左已完成，右详情）
    m_bottomSplitter.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 创建 Todo 列表 (位于主分割器上方)
    m_todoList.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,
        WS_EX_CLIENTEDGE);
    m_todoList.SetFont(m_fontList); 
    m_todoList.SetImageList(m_imgList, LVSIL_SMALL); 
    m_todoList.SetDataManager(&m_dataManager);
    m_todoList.SetIsDoneList(false);

    // 创建 Done 列表 (位于底部分割器左侧)
    m_doneList.Create(m_bottomSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,
        WS_EX_CLIENTEDGE);
    m_doneList.SetFont(m_fontList); 
    m_doneList.SetImageList(m_imgList, LVSIL_SMALL); 
    m_doneList.SetDataManager(&m_dataManager);
    m_doneList.SetIsDoneList(true);

    // 创建右侧详情面板 (位于底部分割器右侧)
    m_detailPanel.Create(m_bottomSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        WS_BORDER, WS_EX_CLIENTEDGE);

    // 设置主分割器面板 (上：Todo，下：底部分割器)
    m_mainSplitter.SetSplitterPanes(m_todoList, m_bottomSplitter);
    
    // 设置底部分割器面板 (左：Done，右：详情)
    m_bottomSplitter.SetSplitterPanes(m_doneList, m_detailPanel);
    
    // 设置分割条样式
    m_mainSplitter.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
    m_bottomSplitter.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);

    // 初始位置将在 OnSize 中设置，这里先设个默认值
    m_mainSplitter.SetSplitterPos(-1);
    m_bottomSplitter.SetSplitterPos(-1);

    // 创建详情面板中的控件
    CreateDetailPanelControls();
    
    // 初始化详情面板为空状态
    UpdateDetailPanel(-1, false);

    // 创建状态栏
    m_statusBar.Create(m_hWnd, rcDefault, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0);
    m_statusBar.SetSimple(FALSE);
    int parts[] = { 400, -1 };
    m_statusBar.SetParts(2, parts);
    m_statusBar.SetText(0, _T("就绪"), 0);

    // 设置列表
    SetupLists();

    // 加载数据
    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        BOOL bLoaded = dbManager.LoadAll(m_dataManager);
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("LoadAll result=%d, todoCount=%zu, doneCount=%zu\n"),
            bLoaded, m_dataManager.todoItems.size(), m_dataManager.doneItems.size());
        ::OutputDebugString(szDebug);

        // 如果数据库为空，添加测试数据
        if (m_dataManager.todoItems.empty() && m_dataManager.doneItems.empty()) {
            ::OutputDebugString(_T("Database is empty, adding test data...\n"));

            CTime now = CTime::GetCurrentTime();
            CTime yesterday = now - CTimeSpan(1, 0, 0, 0);
            CTime dayBefore = now - CTimeSpan(2, 0, 0, 0);

            // 添加4个测试项
            TodoItem item1;
            item1.id = m_dataManager.nextId++;
            item1.priority = Priority::P0;
            item1.title = _T("紧急任务：完成项目修复");
            item1.note = _T("修复 ListView 分组显示问题");
            item1.createTime = dayBefore;
            item1.targetEndTime = now;
            item1.isDone = false;
            m_dataManager.todoItems.push_back(item1);

            TodoItem item2;
            item2.id = m_dataManager.nextId++;
            item2.priority = Priority::P1;
            item2.title = _T("编写测试用例");
            item2.note = _T("覆盖主要功能模块");
            item2.createTime = yesterday;
            item2.targetEndTime = now + CTimeSpan(1, 0, 0, 0);
            item2.isDone = false;
            m_dataManager.todoItems.push_back(item2);

            TodoItem item3;
            item3.id = m_dataManager.nextId++;
            item3.priority = Priority::P2;
            item3.title = _T("代码审查");
            item3.note = _T("审查新功能代码");
            item3.createTime = now;
            item3.targetEndTime = now + CTimeSpan(2, 0, 0, 0);
            item3.isDone = false;
            m_dataManager.todoItems.push_back(item3);

            TodoItem item4;
            item4.id = m_dataManager.nextId++;
            item4.priority = Priority::P3;
            item4.title = _T("更新文档");
            item4.note = _T("更新用户手册");
            item4.createTime = now;
            item4.targetEndTime = now + CTimeSpan(3, 0, 0, 0);
            item4.isDone = false;
            m_dataManager.todoItems.push_back(item4);

            // 保存测试数据
            dbManager.SaveAll(m_dataManager);

            _stprintf_s(szDebug, _T("Added %zu test items\n"), m_dataManager.todoItems.size());
            ::OutputDebugString(szDebug);
        }
    } else {
        ::OutputDebugString(_T("Database Initialize failed\n"));
    }

    // 立即刷新列表（数据已加载）
    ::OutputDebugString(_T("立即刷新列表...\n"));
    UpdateLists();
    
    // 设置列宽自动调整，确保任务描述列能填满剩余空间
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE_USEHEADER);
    
    // 确保任务描述列占据剩余空间
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE);

    // 设置定时器延迟刷新，确保窗口完全准备好后再次刷新
    SetTimer(2000, 200, nullptr);     // 200ms 后再次强制刷新

    // 强制触发一次布局计算，确保工具栏不遮挡
    PostMessage(WM_SIZE);

    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    // 清理所有定时器
    ::KillTimer(m_hWnd, 2000);
    ::KillTimer(m_hWnd, 1001);

    // 保存数据
    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        dbManager.SaveAll(m_dataManager);
    }

    // 发送退出消息，终止消息循环
    ::PostQuitMessage(0);

    return 0;
}

LRESULT CMainFrame::OnAppRefresh(UINT, WPARAM, LPARAM, BOOL&)
{
    ::OutputDebugString(_T("OnAppRefresh: Refreshing lists...\n"));

    // 调试：检查列表状态
    TCHAR szDebug[512];

    // 检查 Todo 列表
    if (m_todoList.IsWindow()) {
        DWORD style = m_todoList.GetStyle();
        DWORD exStyle = m_todoList.GetExStyle();
        _stprintf_s(szDebug, _T("  m_todoList: HWND=0x%08X, style=0x%08X, exStyle=0x%08X\n"),
            (UINT_PTR)m_todoList.m_hWnd, style, exStyle);
        ::OutputDebugString(szDebug);

        // 检查 LVS_OWNERDATA
        BOOL hasOwnerData = (style & LVS_OWNERDATA) != 0;
        _stprintf_s(szDebug, _T("  LVS_OWNERDATA=%d\n"), hasOwnerData);
        ::OutputDebugString(szDebug);

        // 检查分组视图
        int groupCount = ListView_GetGroupCount(m_todoList);
        _stprintf_s(szDebug, _T("  groupCount=%d\n"), groupCount);
        ::OutputDebugString(szDebug);

        // 检查项目数量
        int itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  GetItemCount=%d\n"), itemCount);
        ::OutputDebugString(szDebug);

        // 强制设置项目数量
        m_todoList.SetItemCountEx(m_dataManager.GetItemCount(false), LVSICF_NOSCROLL);
        itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  After SetItemCountEx: GetItemCount=%d\n"), itemCount);
        ::OutputDebugString(szDebug);
    } else {
        ::OutputDebugString(_T("  m_todoList is not a window!\n"));
    }

    UpdateLists();
    return 0;
}

LRESULT CMainFrame::OnSize(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    // 完全接管布局，防止基类干扰
    bHandled = TRUE;

    RECT rcClient;
    GetClientRect(&rcClient);
    
    // 调整工具栏容器 ReBar
    int toolbarHeight = 0;
    if (m_rebar.IsWindow()) {
        // 让 ReBar 占据顶部，高度先给个预估值，然后通过 GetWindowRect 获取真实高度
        // 注意：ReBar 如果有 CCS_TOP 样式，通常会自动调整高度，但我们需要手动 MoveWindow 确保宽度正确
        m_rebar.MoveWindow(0, 0, rcClient.right, 30);
        
        RECT rcRebar;
        m_rebar.GetWindowRect(&rcRebar);
        toolbarHeight = rcRebar.bottom - rcRebar.top;
        
        // 调试日志
        TCHAR szDebug[64];
        _stprintf_s(szDebug, _T("OnSize: ToolbarHeight=%d\n"), toolbarHeight);
        ::OutputDebugString(szDebug);
    }

        // 2. 调整分割器外部容器的大小
        if (m_mainSplitter.IsWindow()) {
            // 从 ReBar 下方开始，额外增加 2px 避免遮挡表头边框
            int topOffset = toolbarHeight;
            int clientHeight = rcClient.bottom - topOffset;
            int clientWidth = rcClient.right - rcClient.left;
            
            m_mainSplitter.MoveWindow(0, topOffset, clientWidth, clientHeight);
    
            // 3. 处理初始分割位置
            if (m_bFirstSize && clientHeight > 100 && clientWidth > 100) {
                // 主分割器 (上下)：待办列表占 60%
                m_mainSplitter.SetSplitterPos((int)(clientHeight * 0.6));
                
                // 底部分割器 (左右)：已完成列表占 70%
                m_bottomSplitter.SetSplitterPos((int)(clientWidth * 0.7));
                
                            m_bFirstSize = false; // 仅执行一次
                        }
                    }
                
                    // 调整状态栏
                    if (m_statusBar.IsWindow()) {
                        m_statusBar.MoveWindow(&rcClient);
                    }
                
                    return 0;
                }
LRESULT CMainFrame::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    if (wParam == 1001) {
        // 清除状态栏文本
        m_statusBar.SetText(0, _T(""), 0);
        ::KillTimer(m_hWnd, 1001);
    } else if (wParam == 2000) {
        // 强制刷新（解决启动时不显示问题）
        ::OutputDebugString(_T("Timer 2000: Force refresh\n"));
        ::KillTimer(m_hWnd, 2000);
        UpdateLists();
    }
    return 0;
}

LRESULT CMainFrame::OnNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;

    // 调试：输出所有通知（只输出非 LVN_GETDISPINFO 的通知）
    if (pnmh->code != LVN_GETDISPINFO) {
        TCHAR szNotify[256];
        _stprintf_s(szNotify, _T("OnNotify: hwndFrom=0x%08X, code=0x%04X\n"),
            (UINT_PTR)pnmh->hwndFrom, pnmh->code);
        ::OutputDebugString(szNotify);
    }

    // 【关键修复】LVN_GETDISPINFO 必须让 WTL 自动反射到子控件，不能在这里拦截
    // 移除了之前的 LVN_GETDISPINFO 拦截代码

    // 检查源窗口是否为我们的列表
    bool isTodoList = (pnmh->hwndFrom == m_todoList.m_hWnd);
    bool isDoneList = (pnmh->hwndFrom == m_doneList.m_hWnd);

    // 处理点击事件，更新详情面板
    if ((pnmh->code == NM_CLICK || pnmh->code == NM_RCLICK || pnmh->code == NM_DBLCLK) && (isTodoList || isDoneList)) {
        LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
        int index = pnmitem->iItem;
        // 确保 isDoneList 标志正确
        bool bIsDone = isDoneList;

        // 更新详情面板（包括点击空白处的情况）
        UpdateDetailPanel(index, bIsDone);
        
        // 如果点击的是空白处，只处理在NM_CLICK时
        if (index == -1 && pnmh->code == NM_CLICK) {
            // 空白处点击，已经隐藏详情面板，不需要额外处理
            bHandled = TRUE;
            return 0;
        }
        
        if (index != -1) {
            // 点击了有效项目时才处理右键菜单和双击            
            if (pnmh->code == NM_RCLICK) {
                // 右键菜单
                SendMessage(WM_COMMAND, ID_TODO_CONTEXT_MENU, MAKELPARAM(index, bIsDone ? 1 : 0));
                bHandled = TRUE;
                return 0;
            }
            else if (pnmh->code == NM_DBLCLK) {
                // 双击编辑
                SendMessage(WM_COMMAND, ID_TODO_EDIT, MAKELPARAM(index, bIsDone ? 1 : 0));
                bHandled = TRUE;
                return 0;
            }
        }
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CMainFrame::OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT id = LOWORD(wParam);

    // DEBUG: 输出命令 ID
    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("OnCommand: id=%d (0x%X)\n"), id, id);
    ::OutputDebugString(szDebug);

    // 分发命令到相应的处理函数
    switch (id) {
    case ID_TODO_ADD:
        return OnTodoAdd(0, 0, NULL, bHandled);

    case ID_TODO_EXPORT:
        return OnTodoExport(0, 0, NULL, bHandled);

    case ID_TODO_EXPORT_TXT:
        return OnTodoExportTxt(0, 0, NULL, bHandled);

    case ID_VIEW_EXPAND_ALL:
        return OnExpandAll(0, 0, NULL, bHandled);

    case ID_VIEW_COLLAPSE_ALL:
        return OnCollapseAll(0, 0, NULL, bHandled);

    case ID_APP_EXIT:
        return OnFileExit(0, 0, NULL, bHandled);

    case ID_APP_ABOUT:
        return OnAppAbout(0, 0, NULL, bHandled);

    case ID_LANGUAGE_CHINESE:
        return OnLanguageChinese(0, 0, NULL, bHandled);

    case ID_LANGUAGE_ENGLISH:
        return OnLanguageEnglish(0, 0, NULL, bHandled);

    case ID_CONTEXT_MARK_DONE:
        return OnContextMarkDone(0, 0, NULL, bHandled);

    case ID_CONTEXT_COPY_TEXT:
        return OnContextCopyText(0, 0, NULL, bHandled);

    case ID_CONTEXT_PIN:
        return OnContextPin(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P0:
        return OnContextPriorityP0(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P1:
        return OnContextPriorityP1(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P2:
        return OnContextPriorityP2(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P3:
        return OnContextPriorityP3(0, 0, NULL, bHandled);

    case ID_CONTEXT_EDIT:
        {
            // 使用当前选中的任务项
            if (m_nSelectedIndex >= 0) {
                DebugLog(_T("OnCommand: Handling ID_CONTEXT_EDIT with saved index=%d, isDoneList=%d\n"), m_nSelectedIndex, m_bSelectedIsDone);
                LRESULT result = OnTodoEdit(0, 0, MAKELPARAM(m_nSelectedIndex, m_bSelectedIsDone ? 1 : 0), bHandled);
                DebugLog(_T("OnCommand: OnTodoEdit returned %ld\n"), result);
                return result;
            }
            break;
        }
    case ID_TODO_DELETE:
        {
            // 使用当前选中的任务项
            if (m_nSelectedIndex >= 0) {
                DebugLog(_T("OnCommand: Handling ID_TODO_DELETE with saved index=%d, isDoneList=%d\n"), m_nSelectedIndex, m_bSelectedIsDone);
                LRESULT result = OnTodoDelete(0, 0, MAKELPARAM(m_nSelectedIndex, m_bSelectedIsDone ? 1 : 0), bHandled);
                DebugLog(_T("OnCommand: OnTodoDelete returned %ld\n"), result);
                return result;
            } else {
                DebugLog(_T("OnCommand: No selected item for ID_TODO_DELETE\n"));
            }
            break;
        }


    case ID_TODO_COMPLETE:
    case ID_TODO_EDIT:
    case ID_TODO_CONTEXT_MENU:
        {
            // 这些命令需要从 lParam 获取数据
            int index = LOWORD(lParam);
            bool isDoneList = HIWORD(lParam) != 0;

            if (id == ID_TODO_COMPLETE) {
                return OnTodoComplete(0, 0, MAKELPARAM(index, isDoneList ? 1 : 0), bHandled);
            } else if (id == ID_TODO_EDIT) {
                return OnTodoEdit(0, 0, MAKELPARAM(index, isDoneList ? 1 : 0), bHandled);
            } else if (id == ID_TODO_CONTEXT_MENU) {
                return OnTodoContextMenu(0, 0, MAKELPARAM(index, isDoneList ? 1 : 0), bHandled);
            }
        }
        break;

    case ID_WINDOW_TOPMOST:
        return OnToggleTopmost(0, 0, NULL, bHandled);

    default:
        bHandled = FALSE;
        return 0;
    }

    bHandled = TRUE;
    return 0;
}

void CMainFrame::SetupLists()
{
    // Todo 列表列
    m_todoList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_todoList.InsertColumn(0, L"创建日期", LVCFMT_LEFT, 90);
    m_todoList.InsertColumn(1, L"优先级", LVCFMT_CENTER, 55);
    m_todoList.InsertColumn(2, L"任务描述", LVCFMT_LEFT, 380);
    // 移除创建时间列
    m_todoList.InsertColumn(3, L"截止时间", LVCFMT_LEFT, 100);

    // 【架构简化】不再启用分组视图（在RefreshList中会显式禁用）
    // ListView_EnableGroupView(m_todoList, TRUE);

    // Done 列表列
    m_doneList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_doneList.InsertColumn(0, L"优先级", LVCFMT_CENTER, 55);
    m_doneList.InsertColumn(1, L"任务描述", LVCFMT_LEFT, 380);
    m_doneList.InsertColumn(2, L"完成时间", LVCFMT_LEFT, 120);

    // 【架构简化】不再启用分组视图
    // ListView_EnableGroupView(m_doneList,TRUE);
}

void CMainFrame::UpdateLists()
{
    int todoCount = m_dataManager.GetItemCount(false);
    int doneCount = m_dataManager.GetItemCount(true);

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("UpdateLists: Todo=%d, Done=%d\n"), todoCount, doneCount);
    ::OutputDebugString(szDebug);

    m_todoList.RefreshList();
    m_doneList.RefreshList();

    // 【架构简化】不再使用分组，无需展开
    // m_todoList.ExpandAllGroups();
    // m_doneList.ExpandAllGroups();

    _stprintf_s(szDebug, _T("ListRefresh: TodoItems=%d, DoneItems=%d\n"),
        m_todoList.GetItemCount(), m_doneList.GetItemCount());
    ::OutputDebugString(szDebug);
}

LRESULT CMainFrame::OnTodoAdd(WORD, WORD, HWND, BOOL&)
{
    // DEBUG: 输出消息到日志
    ::OutputDebugString(_T("OnTodoAdd called\n"));

    CAddTodoDlg dlg;
    ::OutputDebugString(_T("About to call DoModal\n"));
    INT_PTR nRet = dlg.DoModal();

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("DoModal returned: %Id\n"), nRet);
    ::OutputDebugString(szDebug);

    if (nRet == IDOK) {
        ::OutputDebugString(_T("Dialog returned IDOK\n"));
        TodoItem item = dlg.GetResult();

        TCHAR szDebug[512];
        _stprintf_s(szDebug, _T("Adding todo: title='%s', priority=%d\n"),
            item.title.c_str(), (int)item.priority);
        ::OutputDebugString(szDebug);

        m_dataManager.AddTodo(item);

        _stprintf_s(szDebug, _T("After AddTodo: todoCount=%d\n"),
            m_dataManager.GetItemCount(false));
        ::OutputDebugString(szDebug);

        // 保存到数据库
        CSQLiteManager dbManager;
        if (dbManager.Initialize()) {
            dbManager.SaveTodo(item);
            ::OutputDebugString(_T("Saved to database\n"));
        }

        UpdateLists();

        // 显示成功提示
        CString strMsg;
        CString strTitle(item.title.c_str());
        strMsg.Format(_T("任务 \"%s\" 已添加"), (LPCTSTR)strTitle);
        m_statusBar.SetText(0, (LPCTSTR)strMsg, 0);

        // 3秒后清除状态栏
        ::SetTimer(m_hWnd, 1001, 3000, nullptr);

        ::OutputDebugString(_T("Todo added and lists updated\n"));
    } else {
        ::OutputDebugString(_T("Dialog was cancelled or failed\n"));
    }

    return 0;
}

LRESULT CMainFrame::OnTodoExport(WORD, WORD, HWND, BOOL&)
{
    ExportToCSV();
    return 0;
}

LRESULT CMainFrame::OnTodoExportTxt(WORD, WORD, HWND, BOOL&)
{
    ExportToTodoTxt();
    return 0;
}

LRESULT CMainFrame::OnExpandAll(WORD, WORD, HWND, BOOL&)
{
    m_todoList.ExpandAllGroups();
    m_doneList.ExpandAllGroups();
    return 0;
}

LRESULT CMainFrame::OnCollapseAll(WORD, WORD, HWND, BOOL&)
{
    m_todoList.CollapseAllGroups();
    m_doneList.CollapseAllGroups();
    return 0;
}

LRESULT CMainFrame::OnFileExit(WORD, WORD, HWND, BOOL&)
{
    PostMessage(WM_CLOSE);
    return 0;
}

// 创建详情面板控件
void CMainFrame::CreateDetailPanelControls()
{
    // CStatic 不支持 SetBkColor，移除该调用
    // 详情面板背景色将使用系统默认色
    
    // 获取 m_fontList (Segoe UI) 的 LOGFONT
    LOGFONT lf;
    m_fontList.GetLogFont(&lf);
    
    // 统一使用普通字体（去除加粗）
    HFONT hNormalFont = ::CreateFontIndirect(&lf);

    // 空状态提示
    m_detailEmpty.Create(m_detailPanel, rcDefault, _T("点击任务查看详情"),
        WS_CHILD | WS_VISIBLE | SS_CENTER);
    m_detailEmpty.SetFont(hNormalFont);

    // 优先级
    m_detailPriority.Create(m_detailPanel, rcDefault, _T("优先级："),
        WS_CHILD | WS_VISIBLE | SS_LEFT);
    m_detailPriority.SetFont(hNormalFont);

    // 任务描述 (修改文案与列表头一致)
    m_detailDescription.Create(m_detailPanel, rcDefault, _T("任务描述："),
        WS_CHILD | WS_VISIBLE | SS_LEFT);
    m_detailDescription.SetFont(hNormalFont);

    // 创建时间
    m_detailCreateTime.Create(m_detailPanel, rcDefault, _T("创建时间："),
        WS_CHILD | WS_VISIBLE | SS_LEFT);
    m_detailCreateTime.SetFont(hNormalFont);

    // 截止时间
    m_detailEndTime.Create(m_detailPanel, rcDefault, _T("截止时间："),
        WS_CHILD | WS_VISIBLE | SS_LEFT);
    m_detailEndTime.SetFont(hNormalFont);

    // 备注
    m_detailNote.Create(m_detailPanel, rcDefault, _T("备注："),
        WS_CHILD | WS_VISIBLE | SS_LEFT);
    m_detailNote.SetFont(hNormalFont);
    
    // 分隔线1：优先级下方
    m_detailSeparator1.Create(m_detailPanel, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ);
    
    // 分隔线2：时间信息下方
    m_detailSeparator2.Create(m_detailPanel, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ);
}

// 更新详情面板
void CMainFrame::UpdateDetailPanel(int index, bool isDoneList)
{
    if (index < 0) {
        // 显示空状态
        m_detailEmpty.ShowWindow(SW_SHOW);
        m_detailPriority.ShowWindow(SW_HIDE);
        m_detailDescription.ShowWindow(SW_HIDE);
        m_detailCreateTime.ShowWindow(SW_HIDE);
        m_detailEndTime.ShowWindow(SW_HIDE);
        m_detailNote.ShowWindow(SW_HIDE);
        m_detailSeparator1.ShowWindow(SW_HIDE);
        m_detailSeparator2.ShowWindow(SW_HIDE);
        return;
    }

    // 隐藏空状态
    m_detailEmpty.ShowWindow(SW_HIDE);

    // 显示所有详情控件
    m_detailPriority.ShowWindow(SW_SHOW);
    m_detailDescription.ShowWindow(SW_SHOW);
    m_detailCreateTime.ShowWindow(SW_SHOW);
    m_detailEndTime.ShowWindow(SW_SHOW);
    m_detailNote.ShowWindow(SW_SHOW);
    m_detailSeparator1.ShowWindow(SW_SHOW);
    m_detailSeparator2.ShowWindow(SW_SHOW);

    // 获取任务项
    const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
    if (!pItem) {
        return;
    }

    // 更新控件内容
    CString strText;

    // 优先级
    strText.Format(_T("优先级：%s"), pItem->GetPriorityString());
    m_detailPriority.SetWindowText(strText);

    // 任务描述
    strText.Format(_T("任务描述：%s"), pItem->title.c_str());
    m_detailDescription.SetWindowText(strText);

    // 创建时间
    strText.Format(_T("创建时间：%s"), pItem->GetCreateTimeString());
    m_detailCreateTime.SetWindowText(strText);

    // 截止时间
    strText.Format(_T("截止时间：%s"), pItem->GetEndTimeString());
    m_detailEndTime.SetWindowText(strText);

    // 备注详情
    strText.Format(_T("备注详情：%s"), pItem->note.empty() ? _T("无备注") : pItem->note.c_str());
    m_detailNote.SetWindowText(strText);

    // 调整控件位置（新布局 - 紧凑型）
    RECT rcPanel;
    m_detailPanel.GetClientRect(&rcPanel);

    int x = 10;
    int y = 10;
    int width = rcPanel.right - rcPanel.left - 20;
    int lineHeight = 18; // 标准单行高度 (配合标准字体)
    int gapSmall = 4;    // 控件间距
    int gapLarge = 8;    // 分组间距

    // 1. 优先级
    m_detailPriority.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    // 分隔线1
    m_detailSeparator1.MoveWindow(x, y, width, 1);
    y += gapLarge;

    // 2. 任务描述 (恢复单行，消除空隙)
    m_detailDescription.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    // 3. 创建时间
    m_detailCreateTime.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    // 4. 截止时间
    m_detailEndTime.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapLarge;

    // 分隔线2
    m_detailSeparator2.MoveWindow(x, y, width, 1);
    y += gapLarge;

    // 5. 备注 (占据剩余空间)
    int noteHeight = rcPanel.bottom - rcPanel.top - y - 10;
    if (noteHeight < lineHeight) noteHeight = lineHeight;
    m_detailNote.MoveWindow(x, y, width, noteHeight);
    
    // 空状态居中显示
    m_detailEmpty.MoveWindow(0, 0, rcPanel.right - rcPanel.left, rcPanel.bottom - rcPanel.top);
}


LRESULT CMainFrame::OnAppAbout(WORD, WORD, HWND, BOOL&)
{
    ::MessageBox(m_hWnd,
        _T("Simple Todo v1.0\n\n基于 C++/WTL 的极简任务管理器\n\n"
            "特性：\n"
            "- Virtual List-View 高性能显示\n"
            "- 按日期分组，支持折叠/展开\n"
            "- 优先级颜色标识\n"
            "- SQLite 数据持久化\n"
            "- 支持 todo.txt 格式的导出\n"
            "- 支持 csv 格式导出\n\n"
            "作者：wuyueyu-五月雨\n"
            "QQ/WX：778137\n"
            "Twitter：https://x.com/wuyueyuCN\n"
            "Github：https://github.com/purezhang/simple-todo"),
        _T("关于 Simple Todo"),
        MB_OK | MB_ICONINFORMATION);
    return 0;
}


LRESULT CMainFrame::OnTodoComplete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    // 从 lParam 获取索引和是否为 Done 列表
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("OnTodoComplete: index=%d, isDoneList=%d\n"), index, isDoneList);
    ::OutputDebugString(szDebug);

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            _stprintf_s(szDebug, _T("OnTodoComplete: Found item id=%d, title='%s'\n"), pItem->id, pItem->title.c_str());
            ::OutputDebugString(szDebug);
            
            UINT id = pItem->id;
            if (m_dataManager.CompleteTodo(id)) {
                _stprintf_s(szDebug, _T("OnTodoComplete: Completed item id=%d\n"), id);
                ::OutputDebugString(szDebug);
                
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.MoveTodo(id, true);
                    _stprintf_s(szDebug, _T("OnTodoComplete: Updated database\n"), id);
                    ::OutputDebugString(szDebug);
                }
                UpdateLists();
                _stprintf_s(szDebug, _T("OnTodoComplete: Updated lists\n"), id);
                ::OutputDebugString(szDebug);
            }
        } else {
            _stprintf_s(szDebug, _T("OnTodoComplete: GetItemAt returned null for index=%d, isDoneList=%d\n"), index, isDoneList);
            ::OutputDebugString(szDebug);
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoDelete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    // 从 lParam 获取索引和是否为 Done 列表
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    DebugLog(_T("OnTodoDelete: index=%d, isDoneList=%d\n"), index, isDoneList);

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            DebugLog(_T("OnTodoDelete: Found item id=%d, title='%s'\n"), pItem->id, pItem->title.c_str());
            
            UINT id = pItem->id;
            if (m_dataManager.DeleteTodo(id, isDoneList)) {
                DebugLog(_T("OnTodoDelete: Deleted item from %s list\n"), isDoneList ? _T("done") : _T("todo"));
                
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.DeleteTodo(id);
                    DebugLog(_T("OnTodoDelete: Updated database\n"));
                }
                UpdateLists();
                DebugLog(_T("OnTodoDelete: Updated lists\n"));
            } else {
                DebugLog(_T("OnTodoDelete: Failed to delete item\n"));
            }
        } else {
            DebugLog(_T("OnTodoDelete: GetItemAt returned null for index=%d, isDoneList=%d\n"), index, isDoneList);
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoEdit(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    // 从 lParam 获取索引和是否为 Done 列表
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("OnTodoEdit: index=%d, isDoneList=%d\n"), index, isDoneList);
    ::OutputDebugString(szDebug);

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            _stprintf_s(szDebug, _T("OnTodoEdit: Found item id=%d, title='%s'\n"), pItem->id, pItem->title.c_str());
            ::OutputDebugString(szDebug);
            
            // 使用 AddTodoDlg 对话框来编辑任务，并传入原任务数据
            CAddTodoDlg dlg(*pItem);
            INT_PTR nRet = dlg.DoModal();

            _stprintf_s(szDebug, _T("OnTodoEdit: Dialog returned %Id\n"), nRet);
            ::OutputDebugString(szDebug);

            if (nRet == IDOK) {
                // 获取编辑后的任务数据
                TodoItem updatedItem = dlg.GetResult();
                // 保留原任务的ID、创建时间、完成状态和完成时间
                updatedItem.id = pItem->id;
                updatedItem.createTime = pItem->createTime;
                updatedItem.isDone = pItem->isDone;
                updatedItem.actualDoneTime = pItem->actualDoneTime;

                _stprintf_s(szDebug, _T("OnTodoEdit: Updating item id=%d, new title='%s'\n"), updatedItem.id, updatedItem.title.c_str());
                ::OutputDebugString(szDebug);

                // 更新数据
                if (m_dataManager.UpdateTodo(updatedItem, isDoneList)) {
                    _stprintf_s(szDebug, _T("OnTodoEdit: Updated item in memory\n"), updatedItem.id);
                    ::OutputDebugString(szDebug);
                    
                    // 更新数据库
                    CSQLiteManager dbManager;
                    if (dbManager.Initialize()) {
                        dbManager.UpdateTodo(updatedItem);
                        _stprintf_s(szDebug, _T("OnTodoEdit: Updated item in database\n"), updatedItem.id);
                        ::OutputDebugString(szDebug);
                    }
                    UpdateLists();
                    _stprintf_s(szDebug, _T("OnTodoEdit: Updated lists\n"), updatedItem.id);
                    ::OutputDebugString(szDebug);
                    // 更新详情面板
                    UpdateDetailPanel(index, isDoneList);
                    _stprintf_s(szDebug, _T("OnTodoEdit: Updated detail panel\n"), updatedItem.id);
                    ::OutputDebugString(szDebug);
                }
            }
        } else {
            _stprintf_s(szDebug, _T("OnTodoEdit: GetItemAt returned null for index=%d, isDoneList=%d\n"), index, isDoneList);
            ::OutputDebugString(szDebug);
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    // 从 lParam 获取索引和是否为 Done 列表
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("OnTodoContextMenu: index=%d, isDoneList=%d\n"), index, isDoneList);
    ::OutputDebugString(szDebug);

    m_nSelectedIndex = index;
    m_bSelectedIsDone = isDoneList;

    _stprintf_s(szDebug, _T("OnTodoContextMenu: Saved selected index=%d, isDoneList=%d\n"), m_nSelectedIndex, m_bSelectedIsDone);
    ::OutputDebugString(szDebug);

    if (index >= 0) {
        // 获取鼠标位置 (从 lParam 获取，或者使用当前光标位置)
        POINT pt;
        ::GetCursorPos(&pt);

        _stprintf_s(szDebug, _T("OnTodoContextMenu: Cursor position x=%d, y=%d\n"), pt.x, pt.y);
        ::OutputDebugString(szDebug);

        // 需要转换为客户端坐标
        // 注意：这里可能需要传递 HWND 来正确转换坐标
        // 暂时使用主窗口的 ScreenToClient
        ScreenToClient(&pt);
        ShowContextMenu(index, isDoneList, pt);
        
        _stprintf_s(szDebug, _T("OnTodoContextMenu: ShowContextMenu called\n"), pt.x, pt.y);
        ::OutputDebugString(szDebug);
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnContextMarkDone(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0 && !m_bSelectedIsDone) {
        const TodoItem* pItem = m_dataManager.GetItemAt(m_nSelectedIndex, false);
        if (pItem) {
            UINT id = pItem->id;
            if (m_dataManager.CompleteTodo(id)) {
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.MoveTodo(id, true);
                }
                UpdateLists();
            }
        }
    }
    return 0;
}

LRESULT CMainFrame::OnContextPin(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0 && !m_bSelectedIsDone) {
        const TodoItem* pItem = m_dataManager.GetItemAt(m_nSelectedIndex, false);
        if (pItem) {
            // 创建副本以修改
            TodoItem item = *pItem;
            item.isPinned = !item.isPinned;
            
            // 更新内存数据
            if (m_dataManager.UpdateTodo(item, false)) {
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.UpdateTodo(item);
                }
                UpdateLists();
            }
        }
    }
    return 0;
}

LRESULT CMainFrame::OnContextCopyText(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(m_nSelectedIndex, m_bSelectedIsDone);
        if (pItem) {
            CString strText;
            CString strTitle(pItem->title.c_str());
            strText.Format(_T("[%s] %s"),
                (LPCTSTR)pItem->GetPriorityString(),
                (LPCTSTR)strTitle);

            // 复制到剪贴板
            if (OpenClipboard()) {
                EmptyClipboard();
                HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, (strText.GetLength() + 1) * sizeof(TCHAR));
                if (hglb) {
                    LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
                    if (lptstr) {
                        _tcscpy_s(lptstr, strText.GetLength() + 1, strText.GetString());
                        GlobalUnlock(hglb);
                        SetClipboardData(CF_UNICODETEXT, hglb);
                    }
                }
                CloseClipboard();
            }
        }
    }
    return 0;
}

LRESULT CMainFrame::OnContextPriorityP0(WORD, WORD, HWND, BOOL&)
{
    return ChangePriority(Priority::P0);
}

LRESULT CMainFrame::OnContextPriorityP1(WORD, WORD, HWND, BOOL&)
{
    return ChangePriority(Priority::P1);
}

LRESULT CMainFrame::OnContextPriorityP2(WORD, WORD, HWND, BOOL&)
{
    return ChangePriority(Priority::P2);
}

LRESULT CMainFrame::OnContextPriorityP3(WORD, WORD, HWND, BOOL&)
{
    return ChangePriority(Priority::P3);
}

void CMainFrame::ShowContextMenu(int index, bool isDoneList, POINT pt)
{
    // 转换为屏幕坐标
    ClientToScreen(&pt);

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("ShowContextMenu: index=%d, isDoneList=%d, screen x=%d, y=%d\n"), index, isDoneList, pt.x, pt.y);
    ::OutputDebugString(szDebug);

    // 创建弹出菜单
    CMenu menu;
    menu.CreatePopupMenu();

    if (!isDoneList) {
        menu.AppendMenu(MF_STRING, ID_CONTEXT_MARK_DONE, L"标记为完成");
        menu.AppendMenu(MF_STRING, ID_CONTEXT_EDIT, L"编辑");
        
        // 检查置顶状态
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            if (pItem->isPinned) {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, L"取消置顶");
                _stprintf_s(szDebug, _T("ShowContextMenu: Added '取消置顶' menu item\n"));
                ::OutputDebugString(szDebug);
            } else {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, L"置顶");
                _stprintf_s(szDebug, _T("ShowContextMenu: Added '置顶' menu item\n"));
                ::OutputDebugString(szDebug);
            }
        }
    }
    menu.AppendMenu(MF_STRING, ID_CONTEXT_COPY_TEXT, L"复制文本");
    _stprintf_s(szDebug, _T("ShowContextMenu: Added '复制文本' menu item\n"));
    ::OutputDebugString(szDebug);

    // 优先级子菜单
    CMenu menuPriority;
    menuPriority.CreatePopupMenu();
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P0, L"P0 紧急");
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P1, L"P1 重要");
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P2, L"P2 普通");
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P3, L"P3 暂缓");

    menu.AppendMenu(MF_POPUP, (UINT_PTR)menuPriority.m_hMenu, L"优先级");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_TODO_DELETE, L"删除");
    _stprintf_s(szDebug, _T("ShowContextMenu: Added '删除' menu item\n"));
    ::OutputDebugString(szDebug);

    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        pt.x, pt.y, m_hWnd);
    _stprintf_s(szDebug, _T("ShowContextMenu: Menu displayed\n"));
    ::OutputDebugString(szDebug);
}

LRESULT CMainFrame::ChangePriority(Priority newPriority)
{
    if (m_nSelectedIndex >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(m_nSelectedIndex, m_bSelectedIsDone);
        if (pItem) {
            UINT id = pItem->id;
            if (m_dataManager.ChangePriority(id, newPriority, m_bSelectedIsDone)) {
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    TodoItem updatedItem = *pItem;
                    updatedItem.priority = newPriority;
                    dbManager.UpdateTodo(updatedItem);
                }
                UpdateLists();
            }
        }
    }
    return 0;
}

void CMainFrame::ExportToCSV()
{
    ::OutputDebugString(_T("ExportToCSV: 开始\n"));

    // 简单的 CSV 导出
    CString strFilter = _T("CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*||");
    CFileDialog dlg(FALSE, _T("csv"), _T("todos"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, m_hWnd);

    ::OutputDebugString(_T("ExportToCSV: 即将弹出文件对话框\n"));

    if (dlg.DoModal() == IDOK) {
        ::OutputDebugString(_T("ExportToCSV: 用户已确认保存\n"));

        // 使用 m_szFileName 获取完整路径
        CString strPath = dlg.m_szFileName;
        ::OutputDebugString(_T("ExportToCSV: 路径=") + strPath + _T("\n"));

        // 创建文件
        FILE* fp = nullptr;
        if (_tfopen_s(&fp, strPath, _T("w, ccs=UTF-8")) == 0 && fp) {
            ::OutputDebugString(_T("ExportToCSV: 文件已打开，开始写入\n"));

            // 写入 Todo
            _ftprintf(fp, _T("Todo 列表\n"));
            _ftprintf(fp, _T("优先级,描述,创建时间,结束时间\n"));
            for (const auto& item : m_dataManager.todoItems) {
                CString strTitle(item.title.c_str());
                _ftprintf(fp, _T("%s,%s,%s,%s\n"),
                    (LPCTSTR)item.GetPriorityString(),
                    (LPCTSTR)strTitle,
                    (LPCTSTR)item.GetCreateTimeString(),
                    (LPCTSTR)item.GetEndTimeString());
            }

            _ftprintf(fp, _T("\nDone 列表\n"));
            _ftprintf(fp, _T("优先级,描述,完成时间\n"));
            for (const auto& item : m_dataManager.doneItems) {
                CString strTitle(item.title.c_str());
                _ftprintf(fp, _T("%s,%s,%s\n"),
                    (LPCTSTR)item.GetPriorityString(),
                    (LPCTSTR)strTitle,
                    (LPCTSTR)item.GetDoneTimeString());
            }

            ::OutputDebugString(_T("ExportToCSV: 写入完成，准备关闭文件\n"));
            fclose(fp);
            MessageBox(_T("导出成功！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
            ::OutputDebugString(_T("ExportToCSV: 完成\n"));
        } else {
            ::OutputDebugString(_T("ExportToCSV: 文件打开失败\n"));
        }
    } else {
        ::OutputDebugString(_T("ExportToCSV: 用户取消\n"));
    }
}

// 将 Priority 转换为 todo.txt 优先级字母
wchar_t PriorityToTodoTxtChar(Priority p)
{
    switch (p) {
    case Priority::P0: return L'A';  // 紧急
    case Priority::P1: return L'B';  // 重要
    case Priority::P2: return L'C';  // 普通
    case Priority::P3: return L'D';  // 暂缓
    default: return L' ';
    }
}

// 辅助函数：URL 编码（简单实现，只处理必要字符）
CString UrlEncode(const CString& str)
{
    CString result;
    for (int i = 0; i < str.GetLength(); i++) {
        wchar_t ch = str.GetAt(i);
        if ((ch >= L'0' && ch <= L'9') ||
            (ch >= L'A' && ch <= L'Z') ||
            (ch >= L'a' && ch <= L'z') ||
            ch == L'-' || ch == L'_' || ch == L'.' || ch == L'~') {
            result += ch;
        } else if (ch == L' ') {
            result += L'%';
            result += L'2';
            result += L'0';
        } else {
            // 对于中文字符等，直接保留
            result += ch;
        }
    }
    return result;
}

void CMainFrame::ExportToTodoTxt()
{
    CString strFilter = _T("todo.txt 文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||");
    CFileDialog dlg(FALSE, _T("txt"), _T("todos"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, m_hWnd);

    if (dlg.DoModal() == IDOK) {
        CString strPath = dlg.m_szFileName;

        FILE* fp = nullptr;
        // 使用 UTF-8 模式打开文件
        if (_tfopen_s(&fp, strPath, _T("w, ccs=UTF-8")) == 0 && fp) {
            // 使用宽字符函数写入
            fwprintf_s(fp, L"# todo.txt Export from Simple Todo\n");

            // 获取当前时间
            time_t now = time(nullptr);
            struct tm tm_info;
            localtime_s(&tm_info, &now);
            wchar_t time_buf[64];
            wcsftime(time_buf, sizeof(time_buf)/sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_info);
            fwprintf_s(fp, L"# Generated: %s\n", time_buf);
            fwprintf_s(fp, L"# Format: (A-Z) YYYY-MM-DD Task description +project @context\n");
            fwprintf_s(fp, L"# x YYYY-MM-DD for completed tasks\n\n");

            // 写入未完成任务 (按优先级排序)
            for (const auto& item : m_dataManager.todoItems) {
                // 优先级
                wchar_t prioChar = PriorityToTodoTxtChar(item.priority);
                if (prioChar != L' ') {
                    fwprintf_s(fp, L"(%c) ", prioChar);
                }

                // 日期
                if (item.createTime.GetTime() > 0) {
                    struct tm create_tm;
                    item.createTime.GetLocalTm(&create_tm);
                    fwprintf_s(fp, L"%04d-%02d-%02d ",
                        create_tm.tm_year + 1900,
                        create_tm.tm_mon + 1,
                        create_tm.tm_mday);
                }

                // 任务标题
                fwprintf_s(fp, L"%s", item.title.c_str());

                // 截止时间 (due:)
                if (item.targetEndTime.GetTime() > 0) {
                    struct tm end_tm;
                    item.targetEndTime.GetLocalTm(&end_tm);
                    fwprintf_s(fp, L" due:%04d-%02d-%02d",
                        end_tm.tm_year + 1900,
                        end_tm.tm_mon + 1,
                        end_tm.tm_mday);
                }

                fwprintf_s(fp, L"\n");
            }

            // 分隔线
            fwprintf_s(fp, L"\n# Completed Tasks\n");

            // 写入已完成任务
            for (const auto& item : m_dataManager.doneItems) {
                fwprintf_s(fp, L"x ");

                // 完成日期
                if (item.actualDoneTime.GetTime() > 0) {
                    struct tm done_tm;
                    item.actualDoneTime.GetLocalTm(&done_tm);
                    fwprintf_s(fp, L"%04d-%02d-%02d ",
                        done_tm.tm_year + 1900,
                        done_tm.tm_mon + 1,
                        done_tm.tm_mday);
                }

                // 优先级
                wchar_t prioChar = PriorityToTodoTxtChar(item.priority);
                if (prioChar != L' ') {
                    fwprintf_s(fp, L"(%c) ", (wchar_t)prioChar);
                }

                // 任务标题
                fwprintf_s(fp, L"%s", item.title.c_str());

                // 截止时间
                if (item.targetEndTime.GetTime() > 0) {
                    struct tm end_tm;
                    item.targetEndTime.GetLocalTm(&end_tm);
                    fwprintf_s(fp, L" due:%04d-%02d-%02d",
                        end_tm.tm_year + 1900,
                        end_tm.tm_mon + 1,
                        end_tm.tm_mday);
                }

                fwprintf_s(fp, L"\n");
            }

            fclose(fp);
            MessageBox(_T("todo.txt 导出成功！\n\n格式说明:\n(A) 紧急任务\n(B) 重要任务\n(C) 普通任务\n(D) 暂缓任务\nx 已完成任务\ndue: 截止时间"),
                _T("导出成功"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

LRESULT CMainFrame::OnLanguageChinese(WORD, WORD, HWND, BOOL&)
{
    ::OutputDebugString(_T("OnLanguageChinese called\n"));

    m_bChineseLanguage = true;

    // 重新加载菜单为中文
    HMENU hOldMenu = GetMenu();
    HMENU hNewMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
    if (hNewMenu) {
        ::OutputDebugString(_T("Chinese menu loaded successfully\n"));
        SetMenu(hNewMenu);
        if (hOldMenu) DestroyMenu(hOldMenu);
        DrawMenuBar();
    } else {
        ::OutputDebugString(_T("Failed to load Chinese menu\n"));
    }

    return 0;
}

LRESULT CMainFrame::OnLanguageEnglish(WORD, WORD, HWND, BOOL&)
{
    ::OutputDebugString(_T("OnLanguageEnglish called\n"));

    m_bChineseLanguage = false;

    // 重新加载菜单为英文
    HMENU hOldMenu = GetMenu();
    HMENU hNewMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME_EN));
    if (hNewMenu) {
        ::OutputDebugString(_T("English menu loaded successfully\n"));
        SetMenu(hNewMenu);
        if (hOldMenu) DestroyMenu(hOldMenu);
        DrawMenuBar();
    } else {
        ::OutputDebugString(_T("Failed to load English menu\n"));
    }

    return 0;
}

LRESULT CMainFrame::OnToggleTopmost(WORD, WORD, HWND, BOOL&)
{
    m_bTopmost = !m_bTopmost; // 切换置顶状态

    if (m_bTopmost) {
        // 设置窗口为置顶
        SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 更新菜单项文本
        HMENU hMenu = GetMenu();
        if (hMenu) {
            ModifyMenu(hMenu, ID_WINDOW_TOPMOST, MF_BYCOMMAND | MF_STRING, ID_WINDOW_TOPMOST, _T("取消置顶(&T)"));
            DrawMenuBar();
        }
    } else {
        // 取消置顶
        SetWindowPos(HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        // 更新菜单项文本
        HMENU hMenu = GetMenu();
        if (hMenu) {
            ModifyMenu(hMenu, ID_WINDOW_TOPMOST, MF_BYCOMMAND | MF_STRING, ID_WINDOW_TOPMOST, _T("窗口置顶(&T)"));
            DrawMenuBar();
        }
    }

    return 0;
}
