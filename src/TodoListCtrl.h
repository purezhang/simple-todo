#pragma once
#include "stdafx.h"

// 列索引枚举 (C1: 定义列枚举避免魔法数字)
enum TodoColumns { 
    TODO_COL_DATE = 0,      // 创建日期
    TODO_COL_PRIORITY = 1,  // 优先级
    TODO_COL_TITLE = 2,     // 标题
    TODO_COL_DUE = 3        // 截止时间
};

enum DoneColumns { 
    DONE_COL_PRIORITY = 0,  // 优先级
    DONE_COL_TITLE = 1,     // 标题
    DONE_COL_TIME = 2       // 完成时间
};

// 自定义列表控件 - 完整虚拟列表模式 (LVS_OWNERDATA + LVN_GETDISPINFO)
class CTodoListCtrl :
    public CWindowImpl<CTodoListCtrl, CListViewCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

    BEGIN_MSG_MAP(CTodoListCtrl)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    CTodoListCtrl() : m_pDataManager(nullptr), m_isDoneList(false), m_timeFilter(0) {}

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL& bHandled) {
        bHandled = FALSE;
        return 0;
    }

    void SetDataManager(TodoDataManager* pManager) { m_pDataManager = pManager; }
    void SetIsDoneList(bool isDone) { m_isDoneList = isDone; }
    void SetSearchKeyword(const std::wstring& keyword) { m_searchKeyword = keyword; }
    void SetProjectFilter(const std::wstring& project) { m_projectFilter = project; }
    void SetTimeFilter(int timeFilter) { m_timeFilter = timeFilter; }
    bool IsDoneList() const { return m_isDoneList; }
    TodoDataManager* GetDataManager() const { return m_pDataManager; }

    // ========================================================================
    // 虚拟列表核心：RefreshList 强制重绘
    // ========================================================================
    void RefreshList() {
        if (!m_pDataManager) return;

        // 搜索过滤，获取映射（排序由 DataManager 内部管理）
        m_displayToDataIndex = m_pDataManager->Search(
            m_searchKeyword, m_projectFilter, m_isDoneList, m_timeFilter);

        // 设置虚拟列表项数量
        int itemCount = static_cast<int>(m_displayToDataIndex.size());
        SetItemCountEx(itemCount, 0);

        // 强制重绘：先关闭重绘，刷新后再开启
        SetRedraw(FALSE);
        SetRedraw(TRUE);

        // 强制刷新整个控件
        ::InvalidateRect(m_hWnd, NULL, TRUE);
        UpdateWindow();
    }

    // 通过显示索引获取数据项 (带边界保护)
    const TodoItem* GetItemByDisplayIndex(int displayIndex) const {
        if (!m_pDataManager) return nullptr;
        if (displayIndex < 0 || displayIndex >= static_cast<int>(m_displayToDataIndex.size()))
            return nullptr;
        int dataIndex = m_displayToDataIndex[displayIndex];
        if (dataIndex < 0 || dataIndex >= m_pDataManager->GetItemCount(m_isDoneList))
            return nullptr;
        return m_pDataManager->GetItemAt(dataIndex, m_isDoneList);
    }

    // 通过显示索引获取 item id (用于父窗口通信)
    UINT GetItemIdByDisplayIndex(int displayIndex) const {
        const TodoItem* pItem = GetItemByDisplayIndex(displayIndex);
        return pItem ? pItem->id : 0;
    }
    
    int FindDisplayIndexById(UINT id) const {
        if (!m_pDataManager || id == 0) return -1;
        for (int i = 0; i < static_cast<int>(m_displayToDataIndex.size()); ++i) {
            const TodoItem* pItem = GetItemByDisplayIndex(i);
            if (pItem && pItem->id == id) return i;
        }
        return -1;
    }

    // ========================================================================
    // OnGetDispInfo - 虚拟列表数据提供
    // ========================================================================
    LRESULT OnGetDispInfo(int, LPNMHDR pnmh, BOOL& bHandled) {
        NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
        bHandled = TRUE;

        int displayIndex = pDispInfo->item.iItem;
        
        const TodoItem* pItem = GetItemByDisplayIndex(displayIndex);
        if (!pItem) {
            return 0;
        }

        if (pDispInfo->item.mask & LVIF_TEXT) {
            CString strValue;

            if (!m_isDoneList) {
                switch (pDispInfo->item.iSubItem) {
                    case TODO_COL_DATE:
                        strValue = pItem->createTime.Format(_T("%Y/%m/%d"));
                        break;
                    case TODO_COL_PRIORITY:
                        strValue = pItem->GetPriorityString();
                        if (pItem->isPinned) strValue = _T("📌 ") + strValue;
                        break;
                    case TODO_COL_TITLE:
                        strValue = pItem->title.c_str();
                        break;
                    case TODO_COL_DUE:
                        // 问题8: 明确处理未设置截止时间的情况
                        if (pItem->targetEndTime.GetTime() > 0)
                            strValue = pItem->GetEndTimeString();
                        else
                            strValue = _T("-");
                        break;
                    default:
                        strValue = _T("");
                        break;
                }
            } else {
                switch (pDispInfo->item.iSubItem) {
                    case DONE_COL_PRIORITY:
                        strValue = pItem->GetPriorityString();
                        break;
                    case DONE_COL_TITLE:
                        strValue = pItem->title.c_str();
                        break;
                    case DONE_COL_TIME:
                        strValue = pItem->GetDoneTimeString();
                        break;
                    default:
                        strValue = _T("");
                        break;
                }
            }

            if (pDispInfo->item.pszText && pDispInfo->item.cchTextMax > 0) {
                wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                    (LPCTSTR)strValue, _TRUNCATE);
            }
        }

        return 0;
    }

    // ========================================================================
    // CustomDraw - 修复文本显示问题
    // ========================================================================
    LRESULT OnCustomDraw(int, LPNMHDR pnmh, BOOL& bHandled) {
        NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);
        bHandled = TRUE;

        switch (pLVCD->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT: {
                // 强制设置默认黑色文本和白色背景
                pLVCD->clrText = RGB(0, 0, 0);
                pLVCD->clrTextBk = RGB(255, 255, 255);

                if (m_isDoneList) {
                    // Done 列表: 灰色文本，白色背景
                    pLVCD->clrText = RGB(128, 128, 128);
                    pLVCD->clrTextBk = RGB(255, 255, 255);
                    return CDRF_DODEFAULT;
                }

                // Todo 列表: 请求子项通知，并要求应用字体（防止主题覆盖）
                return CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
            }

            case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
                int displayIndex = static_cast<int>(pLVCD->nmcd.dwItemSpec);
                const TodoItem* pItem = GetItemByDisplayIndex(displayIndex);
                
                // 强制设置默认黑色文本和白色背景
                pLVCD->clrText = RGB(0, 0, 0);
                pLVCD->clrTextBk = RGB(255, 255, 255);
                
                if (!pItem) {
                    return CDRF_DODEFAULT;
                }

                int subItem = pLVCD->iSubItem;

                // 优先级列染色
                if (subItem == TODO_COL_PRIORITY) {
                    switch (pItem->priority) {
                        case Priority::P0: pLVCD->clrText = RGB(220, 38, 38); break;  // 红
                        case Priority::P1: pLVCD->clrText = RGB(217, 119, 6); break;  // 橙
                        case Priority::P2: pLVCD->clrText = RGB(0, 0, 0); break;      // 黑
                        case Priority::P3: pLVCD->clrText = RGB(107, 114, 128); break;// 灰
                        default: break;
                    }
                }
                // 截止时间列：过期红色
                else if (subItem == TODO_COL_DUE) {
                    if (pItem->targetEndTime.GetTime() > 0) {
                        CTime now = CTime::GetCurrentTime();
                        if (pItem->targetEndTime < now) {
                            pLVCD->clrText = RGB(220, 38, 38);
                        }
                    }
                }
                // 确保标题列使用黑色文本
                else if (subItem == TODO_COL_TITLE || subItem == DONE_COL_TITLE) {
                    pLVCD->clrText = RGB(0, 0, 0);
                }
                // 确保其他列使用黑色文本
                else {
                    pLVCD->clrText = RGB(0, 0, 0);
                }

                return CDRF_DODEFAULT;
            }
        }

        return CDRF_DODEFAULT;
    }

    // 点击事件 - 由 MainFrm::OnNotify 统一处理，这里不再发消息
    LRESULT OnClick(int, LPNMHDR pnmh, BOOL&) {
        // 问题1修复: 删除 WM_USER+100，避免与 OnNotify 重复处理
        return 0;
    }

    // 双击事件 - 编辑任务
    LRESULT OnDblClick(int, LPNMHDR pnmh, BOOL&) {
        // 由父窗口 OnNotify 统一处理
        return 0;
    }

    // 键盘事件
    LRESULT OnKeyDown(int, LPNMHDR pnmh, BOOL&) {
        NMLVKEYDOWN* pKeyDown = reinterpret_cast<NMLVKEYDOWN*>(pnmh);

        if (pKeyDown->wVKey == VK_DELETE && !m_isDoneList) {
            int sel = GetNextItem(-1, LVNI_SELECTED);
            if (sel >= 0) NotifyParentDeleteTask(sel);
        } else if (pKeyDown->wVKey == VK_SPACE && !m_isDoneList) {
            int sel = GetNextItem(-1, LVNI_SELECTED);
            if (sel >= 0) NotifyParentCompleteTask(sel);
        }

        return 0;
    }

    // 右键菜单
    LRESULT OnRClick(int, LPNMHDR pnmh, BOOL&) {
        // 由父窗口 OnNotify 统一处理
        return 0;
    }

