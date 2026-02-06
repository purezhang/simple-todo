#include "stdafx.h"
#include <commctrl.h>
#include "MainFrm.h"
#include "AddTodoDlg.h"
#include "SQLiteManager.h"

// ReBar 子类化：转发 WM_COMMAND 消息给父窗口
static WNDPROC g_originalReBarWndProc = nullptr;
static HWND g_hToolbar = nullptr; // 保存 ToolBar 句柄用于识别

static LRESULT CALLBACK ReBarSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_COMMAND) {
        UINT id = LOWORD(wParam);
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("[ReBarSubclass] WM_COMMAND: id=0x%04X (%u), wParam=0x%08X, lParam=0x%08X\n"),
            id, id, (UINT_PTR)wParam, (UINT_PTR)lParam);
        ::OutputDebugString(szDebug);

        // 检查是否来自 ToolBar（lParam 是控件句柄）
        if (lParam == (LPARAM)g_hToolbar) {
            ::OutputDebugString(_T("[ReBarSubclass] 来自 ToolBar，转发后返回 0\n"));
            // 转发给父窗口（MainFrame）
            HWND hParent = ::GetParent(hWnd);
            if (hParent) {
                ::SendMessage(hParent, WM_COMMAND, wParam, lParam);
            }
            // 返回 0 阻止重复处理
            return 0;
        }

        // 其他控件的消息让原始过程处理
        ::OutputDebugString(_T("[ReBarSubclass] 非来自 ToolBar，继续原始处理\n"));
    }
    return CallWindowProc(g_originalReBarWndProc, hWnd, uMsg, wParam, lParam);
}

// ComboBox 子类化：转发 WM_COMMAND 消息给父窗口  
static WNDPROC g_originalComboWndProc = nullptr;
static LRESULT CALLBACK ComboSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_COMMAND) {
        ::OutputDebugString(_T("[ComboSubclass] WM_COMMAND forwarded\n"));
        HWND hParent = ::GetParent(hWnd);
        if (hParent) {
            ::SendMessage(hParent, WM_COMMAND, wParam, lParam);
        }
    }
    return CallWindowProc(g_originalComboWndProc, hWnd, uMsg, wParam, lParam);
}

// 工具栏按钮文字
#define TOPMOST_TEXT_NORMAL   _T("📌置顶")
#define TOPMOST_TEXT_CHECKED  _T("📌取消")
#define TIME_FILTER_TODAY    _T("🏷今天")
#define TIME_FILTER_WEEK     _T("🏷本周")
#define TIME_FILTER_ALL     _T("🏷全部")

void DebugLog(const TCHAR* format, ...) {
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), format, args);
    va_end(args);
    OutputDebugString(buffer);
    
#ifdef _DEBUG
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsole(hConsole, buffer, (DWORD)_tcslen(buffer), &written, NULL);
    }
