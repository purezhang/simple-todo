#include "stdafx.h"
#include <commctrl.h>
#include "MainFrm.h"
#include "AddTodoDlg.h"
#include "SQLiteManager.h"
#include "version.h"

// Debug 日志宏 - 仅在 Debug 模式下生效
#ifdef _DEBUG
    #define DEBUG_OUTPUT(msg) ::OutputDebugString(msg)
#else
    #define DEBUG_OUTPUT(msg) ((void)0)
#endif

#ifdef _DEBUG
static void DebugCommandSource(const TCHAR* tag, WPARAM wParam, LPARAM lParam, HWND hSelf)
{
    TCHAR buf[512];
    HWND hFocus = ::GetFocus();
    _stprintf_s(buf, _T("[CmdSrc] %s id=0x%04X code=0x%04X lParam=0x%p focus=0x%p self=0x%p\n"),
        tag, LOWORD(wParam), HIWORD(wParam), (void*)lParam, (void*)hFocus, (void*)hSelf);
    ::OutputDebugString(buf);
}
#else
static void DebugCommandSource(const TCHAR*, WPARAM, LPARAM, HWND) {}
#endif

// 详情面板窗口过程（用于转发按钮消息）
static LRESULT CALLBACK DetailPanelWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 获取保存的原始窗口过程
    WNDPROC originalWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (uMsg == WM_COMMAND) {
        // 转发按钮消息给父窗口（CMainFrame）
        HWND hParent = ::GetParent(hWnd);
        if (hParent) {
            ::SendMessage(hParent, uMsg, wParam, lParam);
        }
    }

    // 调用原始窗口过程
    if (originalWndProc) {
        return ::CallWindowProc(originalWndProc, hWnd, uMsg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 搜索容器窗口过程（用于转发 EN_CHANGE 等消息到主窗口）
static LRESULT CALLBACK SearchContainerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // GWLP_USERDATA + sizeof(LONG_PTR) 存储原始窗口过程
    WNDPROC originalWndProc = (WNDPROC)::GetWindowLongPtr(hWnd, GWLP_USERDATA + sizeof(LONG_PTR));

    if (uMsg == WM_COMMAND) {
        // 从 GWLP_USERDATA 获取主窗口句柄（由 OnCreate 设置）
        HWND hMainWnd = (HWND)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
        // 健壮性：如果未设置或无效，使用 GetAncestor 获取顶层窗口
        if (!hMainWnd || !::IsWindow(hMainWnd)) {
            hMainWnd = ::GetAncestor(hWnd, GA_ROOTOWNER);
        }
        if (hMainWnd) {
            ::SendMessage(hMainWnd, uMsg, wParam, lParam);
        }
    }

    if (originalWndProc) {
        return ::CallWindowProc(originalWndProc, hWnd, uMsg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 工具栏按钮文字（将被 GetString 替换）
#define TOPMOST_TEXT_NORMAL   _T("📌置顶")
#define TOPMOST_TEXT_CHECKED  _T("📌取消")
#define TIME_FILTER_TODAY    _T("🏷今天")
#define TIME_FILTER_WEEK     _T("🏷本周")
#define TIME_FILTER_ALL     _T("🏷全部")

// ============================================================================
// 国际化字符串表
// ============================================================================
static bool g_bChineseLanguage = true;  // 全局语言标志

static const wchar_t* g_strings_chinese[] = {
    // 通用
    L"提示",                  // Tips
    L"关于 Simple Todo",      // AboutTitle
    L"确定",                  // OK
    L"取消",                  // Cancel
    L"关闭",                  // Close
    L"是",                    // Yes
    L"否",                    // No

    // 任务相关
    L"请输入任务标题！",      // TitleRequired
    L"点击任务查看详情",      // ClickToViewDetail

    // 对话框标签
    L"添加任务",              // DlgAddTodo
    L"编辑任务",              // DlgEditTodo
    L"标题 *",                // LblTitle
    L"备注",                  // LblNote
    L"优先级",                // LblPriority
    L"项目",                  // LblProject
    L"截止时间",              // LblDeadline
    L"今天",                  // BtnToday
    L"明天",                  // BtnTomorrow
    L"本周",                  // BtnThisWeek

    // 优先级
    L"P0 紧急",               // PriorityP0
    L"P1 重要",               // PriorityP1
    L"P2 普通",               // PriorityP2
    L"P3 暂缓",               // PriorityP3

    // 右键菜单
    L"标记为完成",            // MarkAsDone
    L"标记为未完成",          // MarkAsUndone
    L"编辑",                  // Edit
    L"删除",                  // Delete
    L"置顶",                  // Pin
    L"取消置顶",              // Unpin
    L"复制文本",              // CopyText
    L"设置优先级",            // SetPriority

    // 列标题
    L"创建日期",              // ColCreateDate
    L"优先级",                // ColPriority
    L"任务描述",              // ColDescription
    L"截止时间",              // ColDeadline
    L"完成时间",              // ColDoneTime

    // 筛选器
    L"全部",                  // FilterAll
    L"今天",                  // FilterToday
    L"本周",                  // FilterThisWeek
    L"[全部]",                // ProjectAll
    L"[无]",                  // ProjectNone

    // 工具栏
    L"📌置顶",                // TbTopmost
    L"📌已顶",                // TbTopmostOn
    L"🏷全部",                // TbFilter
    L"🏷今天",                // TbFilterToday
    L"🏷本周",                // TbFilterWeek
    L"🆕新增",                // TbAdd

    // 详情面板
    L"优先级：",              // DetailPriority
    L"任务描述：",            // DetailDescription
    L"创建时间：",            // DetailCreateTime
    L"截止时间：",            // DetailDeadline
    L"截止时间：未设置",      // DetailDeadlineNone
    L"分组：",                // DetailProject
    L"备注：",                // DetailNote
    L"(无)",                  // DetailNone
    L"固定",                  // BtnPin
    L"取消",                  // BtnUnpin

    // 状态栏
    L"就绪",                  // StatusReady

    // 导出
    L"导出成功！",            // ExportSuccess
};

static const wchar_t* g_strings_english[] = {
    // 通用
    L"Tips",                  // Tips
    L"About Simple Todo",     // AboutTitle
    L"OK",                    // OK
    L"Cancel",                // Cancel
    L"Close",                 // Close
    L"Yes",                   // Yes
    L"No",                    // No

    // 任务相关
    L"Task title is required!",    // TitleRequired
    L"Click a task to view details", // ClickToViewDetail

    // 对话框标签
    L"Add Todo",              // DlgAddTodo
    L"Edit Todo",             // DlgEditTodo
    L"Title *",               // LblTitle
    L"Note",                  // LblNote
    L"Priority",              // LblPriority
    L"Project",               // LblProject
    L"Deadline",              // LblDeadline
    L"Today",                 // BtnToday
    L"Tomorrow",              // BtnTomorrow
    L"This Week",             // BtnThisWeek

    // 优先级
    L"P0 Urgent",             // PriorityP0
    L"P1 Important",          // PriorityP1
    L"P2 Normal",             // PriorityP2
    L"P3 Low",                // PriorityP3

    // 右键菜单
    L"Mark as Done",          // MarkAsDone
    L"Mark as Undone",        // MarkAsUndone
    L"Edit",                  // Edit
    L"Delete",                // Delete
    L"Pin to Top",            // Pin
    L"Unpin",                 // Unpin
    L"Copy Text",             // CopyText
    L"Set Priority",          // SetPriority

    // 列标题
    L"Create Date",           // ColCreateDate
    L"Priority",              // ColPriority
    L"Description",           // ColDescription
    L"Deadline",              // ColDeadline
    L"Done Time",             // ColDoneTime

    // 筛选器
    L"All",                   // FilterAll
    L"Today",                 // FilterToday
    L"This Week",             // FilterThisWeek
    L"[All]",                 // ProjectAll
    L"[None]",                // ProjectNone

    // 工具栏
    L"📌Pin",                 // TbTopmost
    L"📌Pinned",              // TbTopmostOn
    L"🏷All",                 // TbFilter
    L"🏷Today",               // TbFilterToday
    L"🏷Week",                // TbFilterWeek
    L"🆕Add",                 // TbAdd

    // 详情面板
    L"Priority: ",            // DetailPriority
    L"Description: ",         // DetailDescription
    L"Created: ",             // DetailCreateTime
    L"Deadline: ",            // DetailDeadline
    L"Deadline: Not set",     // DetailDeadlineNone
    L"Project: ",             // DetailProject
    L"Note: ",                // DetailNote
    L"(None)",                // DetailNone
    L"Pin",                   // BtnPin
    L"Unpin",                 // BtnUnpin

    // 状态栏
    L"Ready",                 // StatusReady

    // 导出
    L"Export successful!",    // ExportSuccess
};

// 编译期校验数组长度
static_assert(sizeof(g_strings_chinese) / sizeof(g_strings_chinese[0]) == (int)StringID::COUNT,
    "Chinese string table size mismatch with StringID::COUNT");
static_assert(sizeof(g_strings_english) / sizeof(g_strings_english[0]) == (int)StringID::COUNT,
    "English string table size mismatch with StringID::COUNT");

// 获取当前语言字符串
LPCTSTR GetString(StringID id) {
    const wchar_t** table = g_bChineseLanguage ? g_strings_chinese : g_strings_english;
    return table[static_cast<int>(id)];
}

void DebugLog(const TCHAR* format, ...) {
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(TCHAR), format, args);
    va_end(args);
    DEBUG_OUTPUT(buffer);
    
#ifdef _DEBUG
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsole(hConsole, buffer, (DWORD)_tcslen(buffer), &written, NULL);
    }
#endif
}

CMainFrame::CMainFrame()
    : m_nSelectedIndex(-1), m_bSelectedIsDone(false), m_bChineseLanguage(true),
      m_bTopmost(false), m_bFirstSize(true), m_bDetailVisible(false), m_bDetailPinned(false),
      m_nSplitterPos(0), m_timeFilter(TimeFilter::All),
      m_originalReBarWndProc(nullptr), m_originalSearchContainerWndProc(nullptr),
      m_originalComboWndProc(nullptr), m_originalDetailPanelWndProc(nullptr)
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

    // 点击空白处关闭详情面板（除非已固定）
    if (pMsg->message == WM_LBUTTONDOWN) {
        DEBUG_OUTPUT(_T("[PreTranslate] WM_LBUTTONDOWN\n"));
        if (m_bDetailVisible) {
            DEBUG_OUTPUT(_T("[PreTranslate] m_bDetailVisible=true\n"));
            if (m_bDetailPinned) {
                DEBUG_OUTPUT(_T("[PreTranslate] m_bDetailPinned=true (固定), 跳过关闭\n"));
                return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
            }
            CPoint pt(GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam));
            // 将客户端坐标转换为屏幕坐标
            ClientToScreen(&pt);
            RECT rcDetail;
            m_detailPanel.GetWindowRect(&rcDetail);
            DEBUG_OUTPUT(_T("[PreTranslate] 面板区域\n"));
            if (!PtInRect(&rcDetail, pt)) {
                DEBUG_OUTPUT(_T("[PreTranslate] 点击空白处，关闭面板\n"));
                HideDetailPopup();
                return TRUE;
            }
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
    DEBUG_OUTPUT(_T("[OnCreate] 入口\n"));

    // --- 1. 优先获取 DPI（必须放在最前面） ---
    int dpi = 96;
    HDC hdc = ::GetDC(NULL);
    if (hdc) {
        dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
        ::ReleaseDC(NULL, hdc);
    }

    // 定义统一的布局尺寸（基于 96 DPI 缩放）
    const int ROW_HEIGHT = MulDiv(34, dpi, 96);    // ReBar 每一行的总高度
    const int CTRL_HEIGHT = MulDiv(24, dpi, 96);    // 控件（按钮、输入框）的实际高度

    // --- 2. 基础资源初始化 ---
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
    m_fontList.CreateFontIndirect(&ncm.lfMessageFont);
    m_imgList.Create(1, 20, ILC_COLOR32, 0, 0);

    // --- 3. 创建 ReBar ---
    m_rebar.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE);

    DEBUG_OUTPUT(_T("[OnCreate] ReBar 创建完成，样式已设置\n"));

    // --- 4. 创建并配置 ToolBar ---
    m_toolbar.Create(m_rebar, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
        TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
        0, ATL_IDW_TOOLBAR);

    // 必须设置按钮结构大小
    m_toolbar.SetButtonStructSize(sizeof(TBBUTTON));

    // 设置工具栏按钮的统一尺寸 (必须在 AddButtons 之前调用!)
    // 高度使用 ROW_HEIGHT 使按钮填满带区，实现垂直居中效果
    int btnWidth = MulDiv(60, dpi, 96);
    m_toolbar.SetButtonSize(CSize(btnWidth, CTRL_HEIGHT));  // 使用 CTRL_HEIGHT 让按钮在带区中垂直居中

    // 添加扩展样式，支持双缓冲防止闪烁
    m_toolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);

    TBBUTTON buttons[] = {
        { I_IMAGENONE, ID_WINDOW_TOPMOST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)GetString(StringID::TbTopmost) },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_TIME_FILTER, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)GetString(StringID::TbFilter) },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { I_IMAGENONE, ID_TODO_ADD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)GetString(StringID::TbAdd) }
    };
    m_toolbar.AddButtons(5, buttons);

    // 调试：输出按钮数量
    int btnCount = m_toolbar.GetButtonCount();
    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("[OnCreate] ToolBar按钮数量=%d\n"), btnCount);
    DEBUG_OUTPUT(szDebug);

    // 保存 ToolBar 句柄用于消息识别
    m_hToolbar = m_toolbar.m_hWnd;

    TCHAR szToolbar[64];
    _stprintf_s(szToolbar, _T("[OnCreate] ToolBar 创建完成，句柄=0x%08X\n"), (UINT_PTR)m_hToolbar);
    DEBUG_OUTPUT(szToolbar);

    // 将工具栏加入 ReBar (利用 cyMinChild > cyChild 实现垂直居中)
    // 使用 TB_GETBUTTONSIZE 获取 Toolbar 真实高度
    DWORD dwBtnSize = (DWORD)m_toolbar.SendMessage(TB_GETBUTTONSIZE);
    int realBtnH = HIWORD(dwBtnSize);

    REBARBANDINFO rbbiToolbar = { sizeof(REBARBANDINFO) };
    rbbiToolbar.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiToolbar.fStyle = RBBS_FIXEDBMP | RBBS_NOGRIPPER | RBBS_CHILDEDGE; // CHILDEDGE 启用边缘居中
    rbbiToolbar.hwndChild = m_toolbar;
    rbbiToolbar.cyChild = realBtnH;         // 使用真实控件高度
    rbbiToolbar.cyMinChild = realBtnH;      // 使用真实高度作为带区高度
    rbbiToolbar.cyMaxChild = realBtnH;     // 限制最大高度，防止 ReBar 扩展 Toolbar
    rbbiToolbar.cxMinChild = MulDiv(200, dpi, 96);  // 修复：设置最小宽度，防止折叠
    rbbiToolbar.cx = MulDiv(300, dpi, 96);    // 工具栏预设宽度
    m_rebar.InsertBand(-1, &rbbiToolbar);

    // === 调试信息：Toolbar 和 ReBar 尺寸 ===
