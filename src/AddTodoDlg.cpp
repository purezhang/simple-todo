#include "stdafx.h"
#include "AddTodoDlg.h"
#include "MainFrm.h"  // for GetString

#ifdef _DEBUG
static void DebugTick(const TCHAR* tag)
{
    TCHAR buf[256];
    _stprintf_s(buf, _T("[AddTodoDlg] %s @ %llu ms\n"), tag, (unsigned long long)GetTickCount64());
    ::OutputDebugString(buf);
}

static void DebugSpan(const TCHAR* tag, ULONGLONG start, ULONGLONG end)
{
    TCHAR buf[256];
    _stprintf_s(buf, _T("[AddTodoDlg] %s Δ=%llums\n"), tag, (unsigned long long)(end - start));
    ::OutputDebugString(buf);
}
#else
static void DebugTick(const TCHAR*) {}
static void DebugSpan(const TCHAR*, ULONGLONG, ULONGLONG) {}
#endif

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
    m_item.project = L"";
    m_item.createTime = CTime::GetCurrentTime();
    m_item.targetEndTime = CTime::GetCurrentTime(); // 默认今天24点

    // 设置为今天24点
    CTime now = CTime::GetCurrentTime();
    m_item.targetEndTime = CTime(now.GetYear(), now.GetMonth(), now.GetDay(), 23, 59, 59);
    m_item.isDone = false;
}

CAddTodoDlg::CAddTodoDlg(const TodoItem& item)
    : m_item(item)
{
    // 使用传入的任务数据初始化
}

void CAddTodoDlg::SetTodoItem(const TodoItem& item)
{
    m_item = item;
}

LRESULT CAddTodoDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    ULONGLONG t0 = GetTickCount64();
    m_initStartTick = t0;
    DebugTick(_T("OnInitDialog BEGIN"));
    m_isInitializing = true;

    // 获取控件
    m_comboPriority = GetDlgItem(IDC_PRIORITY_COMBO);
#ifdef _DEBUG
    ::OutputDebugString(m_comboPriority.IsWindow() ? _T("Combo OK\n") : _T("Combo FAILED\n"));
#endif

    m_editTitle = GetDlgItem(IDC_TITLE_EDIT);
#ifdef _DEBUG
    ::OutputDebugString(m_editTitle.IsWindow() ? _T("Title OK\n") : _T("Title FAILED\n"));
#endif

    m_editNote = GetDlgItem(IDC_NOTE_EDIT);
#ifdef _DEBUG
    ::OutputDebugString(m_editNote.IsWindow() ? _T("Note OK\n") : _T("Note FAILED\n"));
#endif

    m_dateTime = GetDlgItem(IDC_END_TIME_DATETIME);
#ifdef _DEBUG
    ::OutputDebugString(m_dateTime.IsWindow() ? _T("DateTime OK\n") : _T("DateTime FAILED\n"));
#endif

    m_comboProject = GetDlgItem(IDC_PROJECT_COMBO);
#ifdef _DEBUG
    ::OutputDebugString(m_comboProject.IsWindow() ? _T("Project Combo OK\n") : _T("Project Combo FAILED\n"));
#endif

    // 动态设置对话框标题和标签（国际化）
    if (m_item.id == 0) {
        SetWindowText(GetString(StringID::DlgAddTodo));
    } else {
        SetWindowText(GetString(StringID::DlgEditTodo));
    }
    SetDlgItemText(IDC_LBL_TITLE, GetString(StringID::LblTitle));
    SetDlgItemText(IDC_LBL_NOTE, GetString(StringID::LblNote));
    SetDlgItemText(IDC_LBL_PRIORITY, GetString(StringID::LblPriority));
    SetDlgItemText(IDC_LBL_PROJECT, GetString(StringID::LblProject));
    SetDlgItemText(IDC_LBL_DEADLINE, GetString(StringID::LblDeadline));
    SetDlgItemText(IDC_TODAY_BTN, GetString(StringID::BtnToday));
    SetDlgItemText(IDC_TOMORROW_BTN, GetString(StringID::BtnTomorrow));
    SetDlgItemText(IDC_THIS_WEEK_BTN, GetString(StringID::BtnThisWeek));
    SetDlgItemText(IDOK, GetString(StringID::OK));
    SetDlgItemText(IDCANCEL, GetString(StringID::Cancel));

    // 初始化优先级下拉框
    m_comboPriority.AddString(GetString(StringID::PriorityP0));
    m_comboPriority.AddString(GetString(StringID::PriorityP1));
    m_comboPriority.AddString(GetString(StringID::PriorityP2));
    m_comboPriority.AddString(GetString(StringID::PriorityP3));

    // 初始化项目下拉框：只放当前项目（如果有），完整列表延迟到下拉
    ULONGLONG tProjBegin = GetTickCount64();
    m_projectsLoaded = false;
    m_comboProject.ResetContent();
    m_comboProject.AddString(GetString(StringID::ProjectNone));
    if (!m_item.project.empty()) {
        m_comboProject.AddString(m_item.project.c_str());
        m_comboProject.SetCurSel(1);
    } else {
        m_comboProject.SetCurSel(0);
    }
    DebugSpan(_T("InitProjectComboMinimal"), tProjBegin, GetTickCount64());

    // 根据任务数据初始化控件
    // 1. 标题
    ULONGLONG tTitle = GetTickCount64();
    m_editTitle.SetWindowText(m_item.title.c_str());
    DebugSpan(_T("SetTitleText"), tTitle, GetTickCount64());

    // 2. 备注
    ULONGLONG tNote = GetTickCount64();
    m_editNote.SetWindowText(m_item.note.c_str());
    DebugSpan(_T("SetNoteText"), tNote, GetTickCount64());

    // 3. 优先级
    ULONGLONG tPriority = GetTickCount64();
    m_comboPriority.SetCurSel((int)m_item.priority);
    DebugSpan(_T("SetPrioritySel"), tPriority, GetTickCount64());

    // 4. 项目
    DebugSpan(_T("SetProjectSel"), tProjBegin, GetTickCount64());

    // 5. 日期时间
    {
        ULONGLONG tDate = GetTickCount64();
        SYSTEMTIME st = {0};
        TimeToSystemTime(m_item.targetEndTime, st);
        DateTime_SetSystemtime(m_dateTime, GDT_VALID, &st);
        DebugSpan(_T("SetDateTime"), tDate, GetTickCount64());
    }

    // 设置焦点到标题输入框
    m_editTitle.SetFocus();

    ULONGLONG tCenter = GetTickCount64();
    CenterWindow(GetParent());
    DebugSpan(_T("CenterWindow"), tCenter, GetTickCount64());
    m_isInitializing = false;
    DebugTick(_T("OnInitDialog END"));
    DebugSpan(_T("OnInitDialog TOTAL"), t0, GetTickCount64());
    ::PostMessage(m_hWnd, WM_APP + 1, 0, 0);
    return FALSE; // 返回 FALSE 表示我们已经设置了焦点
}

