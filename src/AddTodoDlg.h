#pragma once
#include "stdafx.h"

// 添加任务对话框
class CAddTodoDlg :
    public CDialogImpl<CAddTodoDlg>
{
public:
    enum { IDD = IDD_ADDTODO };

    BEGIN_MSG_MAP(CAddTodoDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_CODE_HANDLER(EN_CHANGE, OnTitleChange)
        COMMAND_ID_HANDLER(IDC_TODAY_BTN, OnTodayBtn)
        COMMAND_ID_HANDLER(IDC_TOMORROW_BTN, OnTomorrowBtn)
        COMMAND_ID_HANDLER(IDC_THIS_WEEK_BTN, OnThisWeekBtn)
        COMMAND_ID_HANDLER(IDC_PROJECT_COMBO, OnProjectSelChange)
    END_MSG_MAP()

    CAddTodoDlg();
    CAddTodoDlg(const TodoItem& item);
    TodoItem GetResult() const { return m_item; }
    void SetTodoItem(const TodoItem& item);

    // 设置可用的项目列表
    void SetProjects(const std::vector<std::wstring>& projects) {
        m_projects = projects;
    }

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnOK(WORD, WORD, HWND, BOOL&);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
    LRESULT OnTitleChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTodayBtn(WORD, WORD, HWND, BOOL&);
    LRESULT OnTomorrowBtn(WORD, WORD, HWND, BOOL&);
    LRESULT OnThisWeekBtn(WORD, WORD, HWND, BOOL&);
    LRESULT OnProjectSelChange(WORD, WORD, HWND, BOOL&);

private:
    TodoItem m_item;
    std::vector<std::wstring> m_projects;  // 可用项目列表

    CComboBox m_comboPriority;
    CComboBox m_comboProject;  // 项目下拉框
    CEdit m_editTitle;
    CEdit m_editNote;
    CDateTimePickerCtrl m_dateTime;

    void ParseNaturalLanguage(const std::wstring& text);
    void UpdateEndTimeButtons();
    void UpdateProjectCombo();  // 更新项目下拉框
};
