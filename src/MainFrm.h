#pragma once
#include "stdafx.h"
#include "TodoListCtrl.h"

// 声明 ATL 模块变量（定义在 SimpleTodo.cpp 中）
extern CAppModule _Module;

class CMainFrame :
    public CFrameWindowImpl<CMainFrame>,
    public CMessageFilter,
    public CIdleHandler
{
public:
    DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_APP + 100, OnAppRefresh)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    // 构造/析构
    CMainFrame();
    ~CMainFrame();

    // 消息处理器
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnAppRefresh(UINT, WPARAM, LPARAM, BOOL&);

    // 命令处理器
    LRESULT OnTodoAdd(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExport(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExportTxt(WORD, WORD, HWND, BOOL&);
    LRESULT OnExpandAll(WORD, WORD, HWND, BOOL&);
    LRESULT OnCollapseAll(WORD, WORD, HWND, BOOL&);
    LRESULT OnFileExit(WORD, WORD, HWND, BOOL&);
    LRESULT OnAppAbout(WORD, WORD, HWND, BOOL&);
    LRESULT OnLanguageChinese(WORD, WORD, HWND, BOOL&);
    LRESULT OnLanguageEnglish(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoComplete(UINT, WPARAM, LPARAM, BOOL&);  // 使用 MESSAGE_HANDLER
    LRESULT OnTodoDelete(UINT, WPARAM, LPARAM, BOOL&);       // 使用 MESSAGE_HANDLER
    LRESULT OnTodoEdit(UINT, WPARAM, LPARAM, BOOL&);        // 使用 MESSAGE_HANDLER
    LRESULT OnTodoContextMenu(UINT, WPARAM, LPARAM, BOOL&); // 使用 MESSAGE_HANDLER
    LRESULT OnContextMarkDone(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextCopyText(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPin(WORD, WORD, HWND, BOOL&); // 新增
    LRESULT OnContextPriorityP0(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP1(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP2(WORD, WORD, HWND, BOOL&);
    LRESULT OnContextPriorityP3(WORD, WORD, HWND, BOOL&);
    LRESULT OnToggleTopmost(WORD, WORD, HWND, BOOL&); // 窗口置顶功能

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnIdle();

private:
    // UI 组件
    CHorSplitterWindow m_mainSplitter; // 主分割器：上待办，下底部容器
    CSplitterWindow m_bottomSplitter;  // 底部分割器：左已完成，右详情
    CTodoListCtrl m_todoList;
    CTodoListCtrl m_doneList;
    CStatusBarCtrl m_statusBar;
    
    // 右侧详情面板
    CStatic m_detailPanel;
    CStatic m_detailPriority;
    CStatic m_detailDescription;
    CStatic m_detailCreateTime;
    CStatic m_detailEndTime; // 截止时间
    CStatic m_detailNote; // 备注
    CStatic m_detailEmpty; // 空状态提示
    
    // 分隔线
    CStatic m_detailSeparator1;
    CStatic m_detailSeparator2;

    // 工具栏
    CToolBarCtrl m_toolbar;
    CReBarCtrl m_rebar;

    // 数据管理
    TodoDataManager m_dataManager;

    // 当前选中的任务索引（用于右键菜单）
    int m_nSelectedIndex;
    bool m_bSelectedIsDone;

    // 语言设置 (true = 中文, false = 英文)
    bool m_bChineseLanguage;

    // 窗口置顶状态
    bool m_bTopmost = false;

    // 标志位：是否为第一次调整大小
    bool m_bFirstSize = true;

    // 全局 UI 资源
    CFont m_fontList;     // 列表字体 (Segoe UI)
    CImageList m_imgList; // 用于撑开行高的空图标列表

    // 初始化函数
    void InitializeUI();
    void SetupLists();
    void UpdateLists();
    void UpdateGroups();
    void RefreshDisplay();

    // 详情面板相关函数
    void CreateDetailPanelControls();
    void UpdateDetailPanel(int index, bool isDoneList);

    // 菜单处理
    void ShowContextMenu(int index, bool isDoneList, POINT pt);
    LRESULT ChangePriority(Priority newPriority);

    // 导出功能
    void ExportToCSV();
    void ExportToTodoTxt();
};
