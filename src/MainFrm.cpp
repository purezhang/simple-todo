#include "stdafx.h"
#include <commctrl.h>
#include "MainFrm.h"
#include "AddTodoDlg.h"
#include "SQLiteManager.h"

#define TOPMOST_TEXT_NORMAL   _T("📌 窗口置顶")
#define TOPMOST_TEXT_CHECKED  _T("📌 取消置顶")

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
#ifdef _DEBUG
    AllocConsole();
    SetConsoleTitle(_T("SimpleTodo Debug Console"));
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    DebugLog(_T("SimpleTodo Application Starting\n"));
#endif

    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    m_fontList.CreateFontIndirect(&ncm.lfMessageFont);

    m_imgList.Create(1, 20, ILC_COLOR32, 0, 0);

    m_rebar.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS);

    m_toolbar.Create(m_rebar, rcDefault, NULL, 
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
        TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN, 
        0, ATL_IDW_TOOLBAR);
    
    m_toolbar.SetButtonStructSize();
    
    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_WINDOW_TOPMOST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"窗口置顶" },
        { I_IMAGENONE, ID_VIEW_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"展开全部" },
        { I_IMAGENONE, ID_VIEW_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"收起全部" },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_TODO_ADD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"添加任务" }
    };

    m_toolbar.AddButtons(5, buttons);
    m_toolbar.AutoSize();

    REBARBANDINFO rbbi = { sizeof(REBARBANDINFO) };
    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
    rbbi.hwndChild = m_toolbar;
    rbbi.cxMinChild = 0;
    rbbi.cyMinChild = 24;
    rbbi.cx = -1;
    m_rebar.InsertBand(-1, &rbbi);

    TBBUTTONINFO tbbiInit = { sizeof(TBBUTTONINFO) };
    tbbiInit.dwMask = TBIF_TEXT;
    tbbiInit.pszText = (LPTSTR)(m_bTopmost ? TOPMOST_TEXT_CHECKED : TOPMOST_TEXT_NORMAL);
    m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbbiInit);

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
            ::OutputDebugString(_T("Database is empty, adding test data...\n"));

            CTime now = CTime::GetCurrentTime();
            CTime yesterday = now - CTimeSpan(1, 0, 0, 0);
            CTime dayBefore = now - CTimeSpan(2, 0, 0, 0);

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

            dbManager.SaveAll(m_dataManager);
        }
    }

    UpdateLists();
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(m_todoList, 2, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_doneList, 1, LVSCW_AUTOSIZE);

    SetTimer(2000, 200, nullptr);
    PostMessage(WM_SIZE);

    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
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
        toolbarHeight = rcRebar.bottom - rcRebar.top;

        TCHAR szDebug[64];
        _stprintf_s(szDebug, _T("OnSize: ToolbarHeight=%d\n"), toolbarHeight);
        ::OutputDebugString(szDebug);
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

LRESULT CMainFrame::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    if (wParam == 1001) {
        m_statusBar.SetText(0, _T(""), 0);
        ::KillTimer(m_hWnd, 1001);
    } else if (wParam == 2000) {
        ::OutputDebugString(_T("Timer 2000: Force refresh\n"));
        ::KillTimer(m_hWnd, 2000);
        UpdateLists();
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
    m_todoList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_todoList.InsertColumn(0, L"创建日期", LVCFMT_LEFT, 90);
    m_todoList.InsertColumn(1, L"优先级", LVCFMT_CENTER, 55);
    m_todoList.InsertColumn(2, L"任务描述", LVCFMT_LEFT, 380);
    m_todoList.InsertColumn(3, L"截止时间", LVCFMT_LEFT, 100);

    m_doneList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_doneList.InsertColumn(0, L"优先级", LVCFMT_CENTER, 55);
    m_doneList.InsertColumn(1, L"任务描述", LVCFMT_LEFT, 380);
    m_doneList.InsertColumn(2, L"完成时间", LVCFMT_LEFT, 120);
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

    _stprintf_s(szDebug, _T("ListRefresh: TodoItems=%d, DoneItems=%d\n"),
        m_todoList.GetItemCount(), m_doneList.GetItemCount());
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
    m_bTopmost = !m_bTopmost;

    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (LPTSTR)(m_bTopmost ? TOPMOST_TEXT_CHECKED : TOPMOST_TEXT_NORMAL);
    m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbbi);

    ::SetWindowPos(m_hWnd, m_bTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

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
