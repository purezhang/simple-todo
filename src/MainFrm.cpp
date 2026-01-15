#include "stdafx.h"
#include <commctrl.h>
#include "MainFrm.h"
#include "AddTodoDlg.h"
#include "SQLiteManager.h"

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
    // Startup banner - always visible
    TODO_LOG(_T("===============================================\n"));
    TODO_LOG(_T("SimpleTodo Application Starting\n"));
    TODO_LOG(_T("Build: " __DATE__ " " __TIME__ "\n"));
    TODO_LOG(_T("===============================================\n"));

    // 加载菜单
    HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
    if (hMenu) {
        SetMenu(hMenu);
        TODO_DEBUG_LOG(_T("Menu loaded successfully\n"));
    } else {
        TODO_LOG(_T("ERROR: Failed to load menu\n"));
    }

    // 创建标题栏
    RECT rcTitle = {0, 0, 200, 35};
    m_titleStatic.Create(m_hWnd, rcTitle, _T("Simple Todo - 待办事项管理"),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE);

    // 创建展开/折叠按钮
    RECT rcBtn = {0, 0, 70, 28};
    m_btnExpand.Create(m_hWnd, rcBtn, _T("展开全部"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_BORDER, 0, ID_VIEW_EXPAND_ALL);
    m_btnCollapse.Create(m_hWnd, rcBtn, _T("收起全部"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_BORDER, 0, ID_VIEW_COLLAPSE_ALL);

    // 创建分割器
    m_hWndClient = m_splitter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // 创建 Todo 列表
    m_todoList.Create(m_splitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,  // 移除 LVS_OWNERDATA，使用普通列表
        WS_EX_CLIENTEDGE);
    m_todoList.SetDataManager(&m_dataManager);
    m_todoList.SetIsDoneList(false);

    // 创建 Done 列表
    m_doneList.Create(m_splitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,  // 移除 LVS_OWNERDATA，使用普通列表
        WS_EX_CLIENTEDGE);
    m_doneList.SetDataManager(&m_dataManager);
    m_doneList.SetIsDoneList(true);

    // 设置分割器（上下分割）
    m_splitter.SetSplitterPanes(m_todoList, m_doneList);
    
    // 【修复】启用比例分割，并自动居中
    // SPLIT_PROPORTIONAL: 调整大小时保持比例
    // SetSplitterPos(-1): 设置为中间位置
    m_splitter.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
    m_splitter.SetSplitterPos(-1);

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

    // 设置定时器延迟刷新，确保窗口完全准备好后再次刷新
    SetTimer(2000, 200, nullptr);     // 200ms 后再次强制刷新

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

LRESULT CMainFrame::OnSize(UINT, WPARAM, LPARAM, BOOL&)
{
    // 调整标题栏和按钮位置
    RECT rcClient;
    GetClientRect(&rcClient);

    // 标题栏
    RECT rcTitle = {15, 8, 250, 38};
    m_titleStatic.MoveWindow(&rcTitle);

    // 按钮尺寸
    int nBtnWidth = 70;
    int nBtnHeight = 28;

    // 按钮位置
    RECT rcExpand = {rcClient.right - nBtnWidth * 2 - 25, 5,
        rcClient.right - nBtnWidth - 20, 5 + nBtnHeight};
    RECT rcCollapse = {rcClient.right - nBtnWidth - 15, 5,
        rcClient.right - 10, 5 + nBtnHeight};

    m_btnExpand.MoveWindow(&rcExpand);
    m_btnCollapse.MoveWindow(&rcCollapse);

    // 调整分割器位置，为标题栏留出空间
    if (m_splitter.IsWindow()) {
        RECT rcSplitter = {0, 40, rcClient.right, rcClient.bottom};
        m_splitter.MoveWindow(&rcSplitter);
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

    // 处理右键点击 (NM_RCLICK) 和 双击 (NM_DBLCLK)
    if (pnmh->code == NM_RCLICK || pnmh->code == NM_DBLCLK) {
        // 检查源窗口是否为我们的列表
        bool isTodoList = (pnmh->hwndFrom == m_todoList.m_hWnd);
        bool isDoneList = (pnmh->hwndFrom == m_doneList.m_hWnd);

        if (isTodoList || isDoneList) {
            LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
            int index = pnmitem->iItem;
            // 确保 isDoneList 标志正确
            bool bIsDone = isDoneList;

            if (pnmh->code == NM_RCLICK) {
                // 右键菜单: 无须选中特定项也可以弹出(如通用菜单)，但这里主要是针对项
                // 发送 ID_TODO_CONTEXT_MENU 命令
                // LPARAM: LOWORD=index, HIWORD=isDone
                SendMessage(WM_COMMAND, ID_TODO_CONTEXT_MENU, MAKELPARAM(index, bIsDone ? 1 : 0));
                bHandled = TRUE;
                return 0;
            }
            else if (pnmh->code == NM_DBLCLK && index != -1) {
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

    case ID_CONTEXT_PRIORITY_P0:
        return OnContextPriorityP0(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P1:
        return OnContextPriorityP1(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P2:
        return OnContextPriorityP2(0, 0, NULL, bHandled);

    case ID_CONTEXT_PRIORITY_P3:
        return OnContextPriorityP3(0, 0, NULL, bHandled);

    case ID_TODO_COMPLETE:
    case ID_TODO_DELETE:
    case ID_TODO_EDIT:
    case ID_TODO_CONTEXT_MENU:
        {
            // 这些命令需要从 lParam 获取数据
            int index = LOWORD(lParam);
            bool isDoneList = HIWORD(lParam) != 0;

            if (id == ID_TODO_COMPLETE) {
                return OnTodoComplete(0, MAKELPARAM(index, isDoneList ? 1 : 0), 0, bHandled);
            } else if (id == ID_TODO_DELETE) {
                return OnTodoDelete(0, MAKELPARAM(index, isDoneList ? 1 : 0), 0, bHandled);
            } else if (id == ID_TODO_EDIT) {
                return OnTodoEdit(0, MAKELPARAM(index, isDoneList ? 1 : 0), 0, bHandled);
            } else if (id == ID_TODO_CONTEXT_MENU) {
                return OnTodoContextMenu(0, MAKELPARAM(index, isDoneList ? 1 : 0), 0, bHandled);
            }
        }
        break;

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

    m_todoList.InsertColumn(0, L"日期", LVCFMT_LEFT, 90);
    m_todoList.InsertColumn(1, L"优先级", LVCFMT_CENTER, 55);
    m_todoList.InsertColumn(2, L"任务描述", LVCFMT_LEFT, 380);
    m_todoList.InsertColumn(3, L"创建时间", LVCFMT_LEFT, 100);
    m_todoList.InsertColumn(4, L"截止时间", LVCFMT_LEFT, 100);

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

LRESULT CMainFrame::OnAppAbout(WORD, WORD, HWND, BOOL&)
{
    ::MessageBox(m_hWnd,
        _T("Simple Todo v1.0\n\n基于 C++/WTL 的极简任务管理器\n\n"
            "特性：\n"
            "- Virtual List-View 高性能显示\n"
            "- 按日期分组，支持折叠/展开\n"
            "- 优先级颜色标识\n"
            "- SQLite 数据持久化"),
        _T("关于 Simple Todo"),
        MB_OK | MB_ICONINFORMATION);
    return 0;
}

LRESULT CMainFrame::OnTodoComplete(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    // 从 wParam 获取索引和是否为 Done 列表
    int index = LOWORD(wParam);
    bool isDoneList = HIWORD(wParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            UINT id = pItem->id;
            if (m_dataManager.CompleteTodo(id)) {
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.MoveTodo(id, true);
                }
                UpdateLists();
            }
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoDelete(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    // 从 wParam 获取索引和是否为 Done 列表
    int index = LOWORD(wParam);
    bool isDoneList = HIWORD(wParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            UINT id = pItem->id;
            if (m_dataManager.DeleteTodo(id, isDoneList)) {
                // 更新数据库
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.DeleteTodo(id);
                }
                UpdateLists();
            }
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoEdit(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    // 从 wParam 获取索引和是否为 Done 列表
    int index = LOWORD(wParam);
    bool isDoneList = HIWORD(wParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            // 这里可以实现编辑功能
            // 暂时显示任务信息
            CString strMsg;
            CString strTitle(pItem->title.c_str());
            strMsg.Format(_T("编辑任务:\n标题: %s\n优先级: %s"),
                (LPCTSTR)strTitle,
                (LPCTSTR)pItem->GetPriorityString());
            MessageBox(strMsg, _T("编辑任务"), MB_OK | MB_ICONINFORMATION);
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // 从 wParam 获取索引和是否为 Done 列表
    int index = LOWORD(wParam);
    bool isDoneList = HIWORD(wParam) != 0;

    m_nSelectedIndex = index;
    m_bSelectedIsDone = isDoneList;

    if (index >= 0) {
        // 获取鼠标位置 (从 lParam 获取，或者使用当前光标位置)
        POINT pt;
        ::GetCursorPos(&pt);

        // 需要转换为客户端坐标
        // 注意：这里可能需要传递 HWND 来正确转换坐标
        // 暂时使用主窗口的 ScreenToClient
        ScreenToClient(&pt);
        ShowContextMenu(index, isDoneList, pt);
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

    // 创建弹出菜单
    CMenu menu;
    menu.CreatePopupMenu();

    if (!isDoneList) {
        menu.AppendMenu(MF_STRING, ID_CONTEXT_MARK_DONE, L"标记为完成");
    }
    menu.AppendMenu(MF_STRING, ID_CONTEXT_COPY_TEXT, L"复制文本");

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

    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        pt.x, pt.y, m_hWnd);
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