#endif
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
        PostMessage(WM_COMMAND, ID_TODO_ADD);
        return TRUE;
    }

    if (pMsg->message == WM_LBUTTONDOWN && m_bDetailVisible) {
        CPoint pt(GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam));
        RECT rcDetail;
        m_detailPanel.GetWindowRect(&rcDetail);
        ::ScreenToClient(m_hWnd, (LPPOINT)&rcDetail);
        ::ScreenToClient(m_hWnd, (LPPOINT)&rcDetail + 1);
        if (!PtInRect(&rcDetail, pt)) {
            HideDetailPopup();
            return TRUE;
        }
    }

    return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
{
    ::OutputDebugString(_T("[OnCreate] 入口\n"));
#ifdef _DEBUG
    DebugLog(_T("SimpleTodo Application Starting\n"));
#endif

    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    m_fontList.CreateFontIndirect(&ncm.lfMessageFont);

    m_imgList.Create(1, 20, ILC_COLOR32, 0, 0);

    m_rebar.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS);

    ::OutputDebugString(_T("[OnCreate] ReBar 创建完成，准备子类化\n"));

    // 子类化 ReBar 转发 WM_COMMAND 消息
    g_originalReBarWndProc = (WNDPROC)::SetWindowLongPtr(m_rebar.m_hWnd, GWLP_WNDPROC, (LONG_PTR)ReBarSubclassProc);
    ::OutputDebugString(_T("[OnCreate] ReBar 子类化完成\n"));

    m_toolbar.Create(m_rebar, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
        TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN,
        0, ATL_IDW_TOOLBAR);

    m_toolbar.SetButtonStructSize();

    // 工具栏按钮: 置顶 | 时间筛选 | 添加任务
    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_WINDOW_TOPMOST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"📌置顶" },
        { I_IMAGENONE, ID_TIME_FILTER, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"🏷全部" },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_TODO_ADD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"🆕新增" }
    };

    m_toolbar.AddButtons(4, buttons);

    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_SIZE;
    
    tbbi.cx = 45;
    m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbbi);
    
    tbbi.cx = 45;
    m_toolbar.SetButtonInfo(ID_TIME_FILTER, &tbbi);
    
    tbbi.cx = 45;
    m_toolbar.SetButtonInfo(ID_TODO_ADD, &tbbi);

    // 保存 ToolBar 句柄用于消息识别
    g_hToolbar = m_toolbar.m_hWnd;

    REBARBANDINFO rbbiToolbar = { sizeof(REBARBANDINFO) };
    rbbiToolbar.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiToolbar.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiToolbar.hwndChild = m_toolbar;
    rbbiToolbar.cxMinChild = 0;
    rbbiToolbar.cyMinChild = 24;
    rbbiToolbar.cx = 350;
    m_rebar.InsertBand(-1, &rbbiToolbar);

    REBARBANDINFO rbbi = { sizeof(REBARBANDINFO) };
    m_searchLabel.Create(m_rebar, rcDefault, L"🔍 ",
        WS_CHILD | WS_VISIBLE,
        0, ATL_IDW_TOOLBAR + 10);

    m_searchEdit.Create(m_rebar, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT,
        0, ID_SEARCH_EDIT);

    // 设置搜索框字体和样式
    m_searchEdit.SetFont(m_fontList);

    REBARBANDINFO rbbiSearch = { sizeof(REBARBANDINFO) };
    rbbiSearch.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiSearch.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiSearch.hwndChild = m_searchLabel;
    rbbiSearch.cxMinChild = 0;
    rbbiSearch.cyMinChild = 20;
    rbbiSearch.cx = 30;
    m_rebar.InsertBand(-1, &rbbiSearch);

    REBARBANDINFO rbbiEdit = { sizeof(REBARBANDINFO) };
    rbbiEdit.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiEdit.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiEdit.hwndChild = m_searchEdit;
    rbbiEdit.cxMinChild = 200;
    rbbiEdit.cyMinChild = 21;
    rbbiEdit.cx = 200;
    m_rebar.InsertBand(-1, &rbbiEdit);

    // 添加项目筛选下拉框
    m_projectFilter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL,
        0, ID_PROJECT_FILTER);
    m_projectFilter.SetFont(m_fontList);
    m_projectFilter.AddString(L"[全部]");
    m_projectFilter.SetCurSel(0);

    // 子类化 ComboBox 转发 WM_COMMAND 消息
    g_originalComboWndProc = (WNDPROC)::SetWindowLongPtr(m_projectFilter.m_hWnd, GWLP_WNDPROC, (LONG_PTR)ComboSubclassProc);
    ::OutputDebugString(_T("[OnCreate] ComboBox 子类化完成\n"));

    REBARBANDINFO rbbiProject = { sizeof(REBARBANDINFO) };
    rbbiProject.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiProject.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiProject.hwndChild = m_projectFilter;
    rbbiProject.cxMinChild = 100;
    rbbiProject.cyMinChild = 21;
    rbbiProject.cx = 100;
    m_rebar.InsertBand(-1, &rbbiProject);

    // 让项目筛选靠右（通过设置较大的cx使其占据剩余空间）
    REBARBANDINFO rbbiProjectFill = { sizeof(REBARBANDINFO) };
    rbbiProjectFill.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE;
    rbbiProjectFill.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiProjectFill.hwndChild = m_projectFilter;
    rbbiProjectFill.cx = -1;  // 占据剩余空间
    m_rebar.InsertBand(-1, &rbbiProjectFill);

    m_mainSplitter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_todoList.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,
        WS_EX_CLIENTEDGE);
    m_todoList.SetFont(m_fontList);
    m_todoList.SetImageList(m_imgList, LVSIL_SMALL);
    m_todoList.SetDataManager(&m_dataManager);
    m_todoList.SetIsDoneList(false);

    m_doneList.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS,
        WS_EX_CLIENTEDGE);
    m_doneList.SetFont(m_fontList);
    m_doneList.SetImageList(m_imgList, LVSIL_SMALL);
    m_doneList.SetDataManager(&m_dataManager);
    m_doneList.SetIsDoneList(true);

    m_detailPanel.Create(m_hWnd, rcDefault, NULL,
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
        WS_EX_CLIENTEDGE);

    m_mainSplitter.SetSplitterPanes(m_todoList, m_doneList);

    m_mainSplitter.SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);

    m_mainSplitter.SetSplitterPos(-1);

    CreateDetailPanelControls();
    UpdateDetailPanel(-1, false);

    m_statusBar.Create(m_hWnd, rcDefault, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0);
    m_statusBar.SetSimple(FALSE);
    int parts[] = { 400, -1 };
    m_statusBar.SetParts(2, parts);
    m_statusBar.SetText(0, _T("就绪"), 0);

    SetupLists();

    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        BOOL bLoaded = dbManager.LoadAll(m_dataManager);
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("LoadAll result=%d, todoCount=%zu, doneCount=%zu\n"),
            bLoaded, m_dataManager.todoItems.size(), m_dataManager.doneItems.size());
        ::OutputDebugString(szDebug);

        if (m_dataManager.todoItems.empty() && m_dataManager.doneItems.empty()) {
            ::OutputDebugString(_T("生成默认测试数据...\n"));

            // 生成12个测试数据
            // 3个项目分类 × 4个优先级 = 12个任务
            struct DemoTask {
                Priority priority;
                const wchar_t* project;
                const wchar_t* title;
                const wchar_t* note;
                int daysUntilDue;  // 截止时间偏移天数
            };

            CTime now = CTime::GetCurrentTime();

            // 3个项目 × 4个优先级 = 12个任务
            DemoTask demos[] = {
                // 项目A
                { Priority::P0, L"项目A", L"demo: 紧急功能上线", L"这是项目A的P0紧急任务描述，需要立即处理", 0 },
                { Priority::P1, L"项目A", L"demo: 核心功能开发", L"这是项目A的P1重要任务描述", 1 },
                { Priority::P2, L"项目A", L"demo: 功能模块优化", L"这是项目A的P2普通任务描述", 2 },
                { Priority::P3, L"项目A", L"demo: 文档整理", L"这是项目A的P3暂缓任务描述", 5 },

                // 项目B
                { Priority::P0, L"项目B", L"demo: 系统故障修复", L"这是项目B的P0紧急任务描述，系统故障需要立即处理", 0 },
                { Priority::P1, L"项目B", L"demo: 性能调优", L"这是项目B的P1重要任务描述", 1 },
                { Priority::P2, L"项目B", L"demo: 代码重构", L"这是项目B的P2普通任务描述", 3 },
                { Priority::P3, L"项目B", L"demo: 注释补充", L"这是项目B的P3暂缓任务描述", 7 },

                // 项目C
                { Priority::P0, L"项目C", L"demo: 安全漏洞修补", L"这是项目C的P0紧急任务描述，安全问题刻不容缓", 0 },
                { Priority::P1, L"项目C", L"demo: 新需求实现", L"这是项目C的P1重要任务描述", 2 },
                { Priority::P2, L"项目C", L"demo: 单元测试补充", L"这是项目C的P2普通任务描述", 4 },
                { Priority::P3, L"项目C", L"demo: 废弃代码清理", L"这是项目C的P3暂缓任务描述", 10 },
            };

            // 添加12个待办任务
            for (int i = 0; i < 12; ++i) {
                TodoItem item;
                item.id = m_dataManager.nextId++;
                item.priority = demos[i].priority;
                item.title = demos[i].title;
                item.note = demos[i].note;
                item.project = demos[i].project;
                item.createTime = now;
                item.targetEndTime = now + CTimeSpan(demos[i].daysUntilDue, 0, 0, 0);
                item.isDone = false;
                m_dataManager.todoItems.push_back(item);
            }

            // 添加3个已完成任务演示已完成列表
            TodoItem doneItem1;
            doneItem1.id = m_dataManager.nextId++;
            doneItem1.priority = Priority::P1;
            doneItem1.title = L"demo: 已完成的需求分析";
            doneItem1.note = L"这是已完成的任务描述，演示已完成列表功能";
            doneItem1.project = L"项目A";
            doneItem1.createTime = now - CTimeSpan(5, 0, 0, 0);
            doneItem1.actualDoneTime = now - CTimeSpan(3, 0, 0, 0);
            doneItem1.targetEndTime = now - CTimeSpan(3, 0, 0, 0);
            doneItem1.isDone = true;
            m_dataManager.doneItems.push_back(doneItem1);

            TodoItem doneItem2;
            doneItem2.id = m_dataManager.nextId++;
            doneItem2.priority = Priority::P2;
            doneItem2.title = L"demo: 已完成的代码编写";
            doneItem2.note = L"这也是已完成的任务描述";
            doneItem2.project = L"项目B";
            doneItem2.createTime = now - CTimeSpan(7, 0, 0, 0);
            doneItem2.actualDoneTime = now - CTimeSpan(5, 0, 0, 0);
            doneItem2.targetEndTime = now - CTimeSpan(5, 0, 0, 0);
            doneItem2.isDone = true;
            m_dataManager.doneItems.push_back(doneItem2);

            TodoItem doneItem3;
            doneItem3.id = m_dataManager.nextId++;
            doneItem3.priority = Priority::P0;
            doneItem3.title = L"demo: 已完成的紧急修复";
            doneItem3.note = L"P0紧急任务已完成";
            doneItem3.project = L"项目C";
            doneItem3.createTime = now - CTimeSpan(2, 0, 0, 0);
            doneItem3.actualDoneTime = now - CTimeSpan(2, 0, 0, 0);
            doneItem3.targetEndTime = now - CTimeSpan(2, 0, 0, 0);
            doneItem3.isDone = true;
            m_dataManager.doneItems.push_back(doneItem3);

            ::OutputDebugString(_T("已生成12个待办 + 3个已完成 测试数据\n"));

            dbManager.SaveAll(m_dataManager);
        }
    }

    UpdateLists();
    UpdateProjectFilterList();
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE);

    // 加载保存的窗口设置
    LoadWindowSettings();

    SetTimer(2000, 200, nullptr);
    PostMessage(WM_SIZE);

    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    // 保存窗口设置
    SaveWindowSettings();

    ::KillTimer(m_hWnd, 2000);
    ::KillTimer(m_hWnd, 1001);

    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        dbManager.SaveAll(m_dataManager);
    }

    ::PostQuitMessage(0);
    return 0;
}

