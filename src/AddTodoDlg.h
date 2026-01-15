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
        NOTIFY_CODE_HANDLER(EN_CHANGE, OnTitleChange)
        COMMAND_ID_HANDLER(IDC_TODAY_BTN, OnTodayBtn)
        COMMAND_ID_HANDLER(IDC_TOMORROW_BTN, OnTomorrowBtn)
        COMMAND_ID_HANDLER(IDC_THIS_WEEK_BTN, OnThisWeekBtn)
    END_MSG_MAP()

    CAddTodoDlg();
    TodoItem GetResult() const { return m_item; }

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnOK(WORD, WORD, HWND, BOOL&);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL&);
    LRESULT OnTitleChange(int idCtrl, LPNMHDR pnmh, BOOL&);
    LRESULT OnTodayBtn(WORD, WORD, HWND, BOOL&);
    LRESULT OnTomorrowBtn(WORD, WORD, HWND, BOOL&);
    LRESULT OnThisWeekBtn(WORD, WORD, HWND, BOOL&);

private:
    TodoItem m_item;

    CComboBox m_comboPriority;
    CEdit m_editTitle;
    CEdit m_editNote;
    CDateTimePickerCtrl m_dateTime;

    void ParseNaturalLanguage(const std::wstring& text);
    void UpdateEndTimeButtons();
};