LRESULT CAddTodoDlg::OnAfterInit(UINT, WPARAM, LPARAM, BOOL&)
{
    if (m_initStartTick != 0) {
        DebugSpan(_T("Dialog Ready (message pump)"), m_initStartTick, GetTickCount64());
    }
    if (m_invokeTick != 0) {
        DebugSpan(_T("Click->Ready"), m_invokeTick, GetTickCount64());
    }
    return 0;
}

LRESULT CAddTodoDlg::OnOK(WORD, WORD, HWND, BOOL&)
{
    // 获取优先级
    int nSel = m_comboPriority.GetCurSel();
    if (nSel >= 0 && nSel <= 3) {
        m_item.priority = (Priority)nSel;
    }

    // 获取项目（CBS_DROPDOWN 可编辑，需用 GetWindowText）
    CString strProject;
    m_comboProject.GetWindowText(strProject);
    strProject.Trim();
    if (!strProject.IsEmpty() && strProject != L"[无]") {
        m_item.project = strProject.GetString();
    } else {
        m_item.project.clear();
    }

#ifdef _DEBUG
    TCHAR szDebug[512];
    _stprintf_s(szDebug, _T("OnOK: project='%s', title='%s'\n"),
        m_item.project.c_str(), m_item.title.c_str());
    ::OutputDebugString(szDebug);
#endif

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
        MessageBox(GetString(StringID::TitleRequired), GetString(StringID::Tips), MB_OK | MB_ICONWARNING);
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

LRESULT CAddTodoDlg::OnTitleChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // 只处理标题编辑框的变化
    if (wID != IDC_TITLE_EDIT) {
        bHandled = FALSE;
        return 0;
    }
    if (m_isInitializing) {
        return 0;
    }

    // 获取当前输入的文本
    CString strText;
    m_editTitle.GetWindowText(strText);

    // 尝试解析自然语言
    DebugTick(_T("OnTitleChange"));
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
    int nDayOfWeek = now.GetDayOfWeek(); // 1=周日, 2=周一, ..., 6=周五, 7=周六
    // 计算到本周五的天数 (周五 = GetDayOfWeek() 返回 6)
    int nDaysToAdd = (6 - nDayOfWeek + 7) % 7;
    if (nDaysToAdd == 0) nDaysToAdd = 7; // 如果今天是周五，设为下周五

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
    if (pos != std::wstring::npos && pos >= 2) {
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

void CAddTodoDlg::UpdateProjectCombo()
{
    DebugTick(_T("UpdateProjectCombo BEGIN"));
    m_comboProject.SetRedraw(FALSE);
    // 清空下拉框
    m_comboProject.ResetContent();

    // 添加"无"选项（表示不选择任何项目）
    m_comboProject.AddString(GetString(StringID::ProjectNone));

    // 添加已有项目
    for (const auto& proj : m_projects) {
        m_comboProject.AddString(proj.c_str());
    }

    // 默认选中"无"
    m_comboProject.SetCurSel(0);
    m_comboProject.SetRedraw(TRUE);
    m_comboProject.Invalidate();
    DebugTick(_T("UpdateProjectCombo END"));
}

LRESULT CAddTodoDlg::OnProjectSelChange(WORD, WORD, HWND, BOOL&)
{
    // 可以在这里处理项目选择变化
    return 0;
}

LRESULT CAddTodoDlg::OnProjectDropDown(WORD, WORD, HWND, BOOL&)
{
    if (m_projectsLoaded) return 0;
    m_projectsLoaded = true;

    // 记住当前选择的文本（可能是当前项目）
    CString currentText;
    m_comboProject.GetWindowText(currentText);

    UpdateProjectCombo();

    if (!currentText.IsEmpty()) {
        int found = m_comboProject.FindStringExact(-1, currentText);
        if (found >= 0) {
            m_comboProject.SetCurSel(found);
        }
    }

    return 0;
}
