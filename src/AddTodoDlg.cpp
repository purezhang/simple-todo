#include "stdafx.h"
#include "AddTodoDlg.h"

// 辅助函数：CTime 转 SYSTEMTIME
static void TimeToSystemTime(const CTime& time, SYSTEMTIME& st)
{
    st.wYear = (WORD)time.GetYear();
    st.wMonth = (WORD)time.GetMonth();
    st.wDay = (WORD)time.GetDay();
    st.wHour = (WORD)time.GetHour();
    st.wMinute = (WORD)time.GetMinute();
    st.wSecond = (WORD)time.GetSecond();
    st.wMilliseconds = 0;
}

// 辅助函数：SYSTEMTIME 转 CTime
static void SystemTimeToTime(const SYSTEMTIME& st, CTime& time)
{
    time = CTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

CAddTodoDlg::CAddTodoDlg()
{
    // 默认值
    m_item.id = 0;
    m_item.priority = Priority::P1;
    m_item.title = L"";
    m_item.note = L"";
    m_item.createTime = CTime::GetCurrentTime();
    m_item.targetEndTime = CTime::GetCurrentTime(); // 默认今天24点

    // 设置为今天24点
    CTime now = CTime::GetCurrentTime();
    m_item.targetEndTime = CTime(now.GetYear(), now.GetMonth(), now.GetDay(), 23, 59, 59);
    m_item.isDone = false;
}

LRESULT CAddTodoDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    ::OutputDebugString(_T("CAddTodoDlg::OnInitDialog called\n"));

    // 获取控件
    m_comboPriority = GetDlgItem(IDC_PRIORITY_COMBO);
    ::OutputDebugString(m_comboPriority.IsWindow() ? _T("Combo OK\n") : _T("Combo FAILED\n"));

    m_editTitle = GetDlgItem(IDC_TITLE_EDIT);
    ::OutputDebugString(m_editTitle.IsWindow() ? _T("Title OK\n") : _T("Title FAILED\n"));

    m_editNote = GetDlgItem(IDC_NOTE_EDIT);
    ::OutputDebugString(m_editNote.IsWindow() ? _T("Note OK\n") : _T("Note FAILED\n"));

    m_dateTime = GetDlgItem(IDC_END_TIME_DATETIME);
    ::OutputDebugString(m_dateTime.IsWindow() ? _T("DateTime OK\n") : _T("DateTime FAILED\n"));

    // 初始化优先级下拉框
    m_comboPriority.AddString(_T("P0 紧急"));
    m_comboPriority.AddString(_T("P1 重要"));
    m_comboPriority.AddString(_T("P2 普通"));
    m_comboPriority.AddString(_T("P3 暂缓"));
    m_comboPriority.SetCurSel(1); // 默认 P1

    // 设置日期时间
    {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }

    // 设置焦点到标题输入框
    m_editTitle.SetFocus();

    CenterWindow(GetParent());
    return FALSE; // 返回 FALSE 表示我们已经设置了焦点
}

