#pragma once

// ============================================================================
// 国际化字符串ID枚举
// ============================================================================
enum class StringID {
    // 通用
    Tips,               // 提示
    AboutTitle,         // 关于 Simple Todo
    OK,                 // 确定
    Cancel,             // 取消
    Close,              // 关闭
    Yes,                // 是
    No,                 // 否

    // 任务相关
    TitleRequired,      // 请输入任务标题！
    ClickToViewDetail,  // 点击任务查看详情

    // 对话框标签
    DlgAddTodo,         // 添加任务
    DlgEditTodo,        // 编辑任务
    LblTitle,           // 标题 *
    LblNote,            // 备注
    LblPriority,        // 优先级
    LblProject,         // 项目
    LblDeadline,        // 截止时间
    BtnToday,           // 今天
    BtnTomorrow,        // 明天
    BtnThisWeek,        // 本周

    // 优先级
    PriorityP0,         // P0 紧急
    PriorityP1,         // P1 重要
    PriorityP2,         // P2 普通
    PriorityP3,         // P3 暂缓

    // 右键菜单
    MarkAsDone,         // 标记为完成
    MarkAsUndone,       // 标记为未完成
    Edit,               // 编辑
    Delete,             // 删除
    Pin,                // 置顶
    Unpin,              // 取消置顶
    CopyText,           // 复制文本
    SetPriority,        // 设置优先级

    // 列标题
    ColCreateDate,      // 创建日期
    ColPriority,        // 优先级
    ColDescription,     // 任务描述
    ColDeadline,        // 截止时间
    ColDoneTime,        // 完成时间

    // 筛选器
    FilterAll,          // 全部
    FilterToday,        // 今天
    FilterThisWeek,     // 本周
    ProjectAll,         // [全部]
    ProjectNone,        // [无]

    // 工具栏
    TbTopmost,          // 📌置顶
    TbTopmostOn,        // 📌已顶
    TbFilter,           // 🏷全部
    TbFilterToday,      // 🏷今天
    TbFilterWeek,       // 🏷本周
    TbAdd,              // 🆕新增

    // 详情面板
    DetailPriority,     // 优先级：
    DetailDescription,  // 任务描述：
    DetailCreateTime,   // 创建时间：
    DetailDeadline,     // 截止时间：
    DetailDeadlineNone, // 截止时间：未设置
    DetailProject,      // 分组：
    DetailNote,         // 备注：
    DetailNone,         // (无)
    BtnPin,             // 固定
    BtnUnpin,           // 取消

    // 状态栏
    StatusReady,        // 就绪

    // 导出
    ExportSuccess,      // 导出成功

    COUNT               // 总数标记
};

// 获取当前语言字符串（声明）
LPCTSTR GetString(StringID id);

// 注册表常量
static const TCHAR* REG_KEY_LANGUAGE = _T("Language");

#include "stdafx.h"
#include "TodoListCtrl.h"

extern CAppModule _Module;

