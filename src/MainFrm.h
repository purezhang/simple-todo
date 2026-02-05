#pragma once
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

    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_APP + 100, OnAppRefresh)
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

    LRESULT OnTodoAdd(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExport(WORD, WORD, HWND, BOOL&);
    LRESULT OnTodoExportTxt(WORD, WORD, HWND, BOOL&);
    LRESULT OnExpandAll(WORD, WORD, HWND, BOOL&);
    LRESULT OnCollapseAll(WORD, WORD, HWND, BOOL&);
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

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnIdle();

private:
    CHorSplitterWindow m_mainSplitter;
    CTodoListCtrl m_todoList;
    CTodoListCtrl m_doneList;
    CStatusBarCtrl m_statusBar;
    
    CStatic m_detailPanel;
    CEdit m_detailPriority;
    CEdit m_detailDescription;
    CEdit m_detailCreateTime;
    CEdit m_detailEndTime;
    CEdit m_detailNote;
    CEdit m_detailEmpty;

    CToolBarCtrl m_toolbar;
    CReBarCtrl m_rebar;

    TodoDataManager m_dataManager;

    int m_nSelectedIndex;
    bool m_bSelectedIsDone;

    bool m_bChineseLanguage;
    bool m_bTopmost = false;
    bool m_bFirstSize = true;
    bool m_bDetailVisible = false;

    CFont m_fontList;
    CImageList m_imgList;

    void SetupLists();
    void UpdateLists();
    void RefreshDisplay();

    void CreateDetailPanelControls();
    void UpdateDetailPanel(int index, bool isDoneList);
    void ShowDetailPopup();
    void HideDetailPopup();

    void ShowContextMenu(int index, bool isDoneList, POINT pt);
    LRESULT ChangePriority(Priority newPriority);

    void ExportToCSV();
    void ExportToTodoTxt();
};