LRESULT CMainFrame::OnAppRefresh(UINT, WPARAM, LPARAM, BOOL&)
{
    ::OutputDebugString(_T("OnAppRefresh: Refreshing lists...\n"));

    TCHAR szDebug[512];
    if (m_todoList.IsWindow()) {
        DWORD style = m_todoList.GetStyle();
        DWORD exStyle = m_todoList.GetExStyle();
        _stprintf_s(szDebug, _T("  m_todoList: HWND=0x%08X, style=0x%08X, exStyle=0x%08X\n"),
            (UINT_PTR)m_todoList.m_hWnd, style, exStyle);
        ::OutputDebugString(szDebug);

        BOOL hasOwnerData = (style & LVS_OWNERDATA) != 0;
        _stprintf_s(szDebug, _T("  LVS_OWNERDATA=%d\n"), hasOwnerData);
        ::OutputDebugString(szDebug);

        int groupCount = ListView_GetGroupCount(m_todoList);
        _stprintf_s(szDebug, _T("  groupCount=%d\n"), groupCount);
        ::OutputDebugString(szDebug);

        int itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  GetItemCount=%d\n"), itemCount);
        ::OutputDebugString(szDebug);

        m_todoList.SetItemCountEx(m_dataManager.GetItemCount(false), LVSICF_NOSCROLL);
        itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  After SetItemCountEx: GetItemCount=%d\n"), itemCount);
        ::OutputDebugString(szDebug);
    }

    UpdateLists();
    return 0;
}

LRESULT CMainFrame::OnSize(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    bHandled = TRUE;

    RECT rcClient;
    GetClientRect(&rcClient);

    int toolbarHeight = 0;
    if (m_rebar.IsWindow()) {
        m_rebar.MoveWindow(0, 0, rcClient.right, 30);

        RECT rcRebar;
        m_rebar.GetClientRect(&rcRebar);
        int toolbarHeight = rcRebar.bottom - rcRebar.top;
    }

    if (m_mainSplitter.IsWindow()) {
        int topOffset = toolbarHeight;
        int clientHeight = rcClient.bottom - topOffset;
        int clientWidth = rcClient.right - rcClient.left;

        m_mainSplitter.MoveWindow(0, topOffset, clientWidth, clientHeight);

        if (m_bFirstSize && clientHeight > 100 && clientWidth > 100) {
            m_mainSplitter.SetSplitterPos((int)(clientHeight * 0.6));
            m_bFirstSize = false;
        }
    }

    if (m_statusBar.IsWindow()) {
        m_statusBar.MoveWindow(&rcClient);
    }

    // Update popup position when main window is resized
    if (m_bDetailVisible) {
        ShowDetailPopup();
    }

    return 0;
}

LRESULT CMainFrame::OnMove(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    bHandled = TRUE;

    // Update popup position when main window is moved
    if (m_bDetailVisible) {
        ShowDetailPopup();
    }

    return 0;
    }