class CMainFrame :
    public CFrameWindowImpl<CMainFrame>,
    public CMessageFilter,
    public CIdleHandler
{
public:
    DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

    // Timer 常量定义
    static constexpr UINT_PTR IDT_STATUS_CLEAR = 1001;
    static constexpr UINT_PTR IDT_FORCE_REFRESH = 2000;
    static constexpr UINT_PTR IDT_SEARCH_DEBOUNCE = 2001;

    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_APP + 100, OnAppRefresh)
        MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)

        // 关键：添加消息反射，将通知发回给子控件处理数据和自绘
        // 虚拟列表的 LVN_GETDISPINFO、NM_CUSTOMDRAW 等需要反射回 TodoListCtrl 处理
        REFLECT_NOTIFICATIONS()

        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    CMainFrame();
    ~CMainFrame();

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnAppRefresh(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCtlColorListBox(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnTodoAdd(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExport(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExportTxt(WORD, WORD, HWND, BOOL&);
    LRESULT OnFileExit(WORD, WORD, HWND, BOOL&);
    LRESULT OnAppAbout(WORD, WORD, HWND, BOOL&);
    LRESULT OnLanguageChinese(WORD, WORD, HWND, BOOL&);
    LRESULT OnLanguageEnglish(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoComplete(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTodoDelete(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTodoEdit(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTodoContextMenu(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnContextMarkDone(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextCopyText(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPin(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP0(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP1(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP2(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP3(WORD, WORD, HWND, BOOL&);
    LRESULT OnToggleTopmost(WORD, WORD, HWND, BOOL&);
    LRESULT OnToggleTimeFilter(WORD, WORD, HWND, BOOL&);
    LRESULT OnProjectFilterChanged(WORD, WORD, HWND, BOOL&);

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnIdle();

public:
    HWND m_hToolbar = nullptr;  // 保存 ToolBar 句柄

private:
    // 工具栏按钮文字定义
    static LPCTSTR TOPMOST_TEXT_NORMAL;
    static LPCTSTR TOPMOST_TEXT_CHECKED;
    static LPCTSTR TIME_FILTER_TODAY;
    static LPCTSTR TIME_FILTER_WEEK;
    static LPCTSTR TIME_FILTER_ALL;
    CHorSplitterWindow m_mainSplitter;
    CTodoListCtrl m_todoList;
    CTodoListCtrl m_doneList;
    CStatusBarCtrl m_statusBar;
    
    CStatic m_detailPanel;
    CEdit m_detailPriority;
    CEdit m_detailDescription;
    CEdit m_detailCreateTime;
    CEdit m_detailEndTime;
    CEdit m_detailProject;
    CEdit m_detailNote;
    CEdit m_detailEmpty;

    // 详情面板按钮
    CButton m_btnClose;     // 关闭按钮（右下角）
    CButton m_btnKeep;      // 固定/取消按钮（右下角）
    bool m_bDetailPinned = false;  // 详情面板固定模式

    CToolBarCtrl m_toolbar;
    CReBarCtrl m_rebar;

    // 搜索框
    CEdit m_searchEdit;
    CStatic m_searchLabel;
    CStatic m_searchContainer;  // 搜索框容器：组合图标和输入框
    CStatic m_spacer;          // ReBar 填充控件，用于右侧布局

    // 项目筛选下拉框
    CComboBox m_projectFilter;
    std::wstring m_currentProjectFilter;  // 空=全部
    void UpdateProjectFilterList();

    TodoDataManager m_dataManager;

    int m_nSelectedIndex;
    bool m_bSelectedIsDone;

    bool m_bChineseLanguage;
    bool m_bTopmost = false;
    bool m_bFirstSize = true;
    bool m_bDetailVisible = false;
    bool m_bDialogOpen = false;

    // 筛选状态
    enum class TimeFilter { All = 0, Today = 1, ThisWeek = 2 };
    TimeFilter m_timeFilter = TimeFilter::All;

    // 搜索相关
    CString m_searchKeyword;
    void OnSearchChanged();

    CFont m_fontList;
    CImageList m_imgList;

    void SetupLists();
    void UpdateLists();
    void RefreshDisplay();
    void AdjustTodoListColumnWidths(int cx);

    void CreateDetailPanelControls();
    void UpdateDetailPanel(int index, bool isDoneList);
    void ShowDetailPopup();
    void HideDetailPopup();

    void ShowContextMenu(int index, bool isDoneList, POINT pt);
    LRESULT ChangePriority(Priority newPriority);

    void ExportToCSV();
    void ExportToTodoTxt();

    // 窗口设置保存/加载
    void LoadWindowSettings();
    void SaveWindowSettings();

    // 语言切换
    void LoadLanguageSetting();
    void SaveLanguageSetting();
    void ApplyLanguage();

    std::wstring EscapeCSV(const std::wstring& s);
    const TodoItem* GetItemByDisplayIndex(int displayIndex, bool isDoneList) const;
    UINT GetItemIdByDisplayIndex(int displayIndex, bool isDoneList) const;
    bool ReselectById(UINT id, bool isDoneList);

private:
    // 手动子类化：保存原始窗口过程（用于详情面板按钮消息转发）
    WNDPROC m_originalReBarWndProc = nullptr;
    WNDPROC m_originalSearchContainerWndProc = nullptr;
    WNDPROC m_originalComboWndProc = nullptr;
    WNDPROC m_originalDetailPanelWndProc = nullptr;

    // 窗口设置相关
    static const TCHAR* REG_KEY_PATH;
    int m_nSplitterPos = 0;
};