#ifdef _DEBUG
    {
        TCHAR szDbg[512];
        
        // 输出设置的参数
        _stprintf_s(szDbg, _T("[DEBUG] DPI=%d, ROW_HEIGHT=%d, CTRL_HEIGHT=%d\n"), dpi, ROW_HEIGHT, CTRL_HEIGHT);
        DEBUG_OUTPUT(szDbg);
        _stprintf_s(szDbg, _T("[DEBUG] rbbiToolbar: cyChild=%d, cyMinChild=%d\n"), rbbiToolbar.cyChild, rbbiToolbar.cyMinChild);
        DEBUG_OUTPUT(szDbg);
        
        // Toolbar 实际窗口尺寸
        RECT rcToolbar;
        m_toolbar.GetWindowRect(&rcToolbar);
        _stprintf_s(szDbg, _T("[DEBUG] Toolbar WindowRect: L=%d, T=%d, R=%d, B=%d (H=%d)\n"),
            rcToolbar.left, rcToolbar.top, rcToolbar.right, rcToolbar.bottom,
            rcToolbar.bottom - rcToolbar.top);
        DEBUG_OUTPUT(szDbg);
        
        // Toolbar 客户区尺寸
        RECT rcToolbarClient;
        m_toolbar.GetClientRect(&rcToolbarClient);
        _stprintf_s(szDbg, _T("[DEBUG] Toolbar ClientRect: W=%d, H=%d\n"),
            rcToolbarClient.right, rcToolbarClient.bottom);
        DEBUG_OUTPUT(szDbg);
        
        // 获取按钮尺寸
        DWORD dwBtnSize = (DWORD)m_toolbar.SendMessage(TB_GETBUTTONSIZE, 0, 0);
        int btnW = LOWORD(dwBtnSize);
        int btnH = HIWORD(dwBtnSize);
        _stprintf_s(szDbg, _T("[DEBUG] Toolbar ButtonSize: W=%d, H=%d\n"), btnW, btnH);
        DEBUG_OUTPUT(szDbg);
        
        // 获取第一个按钮的位置
        RECT rcBtn0;
        if (m_toolbar.SendMessage(TB_GETITEMRECT, 0, (LPARAM)&rcBtn0)) {
            _stprintf_s(szDbg, _T("[DEBUG] Button[0] ItemRect: L=%d, T=%d, R=%d, B=%d (H=%d)\n"),
                rcBtn0.left, rcBtn0.top, rcBtn0.right, rcBtn0.bottom, rcBtn0.bottom - rcBtn0.top);
            DEBUG_OUTPUT(szDbg);
        }
        
        // ReBar band 信息
        REBARBANDINFO rbbi = { sizeof(REBARBANDINFO) };
        rbbi.fMask = RBBIM_CHILDSIZE;
        if (m_rebar.GetBandInfo(0, &rbbi)) {
            _stprintf_s(szDbg, _T("[DEBUG] ReBar Band[0]: cyChild=%d, cyMinChild=%d, cyMaxChild=%d\n"),
                rbbi.cyChild, rbbi.cyMinChild, rbbi.cyMaxChild);
            DEBUG_OUTPUT(szDbg);
        }
        
        // ReBar 总高度
        RECT rcRebar;
        m_rebar.GetWindowRect(&rcRebar);
        _stprintf_s(szDbg, _T("[DEBUG] ReBar WindowRect: H=%d\n"), rcRebar.bottom - rcRebar.top);
        DEBUG_OUTPUT(szDbg);
    }