LRESULT CMainFrame::OnNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;

    bool isTodoList = (pnmh->hwndFrom == m_todoList.m_hWnd);
    bool isDoneList = (pnmh->hwndFrom == m_doneList.m_hWnd);

    if ((pnmh->code == NM_CLICK || pnmh->code == NM_RCLICK || pnmh->code == NM_DBLCLK) && (isTodoList || isDoneList)) {
        LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
        int index = pnmitem->iItem;
        bool bIsDone = isDoneList;

        if (index >= 0) {
            ShowDetailPopup();
            UpdateDetailPanel(index, bIsDone);
            
            if (pnmh->code == NM_RCLICK) {
                SendMessage(WM_COMMAND, ID_TODO_CONTEXT_MENU, MAKELPARAM(index, bIsDone ? 1 : 0));
                bHandled = TRUE;
                return 0;
            }
            else if (pnmh->code == NM_DBLCLK) {
                SendMessage(WM_COMMAND, ID_TODO_EDIT, MAKELPARAM(index, bIsDone ? 1 : 0));
                bHandled = TRUE;
                return 0;
            }
        } else {
            HideDetailPopup();
        }
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CMainFrame::OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT id = LOWORD(wParam);
    UINT code = HIWORD(wParam);

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("[OnCommand] id=0x%04X (%u), code=0x%04X, lParam=0x%08X\n"),
        id, id, code, (UINT_PTR)lParam);
    ::OutputDebugString(szDebug);

    switch (id) {
    case ID_TODO_ADD:
        return OnTodoAdd(0, 0, NULL, bHandled);
    case ID_TODO_EXPORT:
        return OnTodoExport(0, 0, NULL, bHandled);
    case ID_TODO_EXPORT_TXT:
        return OnTodoExportTxt(0, 0, NULL, bHandled);
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
        if (m_nSelectedIndex >= 0) {
            return OnTodoEdit(0, 0, MAKELPARAM(m_nSelectedIndex, m_bSelectedIsDone ? 1 : 0), bHandled);
        }
        break;
    case ID_TODO_DELETE:
        if (m_nSelectedIndex >= 0) {
            return OnTodoDelete(0, 0, MAKELPARAM(m_nSelectedIndex, m_bSelectedIsDone ? 1 : 0), bHandled);
        }
        break;
    case ID_TODO_COMPLETE:
    case ID_TODO_EDIT:
    case ID_TODO_CONTEXT_MENU:
        {
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
        ::OutputDebugString(_T("[OnCommand] 路由到 OnToggleTopmost\n"));
        return OnToggleTopmost(0, 0, NULL, bHandled);
    case ID_TIME_FILTER:
        ::OutputDebugString(_T("[OnCommand] 路由到 OnToggleTimeFilter\n"));
        return OnToggleTimeFilter(0, 0, NULL, bHandled);
    case ID_PROJECT_FILTER:
        return OnProjectFilterChanged(0, 0, NULL, bHandled);
    default:
        // 检查是否是搜索框的 EN_CHANGE 通知
        if (id == ID_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
            OnSearchChanged();
            return 0;
        }
        // 检查是否是项目筛选下拉框的 CBN_SELCHANGE 通知（从列表选择）
        if (id == ID_PROJECT_FILTER && HIWORD(wParam) == CBN_SELCHANGE) {
            OnProjectFilterChanged(0, 0, NULL, bHandled);
            return 0;
        }
        // 检查是否是项目筛选下拉框的 CBN_EDITCHANGE 通知（手动输入）
        if (id == ID_PROJECT_FILTER && HIWORD(wParam) == CBN_EDITCHANGE) {
            OnProjectFilterChanged(0, 0, NULL, bHandled);
            return 0;
        }
        bHandled = FALSE;
        return 0;
    }

    bHandled = TRUE;
    return 0;
}

void CMainFrame::SetupLists()
{
    m_todoList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_todoList.InsertColumn(0, L"创建日期", LVCFMT_LEFT, 75);
    m_todoList.InsertColumn(1, L"优先级", LVCFMT_CENTER, 45);
    m_todoList.InsertColumn(2, L"任务描述", LVCFMT_LEFT, 380);
    m_todoList.InsertColumn(3, L"截止时间", LVCFMT_LEFT, 110);

    m_doneList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_doneList.InsertColumn(0, L"优先级", LVCFMT_CENTER, 45);
    m_doneList.InsertColumn(1, L"任务描述", LVCFMT_LEFT, 380);
    m_doneList.InsertColumn(2, L"完成时间", LVCFMT_LEFT, 110);
}

void CMainFrame::UpdateLists()
{
    // 关键日志：输出 m_timeFilter 的实际值
    TCHAR szTimeFilterName[16];
    switch (m_timeFilter) {
    case TimeFilter::Today: _tcscpy_s(szTimeFilterName, _T("Today")); break;
    case TimeFilter::ThisWeek: _tcscpy_s(szTimeFilterName, _T("ThisWeek")); break;
    default: _tcscpy_s(szTimeFilterName, _T("All")); break;
    }
    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("[UpdateLists] ENTRY: m_timeFilter=%d (%s)\n"), (int)m_timeFilter, szTimeFilterName);
    ::OutputDebugString(szDebug);

    // 记录筛选条件
    LPCTSTR pszTimeFilter = nullptr;
    switch (m_timeFilter) {
    case TimeFilter::Today: pszTimeFilter = L"今天"; break;
    case TimeFilter::ThisWeek: pszTimeFilter = L"本周"; break;
    default: pszTimeFilter = L"全部"; break;
    }

    if (!m_searchKeyword.IsEmpty()) {
        _stprintf_s(szDebug, _T("[列表] 刷新: 搜索='%s', 项目=%s, 时间=%s\n"),
            (LPCTSTR)m_searchKeyword, m_currentProjectFilter.empty() ? L"[全部]" : m_currentProjectFilter.c_str(), pszTimeFilter);
    } else if (!m_currentProjectFilter.empty()) {
        _stprintf_s(szDebug, _T("[列表] 刷新: 项目=%s, 时间=%s\n"),
            m_currentProjectFilter.c_str(), pszTimeFilter);
    } else {
        _stprintf_s(szDebug, _T("[列表] 刷新: 时间=%s\n"), pszTimeFilter);
    }
    ::OutputDebugString(szDebug);

    // 设置筛选条件并刷新
    m_todoList.SetSearchKeyword(std::wstring(m_searchKeyword));
    m_todoList.SetProjectFilter(m_currentProjectFilter);
    m_todoList.SetTimeFilter((int)m_timeFilter);
    m_doneList.SetSearchKeyword(std::wstring(m_searchKeyword));
    m_doneList.SetProjectFilter(m_currentProjectFilter);
    m_doneList.SetTimeFilter((int)m_timeFilter);

    m_todoList.RefreshList();
    m_doneList.RefreshList();

    // 输出结果汇总
    int todoResult = m_todoList.GetItemCount();
    int doneResult = m_doneList.GetItemCount();
    _stprintf_s(szDebug, _T("[列表] 结果: 待办=%d, 已完成=%d\n"), todoResult, doneResult);
    ::OutputDebugString(szDebug);
}