private:
    TodoDataManager* m_pDataManager;
    bool m_isDoneList;
    std::wstring m_searchKeyword;
    std::wstring m_projectFilter;
    int m_timeFilter;
    std::vector<int> m_displayToDataIndex;

    // ========================================================================
    // 问题4: 通知父窗口时传递 item->id 而非 displayIndex
    // ========================================================================
    void NotifyParentCompleteTask(int displayIndex) {
        ::PostMessage(GetParent(), WM_COMMAND,
            MAKEWPARAM(ID_TODO_COMPLETE, m_isDoneList ? 1 : 0),
            (LPARAM)displayIndex);
    }

    void NotifyParentDeleteTask(int displayIndex) {
        ::PostMessage(GetParent(), WM_COMMAND,
            MAKEWPARAM(ID_TODO_DELETE, m_isDoneList ? 1 : 0),
            (LPARAM)displayIndex);
    }

    void NotifyParentEditTask(int displayIndex) {
        ::PostMessage(GetParent(), WM_COMMAND,
            MAKEWPARAM(ID_TODO_EDIT, m_isDoneList ? 1 : 0),
            (LPARAM)displayIndex);
    }

    void NotifyParentShowContextMenu(int displayIndex, POINT pt) {
        ::PostMessage(GetParent(), WM_COMMAND,
            MAKEWPARAM(ID_TODO_CONTEXT_MENU, m_isDoneList ? 1 : 0),
            (LPARAM)displayIndex);
    }
};