#endif

    // --- 5. 创建并配置搜索框 ---
    m_searchContainer.Create(m_rebar, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
    m_searchLabel.Create(m_searchContainer, rcDefault, L"🔍", WS_CHILD | WS_VISIBLE);
    m_searchEdit.Create(m_searchContainer, rcDefault, NULL, 
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, ID_SEARCH_EDIT);
    m_searchEdit.SetFont(m_fontList);
    m_searchLabel.SetFont(m_fontList);

    // 搜索容器内部布局（子控件在容器内居中）
    int iconWidth = MulDiv(20, dpi, 96);
    int editWidth = MulDiv(120, dpi, 96);
    int labelY = (CTRL_HEIGHT - MulDiv(16, dpi, 96)) / 2;
    int editY = (CTRL_HEIGHT - MulDiv(20, dpi, 96)) / 2;
    m_searchLabel.MoveWindow(2, labelY, iconWidth, MulDiv(16, dpi, 96));
    m_searchEdit.MoveWindow(iconWidth + 5, editY, editWidth, MulDiv(20, dpi, 96));

    DEBUG_OUTPUT(_T("[OnCreate] 搜索框创建完成\n"));

    // 子类化搜索容器以转发 WM_COMMAND 消息到主窗口
    if (m_searchContainer.IsWindow()) {
        // 先保存主窗口句柄到 GWLP_USERDATA
        ::SetWindowLongPtr(m_searchContainer.m_hWnd, GWLP_USERDATA, (LONG_PTR)m_hWnd);
        // 再子类化，将原始窗口过程保存到 GWLP_USERDATA + sizeof(LONG_PTR)
        m_originalSearchContainerWndProc = (WNDPROC)::SetWindowLongPtr(
            m_searchContainer.m_hWnd, GWLP_WNDPROC, (LONG_PTR)SearchContainerWndProc);
        ::SetWindowLongPtr(m_searchContainer.m_hWnd, GWLP_USERDATA + sizeof(LONG_PTR),
                          (LONG_PTR)m_originalSearchContainerWndProc);
    }

    // 搜索容器加入 ReBar
    // 使用 GetWindowRect 获取容器真实高度
    RECT rcSearch;
    m_searchContainer.GetWindowRect(&rcSearch);
    int realSearchH = rcSearch.bottom - rcSearch.top;

    REBARBANDINFO rbbiSearch = { sizeof(REBARBANDINFO) };
    rbbiSearch.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiSearch.fStyle = RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiSearch.hwndChild = m_searchContainer;
    rbbiSearch.cyChild = realSearchH;
    rbbiSearch.cyMinChild = realSearchH;       // 使用真实高度
    rbbiSearch.cxMinChild = iconWidth + editWidth + 20;
    rbbiSearch.cx = iconWidth + editWidth + 20;
    m_rebar.InsertBand(-1, &rbbiSearch);

    // --- 6. 项目筛选下拉框 ---
    m_projectFilter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL,
        0, ID_PROJECT_FILTER);
    m_projectFilter.SetFont(m_fontList);
    m_projectFilter.AddString(GetString(StringID::ProjectAll));
    m_projectFilter.SetCurSel(0);

    DEBUG_OUTPUT(_T("[OnCreate] ComboBox 创建完成\n"));

    // ComboBox parent 验证调试输出
    HWND hComboParent = ::GetParent(m_projectFilter.m_hWnd);
    TCHAR szDbg[256];
    _stprintf_s(szDbg, _T("[OnCreate] ComboBox parent=0x%08X (Main=0x%08X, ReBar=0x%08X)\n"),
        (UINT_PTR)hComboParent, (UINT_PTR)m_hWnd, (UINT_PTR)m_rebar.m_hWnd);
    DEBUG_OUTPUT(szDbg);

    // 使用 GetWindowRect 获取 ComboBox 真实高度
    RECT rcCombo;
    m_projectFilter.GetWindowRect(&rcCombo);
    int realComboH = rcCombo.bottom - rcCombo.top;

    REBARBANDINFO rbbiProject = { sizeof(REBARBANDINFO) };
    rbbiProject.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbbiProject.fStyle = RBBS_FIXEDBMP | RBBS_NOGRIPPER;
    rbbiProject.hwndChild = m_projectFilter;
    rbbiProject.cyChild = realComboH;
    rbbiProject.cyMinChild = realComboH;
    rbbiProject.cxMinChild = MulDiv(100, dpi, 96);
    rbbiProject.cx = MulDiv(100, dpi, 96);
    m_rebar.InsertBand(-1, &rbbiProject);

    // --- 7. 添加左侧填充控件，让项目筛选靠右 ---
    m_spacer.Create(m_rebar, rcDefault, NULL, WS_CHILD | WS_VISIBLE);
    REBARBANDINFO rbbiSpacer = { sizeof(REBARBANDINFO) };
    rbbiSpacer.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE;
    rbbiSpacer.fStyle = RBBS_FIXEDBMP | RBBS_NOGRIPPER;  // 移除 RBBS_CHILDEDGE
    rbbiSpacer.hwndChild = m_spacer;
    rbbiSpacer.cx = 0;  // 填充剩余空间
    m_rebar.InsertBand(-1, &rbbiSpacer);

    m_mainSplitter.Create(m_hWnd, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_todoList.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,  // 必须添加虚拟列表样式
        WS_EX_CLIENTEDGE);
    m_todoList.SetFont(m_fontList);
    m_todoList.SetImageList(m_imgList, LVSIL_SMALL);
    m_todoList.SetDataManager(&m_dataManager);
    m_todoList.SetIsDoneList(false);

    m_doneList.Create(m_mainSplitter, rcDefault, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,  // 必须添加虚拟列表样式
        WS_EX_CLIENTEDGE);
    m_doneList.SetFont(m_fontList);
    m_doneList.SetImageList(m_imgList, LVSIL_SMALL);
    m_doneList.SetDataManager(&m_dataManager);
    m_doneList.SetIsDoneList(true);

    m_detailPanel.Create(m_hWnd, rcDefault, NULL,
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER,
        WS_EX_CLIENTEDGE);

    // 子类化详情面板窗口以转发按钮消息
    if (m_detailPanel.IsWindow()) {
        m_originalDetailPanelWndProc = (WNDPROC)::SetWindowLongPtr(
            m_detailPanel.m_hWnd, GWLP_WNDPROC, (LONG_PTR)DetailPanelWndProc);
        // 保存原始窗口过程到 GWLP_USERDATA 供 DetailPanelWndProc 使用
        ::SetWindowLongPtr(m_detailPanel.m_hWnd, GWLP_USERDATA, (LONG_PTR)m_originalDetailPanelWndProc);
    }

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
    m_statusBar.SetText(0, GetString(StringID::StatusReady), 0);

    SetupLists();

    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        BOOL bLoaded = dbManager.LoadAll(m_dataManager);
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("LoadAll result=%d, todoCount=%zu, doneCount=%zu\n"),
            bLoaded, m_dataManager.todoItems.size(), m_dataManager.doneItems.size());
        DEBUG_OUTPUT(szDebug);

        if (m_dataManager.todoItems.empty() && m_dataManager.doneItems.empty()) {
            DEBUG_OUTPUT(_T("生成默认测试数据...\n"));

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

            DEBUG_OUTPUT(_T("已生成12个待办 + 3个已完成 测试数据\n"));

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

    // 加载语言设置
    LoadLanguageSetting();
    ApplyLanguage();

    SetTimer(IDT_FORCE_REFRESH, 200, nullptr);
    PostMessage(WM_SIZE);

    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
{
    SaveWindowSettings();
    ::KillTimer(m_hWnd, IDT_FORCE_REFRESH);
    ::KillTimer(m_hWnd, IDT_STATUS_CLEAR);
    ::KillTimer(m_hWnd, IDT_SEARCH_DEBOUNCE);

    CSQLiteManager dbManager;
    if (dbManager.Initialize()) {
        dbManager.SaveAll(m_dataManager);
    }

    // 必须：恢复原始窗口过程，防止退出崩溃
    // 注：只恢复实际被子类化的窗口（目前只有 m_detailPanel）
    if (m_detailPanel.IsWindow() && m_originalDetailPanelWndProc) {
        ::SetWindowLongPtr(m_detailPanel.m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_originalDetailPanelWndProc);
    }
    if (m_searchContainer.IsWindow() && m_originalSearchContainerWndProc) {
        ::SetWindowLongPtr(m_searchContainer.m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_originalSearchContainerWndProc);
    }

    // 发送退出消息，结束消息循环
    ::PostQuitMessage(0);
    return 0;
}

LRESULT CMainFrame::OnAppRefresh(UINT, WPARAM, LPARAM, BOOL&)
{
    DEBUG_OUTPUT(_T("OnAppRefresh: Refreshing lists...\n"));

    TCHAR szDebug[512];
    if (m_todoList.IsWindow()) {
        DWORD style = m_todoList.GetStyle();
        DWORD exStyle = m_todoList.GetExStyle();
        _stprintf_s(szDebug, _T("  m_todoList: HWND=0x%08X, style=0x%08X, exStyle=0x%08X\n"),
            (UINT_PTR)m_todoList.m_hWnd, style, exStyle);
        DEBUG_OUTPUT(szDebug);

        BOOL hasOwnerData = (style & LVS_OWNERDATA) != 0;
        _stprintf_s(szDebug, _T("  LVS_OWNERDATA=%d\n"), hasOwnerData);
        DEBUG_OUTPUT(szDebug);

        int groupCount = ListView_GetGroupCount(m_todoList);
        _stprintf_s(szDebug, _T("  groupCount=%d\n"), groupCount);
        DEBUG_OUTPUT(szDebug);

        int itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  GetItemCount=%d\n"), itemCount);
        DEBUG_OUTPUT(szDebug);

        m_todoList.SetItemCountEx(m_dataManager.GetItemCount(false), LVSICF_NOSCROLL);
        itemCount = m_todoList.GetItemCount();
        _stprintf_s(szDebug, _T("  After SetItemCountEx: GetItemCount=%d\n"), itemCount);
        DEBUG_OUTPUT(szDebug);
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
        RECT rcRebarWin, rcRebarClient;
        m_rebar.GetWindowRect(&rcRebarWin);
        m_rebar.GetClientRect(&rcRebarClient);
        toolbarHeight = rcRebarClient.bottom - rcRebarClient.top;

        m_rebar.MoveWindow(0, 0, rcClient.right, toolbarHeight);

#ifdef _DEBUG
        RECT rcSplitterWin;
        m_mainSplitter.GetWindowRect(&rcSplitterWin);
        RECT rcSplitterClient;
        m_mainSplitter.GetClientRect(&rcSplitterClient);

        TCHAR szDebug[512];
        _stprintf_s(szDebug, _T("[OnSize] ReBar WinRect=(%d,%d,%d,%d) ClientRect.height=%d, Splitter WinRect.top=%d, Client.top=%d\n"),
            rcRebarWin.left, rcRebarWin.top, rcRebarWin.right, rcRebarWin.bottom,
            toolbarHeight,
            rcSplitterWin.top, rcSplitterClient.top);
        DEBUG_OUTPUT(szDebug);
#endif
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

        // 调整 TodoList 列宽
        AdjustTodoListColumnWidths(clientWidth);
    }

    if (m_statusBar.IsWindow()) {
        m_statusBar.MoveWindow(&rcClient);
    }

    // Update popup position when main window is resized
    if (m_bDetailVisible) {
        ShowDetailPopup();
        // 更新面板内控件位置
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

        m_detailProject.MoveWindow(x, y, width, lineHeight);
        y += lineHeight + gapSmall;

        // 保留按钮区域高度
        int btnAreaHeight = 30;
        int noteHeight = rcPanel.bottom - rcPanel.top - y - btnAreaHeight - 10;
        if (noteHeight < lineHeight) noteHeight = lineHeight;
        m_detailNote.MoveWindow(x, y, width, noteHeight);

        // 按钮区域
        int btnY = rcPanel.bottom - btnAreaHeight - 5;
        int btnHeight = 24;
        int btnWidth = 60;
        int btnGap = 5;

        // 底部按钮区域（2个按钮：关闭和固定）
        int bottomBtnX = rcPanel.right - (btnWidth + btnGap) * 2 - 5;

        // 关闭按钮
        m_btnClose.MoveWindow(bottomBtnX, btnY, btnWidth, btnHeight);
        bottomBtnX += btnWidth + btnGap;

        // 固定/取消按钮
        m_btnKeep.MoveWindow(bottomBtnX, btnY, btnWidth, btnHeight);
    }

    return 0;
}

LRESULT CMainFrame::OnMove(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    bHandled = TRUE;
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
                SendMessage(WM_COMMAND, MAKEWPARAM(ID_TODO_CONTEXT_MENU, bIsDone ? 1 : 0), (LPARAM)index);
                bHandled = TRUE;
                return 0;
            }
            else if (pnmh->code == NM_DBLCLK) {
                SendMessage(WM_COMMAND, MAKEWPARAM(ID_TODO_EDIT, bIsDone ? 1 : 0), (LPARAM)index);
                bHandled = TRUE;
                return 0;
            }
        } else {
            // 只有在未固定时才关闭面板
            if (!m_bDetailPinned) {
                HideDetailPopup();
            }
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
    DEBUG_OUTPUT(szDebug);
    DebugCommandSource(_T("OnCommand"), wParam, lParam, m_hWnd);

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
        if (HIWORD(wParam) != 0 || lParam != 0) {
            int index = (int)lParam;
            bool isDoneList = HIWORD(wParam) != 0;
            return OnTodoDelete(0, 0, MAKELPARAM(index, isDoneList ? 1 : 0), bHandled);
        }
        if (m_nSelectedIndex >= 0) {
            return OnTodoDelete(0, 0, MAKELPARAM(m_nSelectedIndex, m_bSelectedIsDone ? 1 : 0), bHandled);
        }
        break;
    case ID_TODO_COMPLETE:
    case ID_TODO_EDIT:
    case ID_TODO_CONTEXT_MENU:
        {
            int index = (int)lParam;
            bool isDoneList = HIWORD(wParam) != 0;
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
        DEBUG_OUTPUT(_T("[OnCommand] 路由到 OnToggleTopmost\n"));
        return OnToggleTopmost(0, 0, NULL, bHandled);
    case ID_TIME_FILTER:
        DEBUG_OUTPUT(_T("[OnCommand] 路由到 OnToggleTimeFilter\n"));
        return OnToggleTimeFilter(0, 0, NULL, bHandled);
    case ID_PROJECT_FILTER:
        return OnProjectFilterChanged(0, 0, NULL, bHandled);
    // 详情面板按钮
    case IDC_CLOSE_BUTTON:
        HideDetailPopup();
        return 0;
    case IDC_KEEP_BUTTON:
        // 切换固定状态
        m_bDetailPinned = !m_bDetailPinned;
        if (m_bDetailPinned) {
            m_btnKeep.SetWindowText(GetString(StringID::BtnUnpin));
        } else {
            m_btnKeep.SetWindowText(GetString(StringID::BtnPin));
        }
        return 0;
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
        bHandled = FALSE;
        return 0;
    }

    bHandled = TRUE;
    return 0;
}