void CMainFrame::CreateDetailPanelControls()
{
    HFONT hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    m_detailEmpty.Create(m_detailPanel, rcDefault, _T("点击任务查看详情"),
        WS_CHILD | ES_CENTER | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailEmpty.SetFont(hNormalFont);

    m_detailPriority.Create(m_detailPanel, rcDefault, _T("优先级："),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailPriority.SetFont(hNormalFont);

    m_detailDescription.Create(m_detailPanel, rcDefault, _T("任务描述："),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailDescription.SetFont(hNormalFont);

    m_detailCreateTime.Create(m_detailPanel, rcDefault, _T("创建时间："),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailCreateTime.SetFont(hNormalFont);

    m_detailEndTime.Create(m_detailPanel, rcDefault, _T("截止时间："),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailEndTime.SetFont(hNormalFont);

    m_detailNote.Create(m_detailPanel, rcDefault, _T("备注："),
        WS_CHILD | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        WS_EX_CLIENTEDGE);
    m_detailNote.SetFont(hNormalFont);
}

void CMainFrame::UpdateDetailPanel(int index, bool isDoneList)
{
    if (index < 0) {
        m_detailEmpty.ShowWindow(SW_SHOW);
        m_detailPriority.ShowWindow(SW_HIDE);
        m_detailDescription.ShowWindow(SW_HIDE);
        m_detailCreateTime.ShowWindow(SW_HIDE);
        m_detailEndTime.ShowWindow(SW_HIDE);
        m_detailNote.ShowWindow(SW_HIDE);
        return;
    }

    const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
    if (!pItem) {
        return;
    }

    m_detailEmpty.ShowWindow(SW_HIDE);
    m_detailPriority.ShowWindow(SW_SHOW);
    m_detailDescription.ShowWindow(SW_SHOW);
    m_detailCreateTime.ShowWindow(SW_SHOW);
    m_detailEndTime.ShowWindow(SW_SHOW);
    m_detailNote.ShowWindow(SW_SHOW);

    CString strText;

    strText.Format(_T("优先级：%s"), pItem->GetPriorityString());
    m_detailPriority.SetWindowText(strText);

    strText.Format(_T("任务描述：%s"), pItem->title.c_str());
    m_detailDescription.SetWindowText(strText);

    strText.Format(_T("创建时间：%s"), pItem->GetCreateTimeString());
    m_detailCreateTime.SetWindowText(strText);

    if (pItem->targetEndTime.GetTime() > 0) {
        strText.Format(_T("截止时间：%s"), pItem->GetEndTimeString());
    } else {
        strText = _T("截止时间：未设置");
    }
    m_detailEndTime.SetWindowText(strText);

    strText.Format(_T("备注：%s"), pItem->note.empty() ? _T("(无)") : pItem->note.c_str());
    m_detailNote.SetWindowText(strText);

    RECT rcPanel;
    m_detailPanel.GetClientRect(&rcPanel);

    int x = 10;
    int y = 10;
    int width = rcPanel.right - rcPanel.left - 20;
    int lineHeight = 18;
    int gapSmall = 4;
    int gapLarge = 8;

    m_detailPriority.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    m_detailDescription.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    m_detailCreateTime.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    m_detailEndTime.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    int noteHeight = rcPanel.bottom - rcPanel.top - y - 10;
    if (noteHeight < lineHeight) noteHeight = lineHeight;
    m_detailNote.MoveWindow(x, y, width, noteHeight);

    m_detailEmpty.MoveWindow(0, 0, rcPanel.right - rcPanel.left, rcPanel.bottom - rcPanel.top);
}

void CMainFrame::ShowDetailPopup()
{
    RECT rcMainSplitter;
    m_mainSplitter.GetWindowRect(&rcMainSplitter);

    int width = 300;
    int height = rcMainSplitter.bottom - rcMainSplitter.top;

    m_detailPanel.SetWindowPos(HWND_TOP, rcMainSplitter.right - width, rcMainSplitter.top, width, height, SWP_NOZORDER | SWP_SHOWWINDOW);
    m_bDetailVisible = true;
}

void CMainFrame::HideDetailPopup()
{
    m_detailPanel.ShowWindow(SW_HIDE);
    m_bDetailVisible = false;
}

LRESULT CMainFrame::OnTodoAdd(WORD, WORD, HWND, BOOL&)
{
    ::OutputDebugString(_T("OnTodoAdd called\n"));

    CAddTodoDlg dlg;
    
    // 收集可用项目列表并传递给对话框
    std::vector<std::wstring> projects;
    std::set<std::wstring> projectSet;
    for (const auto& item : m_dataManager.todoItems) {
        if (!item.project.empty()) {
            projectSet.insert(item.project);
        }
    }
    for (const auto& item : m_dataManager.doneItems) {
        if (!item.project.empty()) {
            projectSet.insert(item.project);
        }
    }
    projects.assign(projectSet.begin(), projectSet.end());
    dlg.SetProjects(projects);

    INT_PTR nRet = dlg.DoModal();

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

        CSQLiteManager dbManager;
        if (dbManager.Initialize()) {
            dbManager.SaveTodo(item);
            ::OutputDebugString(_T("Saved to database\n"));
        }

        UpdateLists();
        UpdateProjectFilterList();

        CString strMsg;
        CString strTitle(item.title.c_str());
        strMsg.Format(_T("任务 \"%s\" 已添加"), (LPCTSTR)strTitle);
        m_statusBar.SetText(0, (LPCTSTR)strMsg, 0);

        ::SetTimer(m_hWnd, 1001, 3000, nullptr);
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
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
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

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoDelete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            UINT id = pItem->id;
            if (m_dataManager.DeleteTodo(id, isDoneList)) {
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.DeleteTodo(id);
                }
                UpdateLists();
                HideDetailPopup();
            }
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoEdit(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    if (index >= 0) {
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            CAddTodoDlg dlg(*pItem);
            INT_PTR nRet = dlg.DoModal();

            if (nRet == IDOK) {
                TodoItem updatedItem = dlg.GetResult();
                updatedItem.id = pItem->id;
                updatedItem.createTime = pItem->createTime;
                updatedItem.isDone = pItem->isDone;
                updatedItem.actualDoneTime = pItem->actualDoneTime;

                if (m_dataManager.UpdateTodo(updatedItem, isDoneList)) {
                    CSQLiteManager dbManager;
                    if (dbManager.Initialize()) {
                        dbManager.UpdateTodo(updatedItem);
                    }
        UpdateLists();
        UpdateProjectFilterList();
                    ShowDetailPopup();
                    UpdateDetailPanel(index, isDoneList);
                }
            }
        }
    }

    bHandled = TRUE;
    return 0;
}

LRESULT CMainFrame::OnTodoContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    m_nSelectedIndex = index;
    m_bSelectedIsDone = isDoneList;

    if (index >= 0) {
        POINT pt;
        ::GetCursorPos(&pt);
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

LRESULT CMainFrame::OnContextPin(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0 && !m_bSelectedIsDone) {
        const TodoItem* pItem = m_dataManager.GetItemAt(m_nSelectedIndex, false);
        if (pItem) {
            TodoItem item = *pItem;
            item.isPinned = !item.isPinned;
            
            if (m_dataManager.UpdateTodo(item, false)) {
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
    ClientToScreen(&pt);

    CMenu menu;
    menu.CreatePopupMenu();

    if (!isDoneList) {
        menu.AppendMenu(MF_STRING, ID_CONTEXT_MARK_DONE, L"标记为完成");
        menu.AppendMenu(MF_STRING, ID_CONTEXT_EDIT, L"编辑");
        
        const TodoItem* pItem = m_dataManager.GetItemAt(index, isDoneList);
        if (pItem) {
            if (pItem->isPinned) {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, L"取消置顶");
            } else {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, L"置顶");
            }
        }
    }
    menu.AppendMenu(MF_STRING, ID_CONTEXT_COPY_TEXT, L"复制文本");

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
    CString strFilter = _T("CSV 文件 (*.csv)|*.csv|所有文件 (*.*)|*.*||");
    CFileDialog dlg(FALSE, _T("csv"), _T("todos"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, m_hWnd);

    if (dlg.DoModal() == IDOK) {
        CString strPath = dlg.m_szFileName;

        FILE* fp = nullptr;
        if (_tfopen_s(&fp, strPath, _T("w, ccs=UTF-8")) == 0 && fp) {
            _ftprintf(fp, _T("Todo 列表\n"));
            _ftprintf(fp, _T("优先级,描述,分类,创建时间,截止时间\n"));
            for (const auto& item : m_dataManager.todoItems) {
                CString strTitle(item.title.c_str());
                CString strProject(item.project.c_str());
                _ftprintf(fp, _T("%s,%s,%s,%s,%s\n"),
                    (LPCTSTR)item.GetPriorityString(),
                    (LPCTSTR)strTitle,
                    (LPCTSTR)strProject,
                    (LPCTSTR)item.GetCreateTimeString(),
                    (LPCTSTR)item.GetEndTimeString());
            }

            _ftprintf(fp, _T("\nDone 列表\n"));
            _ftprintf(fp, _T("优先级,描述,分类,完成时间\n"));
            for (const auto& item : m_dataManager.doneItems) {
                CString strTitle(item.title.c_str());
                CString strProject(item.project.c_str());
                _ftprintf(fp, _T("%s,%s,%s,%s\n"),
                    (LPCTSTR)item.GetPriorityString(),
                    (LPCTSTR)strTitle,
                    (LPCTSTR)strProject,
                    (LPCTSTR)item.GetDoneTimeString());
            }

            fclose(fp);
            MessageBox(_T("导出成功！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

wchar_t PriorityToTodoTxtChar(Priority p)
{
    switch (p) {
    case Priority::P0: return L'A';
    case Priority::P1: return L'B';
    case Priority::P2: return L'C';
    case Priority::P3: return L'D';
    default: return L' ';
    }
}

void CMainFrame::ExportToTodoTxt()
{
    CString strFilter = _T("todo.txt 文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||");
    CFileDialog dlg(FALSE, _T("txt"), _T("todos"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, m_hWnd);

    if (dlg.DoModal() == IDOK) {
        CString strPath = dlg.m_szFileName;

        FILE* fp = nullptr;
        if (_tfopen_s(&fp, strPath, _T("w, ccs=UTF-8")) == 0 && fp) {
            fwprintf_s(fp, L"# todo.txt Export from Simple Todo\n");

            time_t now = time(nullptr);
            struct tm tm_info;
            localtime_s(&tm_info, &now);
            wchar_t time_buf[64];
            wcsftime(time_buf, sizeof(time_buf)/sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_info);
            fwprintf_s(fp, L"# Generated: %s\n", time_buf);
            fwprintf_s(fp, L"# Format: (A-Z) YYYY-MM-DD Task description +project @context\n");
            fwprintf_s(fp, L"# x YYYY-MM-DD for completed tasks\n\n");

            for (const auto& item : m_dataManager.todoItems) {
                wchar_t prioChar = PriorityToTodoTxtChar(item.priority);
                if (prioChar != L' ') {
                    fwprintf_s(fp, L"(%c) ", prioChar);
                }

                if (item.createTime.GetTime() > 0) {
                    struct tm create_tm;
                    item.createTime.GetLocalTm(&create_tm);
                    fwprintf_s(fp, L"%04d-%02d-%02d ",
                        create_tm.tm_year + 1900,
                        create_tm.tm_mon + 1,
                        create_tm.tm_mday);
                }

                fwprintf_s(fp, L"%s", item.title.c_str());

                // 输出 project 分类
                if (!item.project.empty()) {
                    fwprintf_s(fp, L" +%s", item.project.c_str());
                }

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

            fwprintf_s(fp, L"\n# Completed Tasks\n");

            for (const auto& item : m_dataManager.doneItems) {
                fwprintf_s(fp, L"x ");

                if (item.actualDoneTime.GetTime() > 0) {
                    struct tm done_tm;
                    item.actualDoneTime.GetLocalTm(&done_tm);
                    fwprintf_s(fp, L"%04d-%02d-%02d ",
                        done_tm.tm_year + 1900,
                        done_tm.tm_mon + 1,
                        done_tm.tm_mday);
                }

                wchar_t prioChar = PriorityToTodoTxtChar(item.priority);
                if (prioChar != L' ') {
                    fwprintf_s(fp, L"(%c) ", prioChar);
                }

                fwprintf_s(fp, L"%s", item.title.c_str());

                // 输出 project 分类
                if (!item.project.empty()) {
                    fwprintf_s(fp, L" +%s", item.project.c_str());
                }

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

            fwprintf_s(fp, L"\n# Completed Tasks\n");

            for (const auto& item : m_dataManager.doneItems) {
                fwprintf_s(fp, L"x ");

                if (item.actualDoneTime.GetTime() > 0) {
                    struct tm done_tm;
                    item.actualDoneTime.GetLocalTm(&done_tm);
                    fwprintf_s(fp, L"%04d-%02d-%02d ",
                        done_tm.tm_year + 1900,
                        done_tm.tm_mon + 1,
                        done_tm.tm_mday);
                }

                wchar_t prioChar = PriorityToTodoTxtChar(item.priority);
                if (prioChar != L' ') {
                    fwprintf_s(fp, L"(%c) ", (wchar_t)prioChar);
                }

                fwprintf_s(fp, L"%s", item.title.c_str());

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

LRESULT CMainFrame::OnToggleTopmost(WORD, WORD, HWND, BOOL&)
{
    ::OutputDebugString(_T("[OnToggleTopmost] 开始处理置顶\n"));

    m_bTopmost = !m_bTopmost;

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("[OnToggleTopmost] m_bTopmost=%d\n"), m_bTopmost);
    ::OutputDebugString(szDebug);

    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (LPTSTR)(m_bTopmost ? TOPMOST_TEXT_CHECKED : TOPMOST_TEXT_NORMAL);
    m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbbi);

    ::OutputDebugString(_T("[OnToggleTopmost] 调用 SetWindowPos\n"));
    ::SetWindowPos(m_hWnd, m_bTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    ::OutputDebugString(_T("[OnToggleTopmost] 处理完成\n"));
    return 0;
}

LRESULT CMainFrame::OnToggleTimeFilter(WORD, WORD, HWND, BOOL&)
{
    // 关键日志：记录切换前的状态
    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] BEFORE: m_timeFilter=%d\n"), (int)m_timeFilter);
    ::OutputDebugString(szDebug);

    // 轮询切换: 全部 -> 今天 -> 本周 -> 全部
    LPCTSTR pszNewFilter = nullptr;
    switch (m_timeFilter) {
    case TimeFilter::All:
        m_timeFilter = TimeFilter::Today;
        pszNewFilter = TIME_FILTER_TODAY;
        break;
    case TimeFilter::Today:
        m_timeFilter = TimeFilter::ThisWeek;
        pszNewFilter = TIME_FILTER_WEEK;
        break;
    case TimeFilter::ThisWeek:
    default:
        m_timeFilter = TimeFilter::All;
        pszNewFilter = TIME_FILTER_ALL;
        break;
    }

    // 关键日志：记录切换后的状态
    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] AFTER: m_timeFilter=%d, pszNewFilter=%s\n"), (int)m_timeFilter, pszNewFilter);
    ::OutputDebugString(szDebug);

    // 更新按钮文字
    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (LPTSTR)pszNewFilter;
    m_toolbar.SetButtonInfo(ID_TIME_FILTER, &tbbi);

    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] CALL UpdateLists\n"));
    ::OutputDebugString(szDebug);

    // 刷新列表（时间筛选结果会在 UpdateLists 中输出）
    UpdateLists();

    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] END\n"));
    ::OutputDebugString(szDebug);
    return 0;
}

void CMainFrame::OnSearchChanged()
{
    // 重置搜索定时器（防抖 500ms）
    ::KillTimer(m_hWnd, SEARCH_TIMER_ID);
    ::SetTimer(m_hWnd, SEARCH_TIMER_ID, 500, nullptr);
}

LRESULT CMainFrame::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    if (wParam == 1001) {
        m_statusBar.SetText(0, _T(""), 0);
        ::KillTimer(m_hWnd, 1001);
    } else if (wParam == 2000) {
        ::OutputDebugString(_T("Timer 2000: Force refresh\n"));
        ::KillTimer(m_hWnd, 2000);
        UpdateLists();
    } else if (wParam == SEARCH_TIMER_ID) {
        // 搜索定时器触发
        ::KillTimer(m_hWnd, SEARCH_TIMER_ID);

        // 读取搜索框内容
        int len = m_searchEdit.GetWindowTextLength();
        if (len > 0) {
            m_searchEdit.GetWindowText(m_searchKeyword.GetBuffer(len + 1), len + 1);
            m_searchKeyword.ReleaseBuffer();
        } else {
            m_searchKeyword.Empty();
        }

        // 刷新列表显示
        UpdateLists();
    }
    return 0;
}

LRESULT CMainFrame::OnLanguageChinese(WORD, WORD, HWND, BOOL&)
{
    m_bChineseLanguage = true;
    return 0;
}

LRESULT CMainFrame::OnLanguageEnglish(WORD, WORD, HWND, BOOL&)
{
    m_bChineseLanguage = false;
    return 0;
}

void CMainFrame::UpdateProjectFilterList()
{
    // 收集所有项目名称
    std::set<std::wstring> projects;
    for (const auto& item : m_dataManager.todoItems) {
        if (!item.project.empty()) {
            projects.insert(item.project);
        }
    }
    for (const auto& item : m_dataManager.doneItems) {
        if (!item.project.empty()) {
            projects.insert(item.project);
        }
    }

    // 保存当前选中的项目
    int curSel = m_projectFilter.GetCurSel();
    CString currentText;
    if (curSel >= 0) {
        m_projectFilter.GetLBText(curSel, currentText);
    }

    // 清空并重新填充
    m_projectFilter.ResetContent();
    m_projectFilter.AddString(L"[全部]");

    for (const auto& proj : projects) {
        m_projectFilter.AddString(proj.c_str());
    }

    // 恢复选中状态或默认选中"全部"
    if (!currentText.IsEmpty()) {
        int found = m_projectFilter.FindStringExact(-1, currentText);
        if (found >= 0) {
            m_projectFilter.SetCurSel(found);
        } else {
            m_projectFilter.SetCurSel(0);
        }
    } else {
        m_projectFilter.SetCurSel(0);
    }
}

LRESULT CMainFrame::OnProjectFilterChanged(WORD, WORD, HWND, BOOL&)
{
    CString selText;
    m_projectFilter.GetWindowText(selText);
    selText.Trim();

    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("=== OnProjectFilterChanged START === selText='%s'\n"), (LPCTSTR)selText);
    ::OutputDebugString(szDebug);

    if (selText.IsEmpty() || selText == L"[全部]") {
        m_currentProjectFilter.clear();
        _stprintf_s(szDebug, _T("OnProjectFilterChanged: 选中[全部], filter='%s'\n"),
            m_currentProjectFilter.c_str());
    } else {
        m_currentProjectFilter = selText.GetString();
        _stprintf_s(szDebug, _T("OnProjectFilterChanged: 选中项目='%s'\n"),
            m_currentProjectFilter.c_str());
    }
    ::OutputDebugString(szDebug);

    // 刷新列表显示
    ::OutputDebugString(_T("OnProjectFilterChanged: 调用 UpdateLists()\n"));
    UpdateLists();

    _stprintf_s(szDebug, _T("=== OnProjectFilterChanged END === filter='%s'\n"),
        m_currentProjectFilter.c_str());
    ::OutputDebugString(szDebug);
    return 0;
}

// 窗口设置注册表键名
const TCHAR* CMainFrame::REG_KEY_PATH = _T("Software\\SimpleTodo");

void CMainFrame::LoadWindowSettings()
{
    HKEY hKey;
    TCHAR szValue[256];
    DWORD dwType, dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // 读取窗口位置和大小
        dwSize = sizeof(szValue);
        if (RegQueryValueEx(hKey, _T("WindowPos"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS && dwType == REG_SZ) {
            int x, y, cx, cy;
            if (swscanf_s(szValue, _T("%d,%d,%d,%d"), &x, &y, &cx, &cy) == 4) {
                // 验证位置是否在屏幕范围内
                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);

                if (x >= 0 && y >= 0 && cx > 0 && cy > 0 &&
                    x < screenWidth && y < screenHeight) {
                    ::SetWindowPos(m_hWnd, NULL, x, y, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        }

        // 读取窗口状态
        dwSize = sizeof(szValue);
        if (RegQueryValueEx(hKey, _T("WindowMax"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS && dwType == REG_SZ) {
            if (_tcscmp(szValue, _T("1")) == 0) {
                ::ShowWindow(m_hWnd, SW_MAXIMIZE);
            }
        }

        // 读取分割条位置
        dwSize = sizeof(szValue);
        if (RegQueryValueEx(hKey, _T("SplitterPos"), NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS && dwType == REG_SZ) {
            int pos;
            if (swscanf_s(szValue, _T("%d"), &pos) == 1) {
                m_nSplitterPos = pos;
            }
        }

        RegCloseKey(hKey);
    }
}

void CMainFrame::SaveWindowSettings()
{
    HKEY hKey;
    DWORD dwDisposition;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        // 保存窗口位置和大小
        WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
        if (::GetWindowPlacement(m_hWnd, &wp)) {
            TCHAR szValue[256];
            if (wp.showCmd == SW_MAXIMIZE) {
                // 最大化时保存的是恢复后的位置
                _stprintf_s(szValue, _T("%d,%d,%d,%d"),
                    wp.rcNormalPosition.left,
                    wp.rcNormalPosition.top,
                    wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                    wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
            } else {
                _stprintf_s(szValue, _T("%d,%d,%d,%d"),
                    wp.rcNormalPosition.left,
                    wp.rcNormalPosition.top,
                    wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                    wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
            }
            RegSetValueEx(hKey, _T("WindowPos"), 0, REG_SZ, (LPBYTE)szValue, (DWORD)(_tcslen(szValue) + 1) * sizeof(TCHAR));
        }

        // 保存窗口状态
        TCHAR szMax[2] = { _T('0') };
        if (::IsZoomed(m_hWnd)) {
            szMax[0] = _T('1');
        }
        RegSetValueEx(hKey, _T("WindowMax"), 0, REG_SZ, (LPBYTE)szMax, sizeof(szMax));

        // 保存分割条位置
        int splitterPos = m_mainSplitter.GetSplitterPos();
        TCHAR szSplitter[32];
        _stprintf_s(szSplitter, _T("%d"), splitterPos);
        RegSetValueEx(hKey, _T("SplitterPos"), 0, REG_SZ, (LPBYTE)szSplitter, (DWORD)(_tcslen(szSplitter) + 1) * sizeof(TCHAR));

        RegCloseKey(hKey);
    }
}