LRESULT CAddTodoDlg::OnOK(WORD, WORD, HWND, BOOL&)
{
    // 获取优先级
    int nSel = m_comboPriority.GetCurSel();
    if (nSel >= 0 && nSel <= 3) {
        m_item.priority = (Priority)nSel;
    }

    // 获取标题
    CString strTitle;
    m_editTitle.GetWindowText(strTitle);
    m_item.title = strTitle.GetString();

    // 获取备注
    CString strNote;
    m_editNote.GetWindowText(strNote);
    m_item.note = strNote.GetString();

    // 获取结束时间
    {
        SYSTEMTIME st = {0};
        DateTime_GetSystemtime(m_dateTime, &st);
        SystemTimeToTime(st, m_item.targetEndTime);
    }

    // 验证标题不为空
    if (m_item.title.empty()) {
        MessageBox(_T("请输入任务标题！"), _T("提示"), MB_OK | MB_ICONWARNING);
        m_editTitle.SetFocus();
        return 0;
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CAddTodoDlg::OnCancel(WORD, WORD, HWND, BOOL&)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CAddTodoDlg::OnTitleChange(int idCtrl, LPNMHDR pnmh, BOOL&)
{
    // 获取当前输入的文本
    CString strText;
    m_editTitle.GetWindowText(strText);

    // 尝试解析自然语言
    ParseNaturalLanguage(strText.GetString());

    return 0;
}

LRESULT CAddTodoDlg::OnTodayBtn(WORD, WORD, HWND, BOOL&)
{
    CTime now = CTime::GetCurrentTime();
    m_item.targetEndTime = CTime(now.GetYear(), now.GetMonth(), now.GetDay(), 23, 59, 59);
    {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
    return 0;
}

LRESULT CAddTodoDlg::OnTomorrowBtn(WORD, WORD, HWND, BOOL&)
{
    CTime tomorrow = CTime::GetCurrentTime() + CTimeSpan(1, 0, 0, 0);
    m_item.targetEndTime = CTime(tomorrow.GetYear(), tomorrow.GetMonth(), tomorrow.GetDay(), 23, 59, 59);
    {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
    return 0;
}

LRESULT CAddTodoDlg::OnThisWeekBtn(WORD, WORD, HWND, BOOL&)
{
    CTime now = CTime::GetCurrentTime();
    int nDayOfWeek = now.GetDayOfWeek(); // 1=周日, 2=周一, ...
    int nDaysToAdd = (8 - nDayOfWeek) % 7; // 周五之前，或者如果是周末则下周五
    if (nDaysToAdd == 0) nDaysToAdd = 7; // 如果是周日，加7天到下周日

    CTime thisWeek = now + CTimeSpan(nDaysToAdd, 0, 0, 0);
    m_item.targetEndTime = CTime(thisWeek.GetYear(), thisWeek.GetMonth(), thisWeek.GetDay(), 23, 59, 59);
    {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
    return 0;
}

void CAddTodoDlg::ParseNaturalLanguage(const std::wstring& text)
{
    // 简单的自然语言解析
    // 格式: "P0 开会 14:00" 或 "开会 明天"

    std::wstring lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);

    // 解析优先级
    size_t pos = 0;
    if ((pos = lowerText.find(L"p0")) != std::wstring::npos) {
        m_item.priority = Priority::P0;
        m_comboPriority.SetCurSel(0);
    } else if ((pos = lowerText.find(L"p1")) != std::wstring::npos) {
        m_item.priority = Priority::P1;
        m_comboPriority.SetCurSel(1);
    } else if ((pos = lowerText.find(L"p2")) != std::wstring::npos) {
        m_item.priority = Priority::P2;
        m_comboPriority.SetCurSel(2);
    } else if ((pos = lowerText.find(L"p3")) != std::wstring::npos) {
        m_item.priority = Priority::P3;
        m_comboPriority.SetCurSel(3);
    }

    // 解析时间 (HH:MM 格式)
    pos = text.find(L':');
    if (pos != std::wstring::npos && pos > 0) {
        // 尝试提取时间
        int hour = 0, minute = 0;
        if (swscanf_s(text.c_str() + pos - 2, L"%d:%d", &hour, &minute) == 2) {
            if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
                CTime now = CTime::GetCurrentTime();
                m_item.targetEndTime = CTime(now.GetYear(), now.GetMonth(),
                    now.GetDay(), hour, minute, 0);
                {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
            }
        }
    }

    // 解析 "今天"、"明天"等关键词
    CTime now = CTime::GetCurrentTime();
    if (text.find(L"今天") != std::wstring::npos) {
        m_item.targetEndTime = CTime(now.GetYear(), now.GetMonth(), now.GetDay(), 23, 59, 59);
        {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
    } else if (text.find(L"明天") != std::wstring::npos) {
        CTime tomorrow = now + CTimeSpan(1, 0, 0, 0);
        m_item.targetEndTime = CTime(tomorrow.GetYear(), tomorrow.GetMonth(), tomorrow.GetDay(), 23, 59, 59);
        {
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
    }
    }
}