void CMainFrame::SetupLists()
{
    // 获取 DPI 并计算列宽（基准：75, 50, 380, 120 @ 96 DPI）
    int dpi = 96;
    HDC hdc = ::GetDC(NULL);
    if (hdc) {
        dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
        ::ReleaseDC(NULL, hdc);
    }

    m_todoList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_todoList.InsertColumn(0, GetString(StringID::ColCreateDate), LVCFMT_CENTER, MulDiv(82, dpi, 96));
    m_todoList.InsertColumn(1, GetString(StringID::ColPriority), LVCFMT_CENTER, MulDiv(50, dpi, 96));
    m_todoList.InsertColumn(2, GetString(StringID::ColDescription), LVCFMT_LEFT, MulDiv(250, dpi, 96));
    m_todoList.InsertColumn(3, GetString(StringID::ColDeadline), LVCFMT_CENTER, MulDiv(120, dpi, 96));

    m_doneList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER |
        LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);

    m_doneList.InsertColumn(0, GetString(StringID::ColPriority), LVCFMT_CENTER, MulDiv(50, dpi, 96));
    m_doneList.InsertColumn(1, GetString(StringID::ColDescription), LVCFMT_LEFT, MulDiv(380, dpi, 96));
    m_doneList.InsertColumn(2, GetString(StringID::ColDoneTime), LVCFMT_CENTER, MulDiv(120, dpi, 96));
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
    DEBUG_OUTPUT(szDebug);

    // 记录筛选条件
    LPCTSTR pszTimeFilter = nullptr;
    switch (m_timeFilter) {
    case TimeFilter::Today: pszTimeFilter = GetString(StringID::FilterToday); break;
    case TimeFilter::ThisWeek: pszTimeFilter = GetString(StringID::FilterThisWeek); break;
    default: pszTimeFilter = GetString(StringID::FilterAll); break;
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
    DEBUG_OUTPUT(szDebug);

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
    DEBUG_OUTPUT(szDebug);

    // 初始设置列宽
    RECT rcClient;
    GetClientRect(&rcClient);
    AdjustTodoListColumnWidths(rcClient.right - rcClient.left);
}

void CMainFrame::AdjustTodoListColumnWidths(int cx)
{
    if (!m_todoList.m_hWnd) return;

    int dpi = 96;
    HDC hdc = ::GetDC(NULL);
    if (hdc) {
        dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
        ::ReleaseDC(NULL, hdc);
    }

    int colCreateDate = MulDiv(82, dpi, 96);
    int colPriority = MulDiv(55, dpi, 96);
    int colDeadline = MulDiv(115, dpi, 96);

    // 计算任务描述列宽度（剩余空间）
    // 减去垂直滚动条宽度和边框，避免出现水平滚动条
    int scrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL);
    int borderPadding = 4;  // ListView 边框和内边距
    int availableWidth = cx - scrollBarWidth - borderPadding;
    int colDescription = availableWidth - colCreateDate - colPriority - colDeadline;
    if (colDescription < 50) colDescription = 50;  // 最小宽度保护

    m_todoList.SetColumnWidth(0, colCreateDate);
    m_todoList.SetColumnWidth(1, colPriority);
    m_todoList.SetColumnWidth(2, colDescription);
    m_todoList.SetColumnWidth(3, colDeadline);

    // Done list 列宽调整（优先级、任务描述、完成时间）
    if (m_doneList.m_hWnd) {
        int doneColPriority = MulDiv(55, dpi, 96);
        int doneColDoneTime = MulDiv(115, dpi, 96);
        int doneColDescription = availableWidth - doneColPriority - doneColDoneTime;
        if (doneColDescription < 50) doneColDescription = 50;

        m_doneList.SetColumnWidth(0, doneColPriority);
        m_doneList.SetColumnWidth(1, doneColDescription);
        m_doneList.SetColumnWidth(2, doneColDoneTime);
    }
}

void CMainFrame::CreateDetailPanelControls()
{
    // 使用与列表一致的字体（m_fontList）
    HFONT hNormalFont = m_fontList;

    m_detailEmpty.Create(m_detailPanel, rcDefault, GetString(StringID::ClickToViewDetail),
        WS_CHILD | ES_CENTER | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailEmpty.SetFont(hNormalFont);

    m_detailPriority.Create(m_detailPanel, rcDefault, GetString(StringID::DetailPriority),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailPriority.SetFont(hNormalFont);

    m_detailDescription.Create(m_detailPanel, rcDefault, GetString(StringID::DetailDescription),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailDescription.SetFont(hNormalFont);

    m_detailCreateTime.Create(m_detailPanel, rcDefault, GetString(StringID::DetailCreateTime),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailCreateTime.SetFont(hNormalFont);

    m_detailEndTime.Create(m_detailPanel, rcDefault, GetString(StringID::DetailDeadline),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailEndTime.SetFont(hNormalFont);

    m_detailProject.Create(m_detailPanel, rcDefault, GetString(StringID::DetailProject),
        WS_CHILD | ES_LEFT | ES_READONLY | ES_AUTOHSCROLL,
        WS_EX_CLIENTEDGE);
    m_detailProject.SetFont(hNormalFont);

    m_detailNote.Create(m_detailPanel, rcDefault, GetString(StringID::DetailNote),
        WS_CHILD | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        WS_EX_CLIENTEDGE);
    m_detailNote.SetFont(hNormalFont);

    // 创建关闭按钮（右下角）
    m_btnClose.Create(m_detailPanel, rcDefault, GetString(StringID::Close),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, IDC_CLOSE_BUTTON);
    m_btnClose.SetFont(hNormalFont);

    // 创建固定/取消按钮（右下角）
    m_btnKeep.Create(m_detailPanel, rcDefault, GetString(StringID::BtnPin),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, IDC_KEEP_BUTTON);
    m_btnKeep.SetFont(hNormalFont);
}

void CMainFrame::UpdateDetailPanel(int index, bool isDoneList)
{
    if (index < 0) {
        m_detailEmpty.ShowWindow(SW_SHOW);
        m_detailPriority.ShowWindow(SW_HIDE);
        m_detailDescription.ShowWindow(SW_HIDE);
        m_detailCreateTime.ShowWindow(SW_HIDE);
        m_detailEndTime.ShowWindow(SW_HIDE);
        m_detailProject.ShowWindow(SW_HIDE);
        m_detailNote.ShowWindow(SW_HIDE);

        // 隐藏按钮
        m_btnClose.ShowWindow(SW_HIDE);
        m_btnKeep.ShowWindow(SW_HIDE);
        return;
    }

    const TodoItem* pItem = GetItemByDisplayIndex(index, isDoneList);
    if (!pItem) {
        return;
    }

    m_detailEmpty.ShowWindow(SW_HIDE);
    m_detailPriority.ShowWindow(SW_SHOW);
    m_detailDescription.ShowWindow(SW_SHOW);
    m_detailCreateTime.ShowWindow(SW_SHOW);
    m_detailEndTime.ShowWindow(SW_SHOW);
    m_detailProject.ShowWindow(SW_SHOW);
    m_detailNote.ShowWindow(SW_SHOW);

    // 显示按钮
    m_btnClose.ShowWindow(SW_SHOW);
    m_btnKeep.ShowWindow(SW_SHOW);

    CString strText;

    strText.Format(_T("%s%s"), GetString(StringID::DetailPriority), pItem->GetPriorityString());
    m_detailPriority.SetWindowText(strText);

    strText.Format(_T("%s%s"), GetString(StringID::DetailDescription), pItem->title.c_str());
    m_detailDescription.SetWindowText(strText);

    strText.Format(_T("%s%s"), GetString(StringID::DetailCreateTime), pItem->GetCreateTimeString());
    m_detailCreateTime.SetWindowText(strText);

    if (pItem->targetEndTime.GetTime() > 0) {
        strText.Format(_T("%s%s"), GetString(StringID::DetailDeadline), pItem->GetEndTimeString());
    } else {
        strText = GetString(StringID::DetailDeadlineNone);
    }
    m_detailEndTime.SetWindowText(strText);

    strText.Format(_T("%s%s"), GetString(StringID::DetailProject),
        pItem->project.empty() ? GetString(StringID::DetailNone) : pItem->project.c_str());
    m_detailProject.SetWindowText(strText);

    strText.Format(_T("%s%s"), GetString(StringID::DetailNote),
        pItem->note.empty() ? GetString(StringID::DetailNone) : pItem->note.c_str());
    m_detailNote.SetWindowText(strText);

    RECT rcPanel;
    m_detailPanel.GetClientRect(&rcPanel);

    int x = 10;
    int y = 10;
    int width = rcPanel.right - rcPanel.left - 20;

    // 获取字体高度以匹配列表控件
    TEXTMETRIC tm;
    HDC hdc = ::GetDC(m_detailPanel.m_hWnd);
    HFONT hOldFont = (HFONT)::SelectObject(hdc, m_fontList);
    ::GetTextMetrics(hdc, &tm);
    ::SelectObject(hdc, hOldFont);
    ::ReleaseDC(m_detailPanel.m_hWnd, hdc);
    int lineHeight = tm.tmHeight + tm.tmExternalLeading;
    if (lineHeight < 22) lineHeight = 22;  // 最小行高 22px

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

    m_detailProject.MoveWindow(x, y, width, lineHeight);
    y += lineHeight + gapSmall;

    // 保留按钮区域高度
    int btnAreaHeight = 30;
    int noteHeight = rcPanel.bottom - rcPanel.top - y - btnAreaHeight - 10;
    if (noteHeight < lineHeight) noteHeight = lineHeight;
    m_detailNote.MoveWindow(x, y, width, noteHeight);

    // 按钮区域
    int btnY = rcPanel.bottom - btnAreaHeight - 5;
    int btnHeight = 24;
    int btnWidth = 60;
    int btnGap = 5;

    // 底部按钮区域（2个按钮：关闭和固定）
    int bottomBtnX = rcPanel.right - (btnWidth + btnGap) * 2 - 5;

    // 关闭按钮
    m_btnClose.MoveWindow(bottomBtnX, btnY, btnWidth, btnHeight);
    bottomBtnX += btnWidth + btnGap;

    // 固定/取消按钮
    m_btnKeep.MoveWindow(bottomBtnX, btnY, btnWidth, btnHeight);

    // 更新固定按钮文字
    if (m_bDetailPinned) {
        m_btnKeep.SetWindowText(GetString(StringID::BtnUnpin));
    } else {
        m_btnKeep.SetWindowText(GetString(StringID::BtnPin));
    }

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
    DEBUG_OUTPUT(_T("OnTodoAdd called\n"));
    if (m_bDialogOpen) {
        DEBUG_OUTPUT(_T("[OnTodoAdd] Dialog already open, ignore\n"));
        return 0;
    }
    ULONGLONG t0 = GetTickCount64();
    m_bDialogOpen = true;
    DEBUG_OUTPUT(_T("[OnTodoAdd] BEGIN\n"));

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
    dlg.SetInvokeTick(t0);

    DEBUG_OUTPUT(_T("[OnTodoAdd] Before DoModal\n"));
    INT_PTR nRet = dlg.DoModal();
    DEBUG_OUTPUT(_T("[OnTodoAdd] After DoModal\n"));
    m_bDialogOpen = false;
    TCHAR szTime[128];
    _stprintf_s(szTime, _T("[OnTodoAdd] DoModal Δ=%llums\n"), (unsigned long long)(GetTickCount64() - t0));
    DEBUG_OUTPUT(szTime);

    if (nRet == IDOK) {
        DEBUG_OUTPUT(_T("Dialog returned IDOK\n"));
        TodoItem item = dlg.GetResult();

        TCHAR szDebug[512];
        _stprintf_s(szDebug, _T("Adding todo: title='%s', priority=%d\n"),
            item.title.c_str(), (int)item.priority);
        DEBUG_OUTPUT(szDebug);

        m_dataManager.AddTodo(item);

        _stprintf_s(szDebug, _T("After AddTodo: todoCount=%d\n"),
            m_dataManager.GetItemCount(false));
        DEBUG_OUTPUT(szDebug);

        CSQLiteManager dbManager;
        if (dbManager.Initialize()) {
            dbManager.SaveTodo(item);
            DEBUG_OUTPUT(_T("Saved to database\n"));
        }

        UpdateLists();
        UpdateProjectFilterList();

        CString strMsg;
        CString strTitle(item.title.c_str());
        strMsg.Format(_T("任务 \"%s\" 已添加"), (LPCTSTR)strTitle);
        m_statusBar.SetText(0, (LPCTSTR)strMsg, 0);

        ::SetTimer(m_hWnd, IDT_STATUS_CLEAR, 3000, nullptr);
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
    CString aboutText;
    aboutText.Format(
        _T("Simple Todo %s\n\n")
        _T("基于 C++/WTL 的极简任务管理器\n\n")
        _T("特性：\n")
        _T("- Virtual List-View 高性能显示\n")
        _T("- 按日期分组，支持折叠/展开\n")
        _T("- 优先级颜色标识\n")
        _T("- SQLite 数据持久化\n")
        _T("- 支持 todo.txt 格式的导出\n")
        _T("- 支持 csv 格式导出\n\n")
        _T("作者：wuyueyu-五月雨\n")
        _T("QQ/WX：778137\n")
        _T("Twitter：https://x.com/wuyueyuCN\n")
        _T("Github：https://github.com/purezhang/simple-todo"),
        APP_VERSION_FULL);
    
    ::MessageBox(m_hWnd, aboutText, _T("关于 Simple Todo"), MB_OK | MB_ICONINFORMATION);
    return 0;
}

LRESULT CMainFrame::OnTodoComplete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    int index = LOWORD(lParam);
    bool isDoneList = HIWORD(lParam) != 0;

    if (index >= 0 && !isDoneList) {
        UINT id = GetItemIdByDisplayIndex(index, false);
        if (id > 0 && m_dataManager.CompleteTodo(id)) {
            CSQLiteManager dbManager;
            if (dbManager.Initialize()) {
                dbManager.MoveTodo(id, true);
            }
            UpdateLists();
            ReselectById(id, true);
            m_doneList.SetFocus();
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
        UINT id = GetItemIdByDisplayIndex(index, isDoneList);
        if (id > 0 && m_dataManager.DeleteTodo(id, isDoneList)) {
            CSQLiteManager dbManager;
            if (dbManager.Initialize()) {
                dbManager.DeleteTodo(id);
            }
            UpdateLists();
            m_todoList.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            m_doneList.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            HideDetailPopup();
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
        const TodoItem* pItem = GetItemByDisplayIndex(index, isDoneList);
        if (pItem) {
            if (m_bDialogOpen) {
                DEBUG_OUTPUT(_T("[OnTodoEdit] Dialog already open, ignore\n"));
                bHandled = TRUE;
                return 0;
            }
            ULONGLONG t0 = GetTickCount64();
            m_bDialogOpen = true;
            DEBUG_OUTPUT(_T("[OnTodoEdit] Before DoModal\n"));
            CAddTodoDlg dlg(*pItem);

            // 传递项目列表（与 OnTodoAdd 保持一致）
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
            std::vector<std::wstring> projects(projectSet.begin(), projectSet.end());
            dlg.SetProjects(projects);

            dlg.SetInvokeTick(t0);
            INT_PTR nRet = dlg.DoModal();
            DEBUG_OUTPUT(_T("[OnTodoEdit] After DoModal\n"));
            m_bDialogOpen = false;
            TCHAR szTime[128];
            _stprintf_s(szTime, _T("[OnTodoEdit] DoModal Δ=%llums\n"), (unsigned long long)(GetTickCount64() - t0));
            DEBUG_OUTPUT(szTime);

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
                    ReselectById(updatedItem.id, isDoneList);
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
        UINT id = GetItemIdByDisplayIndex(m_nSelectedIndex, false);
        if (id > 0 && m_dataManager.CompleteTodo(id)) {
            CSQLiteManager dbManager;
            if (dbManager.Initialize()) {
                dbManager.MoveTodo(id, true);
            }
            UpdateLists();
            ReselectById(id, true);
        }
    }
    return 0;
}

LRESULT CMainFrame::OnContextPin(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0 && !m_bSelectedIsDone) {
        const TodoItem* pItem = GetItemByDisplayIndex(m_nSelectedIndex, false);
        if (pItem) {
            TodoItem item = *pItem;
            item.isPinned = !item.isPinned;
            
            if (m_dataManager.UpdateTodo(item, false)) {
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.UpdateTodo(item);
                }
                UpdateLists();
                ReselectById(item.id, false);
            }
        }
    }
    return 0;
}

LRESULT CMainFrame::OnContextCopyText(WORD, WORD, HWND, BOOL&)
{
    if (m_nSelectedIndex >= 0) {
        const TodoItem* pItem = GetItemByDisplayIndex(m_nSelectedIndex, m_bSelectedIsDone);
        if (pItem) {
            CString strText;
            CString strTitle(pItem->title.c_str());
            strText.Format(_T("[%s] %s"),
                (LPCTSTR)pItem->GetPriorityString(),
                (LPCTSTR)strTitle);

            if (OpenClipboard()) {
                EmptyClipboard();
                SIZE_T nSize = (strText.GetLength() + 1) * sizeof(TCHAR);
                HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, nSize);
                if (hglb) {
                    void* p = GlobalLock(hglb);
                    if (p) {
                        memcpy(p, strText.GetString(), nSize);
                        GlobalUnlock(hglb);
                        if (!::SetClipboardData(CF_UNICODETEXT, hglb)) {
                            ::GlobalFree(hglb);
                        }
                    } else {
                        ::GlobalFree(hglb);
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
        menu.AppendMenu(MF_STRING, ID_CONTEXT_MARK_DONE, (LPCTSTR)GetString(StringID::MarkAsDone));
        menu.AppendMenu(MF_STRING, ID_CONTEXT_EDIT, (LPCTSTR)GetString(StringID::Edit));

        const TodoItem* pItem = GetItemByDisplayIndex(index, isDoneList);
        if (pItem) {
            if (pItem->isPinned) {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, (LPCTSTR)GetString(StringID::Unpin));
            } else {
                menu.AppendMenu(MF_STRING, ID_CONTEXT_PIN, (LPCTSTR)GetString(StringID::Pin));
            }
        }
    }
    menu.AppendMenu(MF_STRING, ID_CONTEXT_COPY_TEXT, (LPCTSTR)GetString(StringID::CopyText));

    CMenu menuPriority;
    menuPriority.CreatePopupMenu();
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P0, (LPCTSTR)GetString(StringID::PriorityP0));
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P1, (LPCTSTR)GetString(StringID::PriorityP1));
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P2, (LPCTSTR)GetString(StringID::PriorityP2));
    menuPriority.AppendMenu(MF_STRING, ID_CONTEXT_PRIORITY_P3, (LPCTSTR)GetString(StringID::PriorityP3));

    menu.AppendMenu(MF_POPUP, (UINT_PTR)menuPriority.m_hMenu, (LPCTSTR)GetString(StringID::SetPriority));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_TODO_DELETE, (LPCTSTR)GetString(StringID::Delete));

    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        pt.x, pt.y, m_hWnd);
}

LRESULT CMainFrame::ChangePriority(Priority newPriority)
{
    if (m_nSelectedIndex >= 0) {
        const TodoItem* pItem = GetItemByDisplayIndex(m_nSelectedIndex, m_bSelectedIsDone);
        if (pItem) {
            UINT id = pItem->id;
            // 先拷贝数据，避免 ChangePriority 导致 vector 重排后 pItem 失效
            TodoItem updatedItem = *pItem;
            updatedItem.priority = newPriority;
            if (m_dataManager.ChangePriority(id, newPriority, m_bSelectedIsDone)) {
                CSQLiteManager dbManager;
                if (dbManager.Initialize()) {
                    dbManager.UpdateTodo(updatedItem);
                }
                UpdateLists();
                ReselectById(id, m_bSelectedIsDone);
            }
        }
    }
    return 0;
}

std::wstring CMainFrame::EscapeCSV(const std::wstring& s)
{
    bool needQuote = s.find(L',') != std::wstring::npos ||
                     s.find(L'"') != std::wstring::npos ||
                     s.find(L'\n') != std::wstring::npos;

    std::wstring out = s;
    size_t pos = 0;
    while ((pos = out.find(L'"', pos)) != std::wstring::npos) {
        out.insert(pos, 1, L'"');
        pos += 2;
    }
    if (needQuote) {
        out = L"\"" + out + L"\"";
    }
    return out;
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
                std::wstring strTitle = EscapeCSV(item.title);
                std::wstring strProject = EscapeCSV(item.project);
                _ftprintf(fp, _T("%s,%s,%s,%s,%s\n"),
                    item.GetPriorityString(),
                    strTitle.c_str(),
                    strProject.c_str(),
                    item.GetCreateTimeString(),
                    item.GetEndTimeString());
            }

            _ftprintf(fp, _T("\nDone 列表\n"));
            _ftprintf(fp, _T("优先级,描述,分类,完成时间\n"));
            for (const auto& item : m_dataManager.doneItems) {
                std::wstring strTitle = EscapeCSV(item.title);
                std::wstring strProject = EscapeCSV(item.project);
                _ftprintf(fp, _T("%s,%s,%s,%s\n"),
                    item.GetPriorityString(),
                    strTitle.c_str(),
                    strProject.c_str(),
                    item.GetDoneTimeString());
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

            fclose(fp);
            MessageBox(_T("todo.txt 导出成功！\n\n格式说明:\n(A) 紧急任务\n(B) 重要任务\n(C) 普通任务\n(D) 暂缓任务\nx 已完成任务\ndue: 截止时间"),
                _T("导出成功"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

LRESULT CMainFrame::OnToggleTopmost(WORD, WORD, HWND, BOOL&)
{
    DEBUG_OUTPUT(_T("[OnToggleTopmost] 开始处理置顶\n"));

    m_bTopmost = !m_bTopmost;

    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("[OnToggleTopmost] m_bTopmost=%d\n"), m_bTopmost);
    DEBUG_OUTPUT(szDebug);

    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (LPTSTR)(m_bTopmost ? GetString(StringID::TbTopmostOn) : GetString(StringID::TbTopmost));
    m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbbi);

    DEBUG_OUTPUT(_T("[OnToggleTopmost] 调用 SetWindowPos\n"));
    ::SetWindowPos(m_hWnd, m_bTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    DEBUG_OUTPUT(_T("[OnToggleTopmost] 处理完成\n"));
    return 0;
}

LRESULT CMainFrame::OnToggleTimeFilter(WORD, WORD, HWND, BOOL&)
{
    // 关键日志：记录切换前的状态
    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] BEFORE: m_timeFilter=%d\n"), (int)m_timeFilter);
    DEBUG_OUTPUT(szDebug);

    // 轮询切换: 全部 -> 今天 -> 本周 -> 全部
    LPCTSTR pszNewFilter = nullptr;
    switch (m_timeFilter) {
    case TimeFilter::All:
        m_timeFilter = TimeFilter::Today;
        pszNewFilter = GetString(StringID::TbFilterToday);
        break;
    case TimeFilter::Today:
        m_timeFilter = TimeFilter::ThisWeek;
        pszNewFilter = GetString(StringID::TbFilterWeek);
        break;
    case TimeFilter::ThisWeek:
    default:
        m_timeFilter = TimeFilter::All;
        pszNewFilter = GetString(StringID::TbFilter);
        break;
    }

    // 关键日志：记录切换后的状态
    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] AFTER: m_timeFilter=%d, pszNewFilter=%s\n"), (int)m_timeFilter, pszNewFilter);
    DEBUG_OUTPUT(szDebug);

    // 更新按钮文字
    TBBUTTONINFO tbbi = { sizeof(TBBUTTONINFO) };
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (LPTSTR)pszNewFilter;
    m_toolbar.SetButtonInfo(ID_TIME_FILTER, &tbbi);

    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] CALL UpdateLists\n"));
    DEBUG_OUTPUT(szDebug);

    // 刷新列表（时间筛选结果会在 UpdateLists 中输出）
    UpdateLists();

    _stprintf_s(szDebug, _T("[OnToggleTimeFilter] END\n"));
    DEBUG_OUTPUT(szDebug);
    return 0;
}

void CMainFrame::OnSearchChanged()
{
    // 重置搜索定时器（防抖 500ms）
    ::KillTimer(m_hWnd, IDT_SEARCH_DEBOUNCE);
    ::SetTimer(m_hWnd, IDT_SEARCH_DEBOUNCE, 500, nullptr);
}

LRESULT CMainFrame::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    if (wParam == IDT_STATUS_CLEAR) {
        m_statusBar.SetText(0, _T(""), 0);
        ::KillTimer(m_hWnd, IDT_STATUS_CLEAR);
    } else if (wParam == IDT_FORCE_REFRESH) {
        DEBUG_OUTPUT(_T("Timer IDT_FORCE_REFRESH: Force refresh\n"));
        ::KillTimer(m_hWnd, IDT_FORCE_REFRESH);
        UpdateLists();
    } else if (wParam == IDT_SEARCH_DEBOUNCE) {
        // 搜索定时器触发
        ::KillTimer(m_hWnd, IDT_SEARCH_DEBOUNCE);

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

// ComboBox 下拉列表背景颜色处理（解决 ReBar 中 ComboBox 下拉列表显示问题）
LRESULT CMainFrame::OnCtlColorListBox(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    // 返回系统白色背景刷，让系统主题正确绘制
    return (LRESULT)::GetStockObject(WHITE_BRUSH);
}

LRESULT CMainFrame::OnLanguageChinese(WORD, WORD, HWND, BOOL&)
{
    if (!m_bChineseLanguage) {
        m_bChineseLanguage = true;
        g_bChineseLanguage = true;
        SaveLanguageSetting();
        ApplyLanguage();
    }
    return 0;
}

LRESULT CMainFrame::OnLanguageEnglish(WORD, WORD, HWND, BOOL&)
{
    if (m_bChineseLanguage) {
        m_bChineseLanguage = false;
        g_bChineseLanguage = false;
        SaveLanguageSetting();
        ApplyLanguage();
    }
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
    m_projectFilter.AddString(GetString(StringID::ProjectAll));

    for (const auto& proj : projects) {
        m_projectFilter.AddString(proj.c_str());
    }

    // 恢复选中状态或默认选中"全部"
    CString allText = GetString(StringID::ProjectAll);
    if (!currentText.IsEmpty()) {
        int found = m_projectFilter.FindStringExact(-1, currentText);
        if (found >= 0) {
            m_projectFilter.SetCurSel(found);
        } else {
            m_projectFilter.SetCurSel(0);  // 选中"全部"
        }
    } else {
        m_projectFilter.SetCurSel(0);  // 默认选中"全部"
    }
}

LRESULT CMainFrame::OnProjectFilterChanged(WORD, WORD, HWND, BOOL&)
{
    CString selText;
    m_projectFilter.GetWindowText(selText);
    selText.Trim();

    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("=== OnProjectFilterChanged START === selText='%s'\n"), (LPCTSTR)selText);
    DEBUG_OUTPUT(szDebug);

    CString allText = GetString(StringID::ProjectAll);
    if (selText.IsEmpty() || selText == allText) {
        m_currentProjectFilter.clear();
        _stprintf_s(szDebug, _T("OnProjectFilterChanged: 选中[全部], filter='%s'\n"),
            m_currentProjectFilter.c_str());
    } else {
        m_currentProjectFilter = selText.GetString();
        _stprintf_s(szDebug, _T("OnProjectFilterChanged: 选中项目='%s'\n"),
            m_currentProjectFilter.c_str());
    }
    DEBUG_OUTPUT(szDebug);

    // 刷新列表显示
    DEBUG_OUTPUT(_T("OnProjectFilterChanged: 调用 UpdateLists()\n"));
    UpdateLists();

    _stprintf_s(szDebug, _T("=== OnProjectFilterChanged END === filter='%s'\n"),
        m_currentProjectFilter.c_str());
    DEBUG_OUTPUT(szDebug);
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

    // 应用分割条位置
    if (m_nSplitterPos > 0 && m_mainSplitter.IsWindow()) {
        m_mainSplitter.SetSplitterPos(m_nSplitterPos);
        m_bFirstSize = false;  // 防止 OnSize 中再次覆盖
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

void CMainFrame::LoadLanguageSetting()
{
    HKEY hKey;
    TCHAR szValue[32] = {0};

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(szValue);
        DWORD dwType = REG_SZ;

        if (RegQueryValueEx(hKey, REG_KEY_LANGUAGE, NULL, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            m_bChineseLanguage = (_tcscmp(szValue, _T("English")) != 0);
        } else {
            m_bChineseLanguage = true;
        }
        RegCloseKey(hKey);
    } else {
        m_bChineseLanguage = true;
    }

    g_bChineseLanguage = m_bChineseLanguage;
}

void CMainFrame::SaveLanguageSetting()
{
    HKEY hKey;
    DWORD dwDisposition;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        LPCTSTR pLanguage = m_bChineseLanguage ? _T("Chinese") : _T("English");
        RegSetValueEx(hKey, REG_KEY_LANGUAGE, 0, REG_SZ,
            (LPBYTE)pLanguage, (DWORD)(_tcslen(pLanguage) + 1) * sizeof(TCHAR));
        RegCloseKey(hKey);
    }
}

void CMainFrame::ApplyLanguage()
{
    // 1. 切换菜单
    HMENU hMenu = ::LoadMenu(_Module.GetModuleInstance(),
        MAKEINTRESOURCE(m_bChineseLanguage ? IDR_MAINFRAME : IDR_MAINFRAME_EN));
    if (hMenu) {
        HMENU hOldMenu = ::GetMenu(m_hWnd);
        ::SetMenu(m_hWnd, hMenu);
        ::DrawMenuBar(m_hWnd);
        if (hOldMenu) {
            ::DestroyMenu(hOldMenu);
        }
    }

    // 2. 刷新列表列标题
    if (m_todoList.IsWindow()) {
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT;

        lvc.pszText = (LPTSTR)GetString(StringID::ColCreateDate);
        m_todoList.SetColumn(0, &lvc);
        lvc.pszText = (LPTSTR)GetString(StringID::ColPriority);
        m_todoList.SetColumn(1, &lvc);
        lvc.pszText = (LPTSTR)GetString(StringID::ColDescription);
        m_todoList.SetColumn(2, &lvc);
        lvc.pszText = (LPTSTR)GetString(StringID::ColDeadline);
        m_todoList.SetColumn(3, &lvc);
    }

    if (m_doneList.IsWindow()) {
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT;

        lvc.pszText = (LPTSTR)GetString(StringID::ColPriority);
        m_doneList.SetColumn(0, &lvc);
        lvc.pszText = (LPTSTR)GetString(StringID::ColDescription);
        m_doneList.SetColumn(1, &lvc);
        lvc.pszText = (LPTSTR)GetString(StringID::ColDoneTime);
        m_doneList.SetColumn(2, &lvc);
    }

    // 3. 刷新详情面板空状态文字
    if (m_detailEmpty.IsWindow()) {
        m_detailEmpty.SetWindowText(GetString(StringID::ClickToViewDetail));
    }

    // 4. 刷新工具栏按钮文字
    if (m_toolbar.IsWindow()) {
        TBBUTTONINFO tbi = {0};
        tbi.cbSize = sizeof(tbi);
        tbi.dwMask = TBIF_TEXT;

        // 置顶按钮
        tbi.pszText = (LPTSTR)(m_bTopmost ? GetString(StringID::TbTopmostOn) : GetString(StringID::TbTopmost));
        m_toolbar.SetButtonInfo(ID_WINDOW_TOPMOST, &tbi);

        // 时间筛选按钮
        switch (m_timeFilter) {
        case TimeFilter::Today:
            tbi.pszText = (LPTSTR)GetString(StringID::TbFilterToday);
            break;
        case TimeFilter::ThisWeek:
            tbi.pszText = (LPTSTR)GetString(StringID::TbFilterWeek);
            break;
        default:
            tbi.pszText = (LPTSTR)GetString(StringID::TbFilter);
            break;
        }
        m_toolbar.SetButtonInfo(ID_TIME_FILTER, &tbi);

        // 新增按钮
        tbi.pszText = (LPTSTR)GetString(StringID::TbAdd);
        m_toolbar.SetButtonInfo(ID_TODO_ADD, &tbi);
    }

    // 5. 刷新项目筛选下拉框第一项
    if (m_projectFilter.IsWindow()) {
        int curSel = m_projectFilter.GetCurSel();
        m_projectFilter.DeleteString(0);
        m_projectFilter.InsertString(0, GetString(StringID::ProjectAll));
        if (curSel == 0) {
            m_projectFilter.SetCurSel(0);
        }
    }

    // 6. 刷新详情面板按钮
    if (m_btnClose.IsWindow()) {
        m_btnClose.SetWindowText(GetString(StringID::Close));
    }
    if (m_btnKeep.IsWindow()) {
        m_btnKeep.SetWindowText(m_bDetailPinned ? GetString(StringID::BtnUnpin) : GetString(StringID::BtnPin));
    }

    // 7. 刷新状态栏
    if (m_statusBar.IsWindow()) {
        m_statusBar.SetText(0, GetString(StringID::StatusReady), 0);
    }
}

const TodoItem* CMainFrame::GetItemByDisplayIndex(int displayIndex, bool isDoneList) const
{
    if (displayIndex < 0) return nullptr;
    return isDoneList
        ? m_doneList.GetItemByDisplayIndex(displayIndex)
        : m_todoList.GetItemByDisplayIndex(displayIndex);
}

UINT CMainFrame::GetItemIdByDisplayIndex(int displayIndex, bool isDoneList) const
{
    if (displayIndex < 0) return 0;
    return isDoneList
        ? m_doneList.GetItemIdByDisplayIndex(displayIndex)
        : m_todoList.GetItemIdByDisplayIndex(displayIndex);
}

bool CMainFrame::ReselectById(UINT id, bool isDoneList)
{
    if (id == 0) return false;
    CTodoListCtrl& list = isDoneList ? m_doneList : m_todoList;
    int displayIndex = list.FindDisplayIndexById(id);
    if (displayIndex < 0) return false;

    list.SetItemState(displayIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    list.EnsureVisible(displayIndex, FALSE);

    m_nSelectedIndex = displayIndex;
    m_bSelectedIsDone = isDoneList;
    ShowDetailPopup();
    UpdateDetailPanel(displayIndex, isDoneList);
    return true;
}
