#pragma once
#include "stdafx.h"

// 定义 ListView 分组消息常量（确保跨 Windows 版本兼容）
#ifndef LVM_INSERTGROUPW
#define LVM_INSERTGROUPW (LVM_FIRST + 145)
#endif
#ifndef LVM_GETGROUPCOUNT
#define LVM_GETGROUPCOUNT (LVM_FIRST + 152)
#endif
#ifndef LVM_REMOVEGROUP
#define LVM_REMOVEGROUP (LVM_FIRST + 146)
#endif
#ifndef LVM_GETGROUPINFO
#define LVM_GETGROUPINFO (LVM_FIRST + 153)
#endif

// 自定义列表控件，支持虚拟模式、分组视图和优先级颜色
class CTodoListCtrl :
    public CWindowImpl<CTodoListCtrl, CListViewCtrl>
{
public:
    DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

    BEGIN_MSG_MAP(CTodoListCtrl)
        // 转发通知到父窗口处理
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
        REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
        REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    // 构造函数
    CTodoListCtrl() : m_pDataManager(nullptr), m_isDoneList(false) {}

    // 设置数据管理器
    void SetDataManager(TodoDataManager* pManager) {
        m_pDataManager = pManager;
    }

    // 设置是否为 Done 列表
    void SetIsDoneList(bool isDone) {
        m_isDoneList = isDone;
    }

    // 设置搜索关键词
    void SetSearchKeyword(const std::wstring& keyword) {
        m_searchKeyword = keyword;
    }

    // 设置项目筛选关键词
    void SetProjectFilter(const std::wstring& project) {
        m_projectFilter = project;
    }

    // 设置时间筛选（0=全部, 1=今天, 2=本周）
    void SetTimeFilter(int timeFilter) {
        m_timeFilter = timeFilter;
    }

    // 获取是否为 Done 列表
    bool IsDoneList() const {
        return m_isDoneList;
    }

    // 获取数据管理器
    TodoDataManager* GetDataManager() const {
        return m_pDataManager;
    }

    // 刷新列表
    // 刷新列表 - 直接插入真实项目，不使用虚拟列表
    void RefreshList() {
        if (m_pDataManager) {
            TODO_DEBUG_LOGF(_T("RefreshList: isDoneList=%d, itemCount=%d\n"),
                m_isDoneList, m_pDataManager->GetItemCount(m_isDoneList));

            // 排序数据
            m_pDataManager->Sort(m_isDoneList);

            // 获取搜索过滤后的索引
            std::vector<int> filteredIndices = m_pDataManager->Search(m_searchKeyword, m_projectFilter, m_isDoneList, m_timeFilter);

            // 清空现有项目
            DeleteAllItems();

            // 确保禁用分组
            ListView_EnableGroupView(m_hWnd, FALSE);
            ListView_RemoveAllGroups(m_hWnd);

            // 直接插入过滤后的项目
            for (int i = 0; i < static_cast<int>(filteredIndices.size()); i++) {
                int originalIndex = filteredIndices[i];
                const TodoItem* pItem = m_pDataManager->GetItemAt(originalIndex, m_isDoneList);
                if (pItem) {
                    // 准备数据
                    CString strDate = pItem->createTime.Format(_T("%Y/%m/%d"));
                    CString strPriority = pItem->GetPriorityString();
                    if (pItem->isPinned && !m_isDoneList) {
                        strPriority = _T("📌 ") + strPriority;
                    }
                    CString strTitle(pItem->title.c_str());
                    CString strTime = m_isDoneList ? pItem->GetDoneTimeString() : pItem->GetCreateTimeString();

                    // 插入项目
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = i;

                    if (!m_isDoneList) {
                        // Todo 列表：[0]创建日期 [1]优先级 [2]标题 [3]截止时间
                        lvi.iSubItem = 0;
                        lvi.pszText = (LPTSTR)(LPCTSTR)strDate;
                        int idx = InsertItem(&lvi);

                        SetItemText(idx, 1, (LPTSTR)(LPCTSTR)strPriority);
                        SetItemText(idx, 2, (LPTSTR)(LPCTSTR)strTitle);

                        CString strEndTime = pItem->GetEndTimeString();
                        SetItemText(idx, 3, (LPTSTR)(LPCTSTR)strEndTime);
                    }
                    else {
                        // Done 列表：保持原样 [0]优先级 [1]标题 [2]完成时间
                        lvi.iSubItem = 0;
                        lvi.pszText = (LPTSTR)(LPCTSTR)strPriority;
                        int idx = InsertItem(&lvi);

                        SetItemText(idx, 1, (LPTSTR)(LPCTSTR)strTitle);
                        SetItemText(idx, 2, (LPTSTR)(LPCTSTR)strTime);
                    }

                    TODO_DEBUG_LOGF(_T("  Inserted item %d (orig=%d): %s\n"), i, originalIndex, pItem->title.c_str());
                }
            }

            TODO_DEBUG_LOGF(_T("RefreshList complete: inserted %d items (filtered from %d)\n"),
                filteredIndices.size(), m_pDataManager->GetItemCount(m_isDoneList));
        }
    }
    
    // RefreshGroups 已移除 (不再使用分组)

    // 获取指定索引的分组ID
    int GetGroupIdByIndex(int index) {
        LVGROUP group = {0};
        group.cbSize = sizeof(LVGROUP);
        group.mask = LVGF_GROUPID;
        // 使用 LVM_GETGROUPINFO 消息
        BOOL bResult = (BOOL)::SendMessage(m_hWnd, LVM_GETGROUPINFO, index, (LPARAM)&group);
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("GetGroupIdByIndex: index=%d, result=%d, groupId=%d\n"),
            index, bResult, group.iGroupId);
        ::OutputDebugString(szDebug);
        if (bResult) {
            return group.iGroupId;
        }
        return -1;
    }

    // 展开所有分组
    void ExpandAllGroups() {
        int groupCount = ListView_GetGroupCount(m_hWnd);
        for (int i = 0; i < groupCount; i++) {
            LVGROUP group = {0};
            group.cbSize = sizeof(LVGROUP);
            group.mask = LVGF_STATE;
            group.state = LVGS_NORMAL;
            group.stateMask = LVGS_COLLAPSED;
            SetGroupInfo(i, &group);
        }
        Invalidate(FALSE);
    }

    // 折叠所有分组
    void CollapseAllGroups() {
        int groupCount = ListView_GetGroupCount(m_hWnd);
        for (int i = 0; i < groupCount; i++) {
            LVGROUP group = {0};
            group.cbSize = sizeof(LVGROUP);
            group.mask = LVGF_STATE;
            group.state = LVGS_COLLAPSED;
            group.stateMask = LVGS_COLLAPSED;
            SetGroupInfo(i, &group);
        }
        Invalidate(FALSE);
    }

    // 点击事件
    LRESULT OnClick(int, LPNMHDR pnmh, BOOL&) {
        NMITEMACTIVATE* pItemAct = reinterpret_cast<NMITEMACTIVATE*>(pnmh);

        if (pItemAct->iItem >= 0) {
            // 通知父窗口更新详情面板
            // 使用自定义消息或扩展当前消息处理
            ::PostMessage(GetParent(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(pnmh));
            
            if (!m_isDoneList) {
                // Todo 列表：点击复选框完成任务
                LVHITTESTINFO hti = {0};
                hti.pt = pItemAct->ptAction;
                SubItemHitTest(&hti);

                if (hti.iItem == pItemAct->iItem && hti.iSubItem == 1) {
                    // 点击的是描述列，可以切换完成状态
                    NotifyParentCompleteTask(pItemAct->iItem);
                }
            }
        }

        return 0;
    }

    // 双击事件 - 编辑任务
    LRESULT OnDblClick(int, LPNMHDR pnmh, BOOL&) {
        NMITEMACTIVATE* pItemAct = reinterpret_cast<NMITEMACTIVATE*>(pnmh);

        if (pItemAct->iItem >= 0) {
            NotifyParentEditTask(pItemAct->iItem);
        }

        return 0;
    }

    // 键盘事件
    LRESULT OnKeyDown(int, LPNMHDR pnmh, BOOL&) {
        NMLVKEYDOWN* pKeyDown = reinterpret_cast<NMLVKEYDOWN*>(pnmh);

        if (pKeyDown->wVKey == VK_DELETE && !m_isDoneList) {
            // Delete 键删除任务
            int sel = GetSelectedIndex();
            if (sel >= 0) {
                NotifyParentDeleteTask(sel);
            }
        } else if (pKeyDown->wVKey == VK_SPACE && !m_isDoneList) {
            // 空格键完成任务
            int sel = GetSelectedIndex();
            if (sel >= 0) {
                NotifyParentCompleteTask(sel);
            }
        }

        return 0;
    }

    // 自定义绘制 - 实现优先级颜色和样式
    LRESULT OnCustomDraw(int, LPNMHDR pnmh, BOOL&) {
        NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);

        switch (pLVCD->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT: {
                // 如果被选中，使用系统默认绘制，忽略自定义颜色
                if (pLVCD->nmcd.uItemState & CDIS_SELECTED) {
                    return CDRF_DODEFAULT;
                }

                const TodoItem* pItem = m_pDataManager->GetItemAt(
                    static_cast<int>(pLVCD->nmcd.dwItemSpec), m_isDoneList);
                
                if (pItem) {
                    if (m_isDoneList) {
                        // 已完成：灰色
                        pLVCD->clrText = RGB(128, 128, 128);
                    } else {
                        // 待办：根据优先级着色
                        // P0: 红色, P1: 深橙/赭色 (避免纯黄看不清)
                        switch (pItem->priority) {
                            case Priority::P0: pLVCD->clrText = RGB(200, 0, 0); break;
                            case Priority::P1: pLVCD->clrText = RGB(180, 100, 0); break;
                            case Priority::P2: pLVCD->clrText = RGB(0, 0, 0); break; // 默认黑
                            case Priority::P3: pLVCD->clrText = RGB(100, 100, 100); break; // 低优灰
                            default: pLVCD->clrText = RGB(0, 0, 0); break;
                        }
                    }
                }
                return CDRF_DODEFAULT; // 让系统继续绘制文本，但使用我们设置的颜色
            }

            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                return CDRF_DODEFAULT;
        }

        return CDRF_DODEFAULT;
    }

    // 右键菜单
    LRESULT OnRClick(int, LPNMHDR pnmh, BOOL&) {
        NMITEMACTIVATE* pItemAct = reinterpret_cast<NMITEMACTIVATE*>(pnmh);

        if (pItemAct->iItem >= 0) {
            NotifyParentShowContextMenu(pItemAct->iItem, pItemAct->ptAction);
        }

        return 0;
    }

    // 虚拟列表获取显示信息 - 直接处理
    LRESULT OnGetDispInfo(int, LPNMHDR pnmh, BOOL&) {
        NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);

        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("CTodoListCtrl::OnGetDispInfo: item=%d, isDone=%d, mask=0x%08X\n"),
            pDispInfo->item.iItem, m_isDoneList, pDispInfo->item.mask);
        ::OutputDebugString(szDebug);

        if (m_pDataManager) {
            int itemCount = m_pDataManager->GetItemCount(m_isDoneList);
            _stprintf_s(szDebug, _T("  Item count in OnGetDispInfo: %d\n"), itemCount);
            ::OutputDebugString(szDebug);

            if (pDispInfo->item.iItem >= 0 && pDispInfo->item.iItem < itemCount) {
                const TodoItem* pItem = m_pDataManager->GetItemAt(pDispInfo->item.iItem, m_isDoneList);
                if (pItem) {
                    _stprintf_s(szDebug, _T("  Got item: id=%d, title='%s'\n"), pItem->id, pItem->title.c_str());
                    ::OutputDebugString(szDebug);

                    // 【关键修复】设置分组ID（即使 mask 不包含 LVIF_GROUPID 也要设置）
                    pDispInfo->item.iGroupId = pItem->GetGroupId();
                    _stprintf_s(szDebug, _T("  Set iGroupId=%d\n"), pDispInfo->item.iGroupId);
                    ::OutputDebugString(szDebug);

                    // 确保返回分组信息
                    pDispInfo->item.mask |= LVIF_GROUPID;

                    // 设置文本
                    if (pDispInfo->item.mask & LVIF_TEXT) {
                        _stprintf_s(szDebug, _T("  Setting text for subitem=%d\n"), pDispInfo->item.iSubItem);
                        ::OutputDebugString(szDebug);
                        
                        CString strValue;
                        switch (pDispInfo->item.iSubItem) {
                            case 0:
                                // [0] 这里的逻辑稍微有点复杂，因为 Done 和 Todo 第一列不一样
                                if (m_isDoneList) strValue = pItem->GetPriorityString();
                                else strValue = pItem->createTime.Format(_T("%Y/%m/%d")); // 创建日期
                                break;
                            case 1:
                                // [1]
                                if (m_isDoneList) strValue = pItem->title.c_str();
                                else strValue = pItem->GetPriorityString(); // 优先级
                                break;
                            case 2:
                                // [2]
                                if (m_isDoneList) strValue = pItem->GetDoneTimeString();
                                else strValue = pItem->title.c_str(); // 标题
                                break;
                            case 3:
                                // [3] Todo 列表的截止时间
                                if (!m_isDoneList) {
                                    strValue = pItem->GetEndTimeString();
                                }
                                break;
                            default:
                                strValue = _T("");
                                break;
                        }
                        
                        _stprintf_s(szDebug, _T("  Setting value: '%s'\n"), (LPCTSTR)strValue);
                        ::OutputDebugString(szDebug);
                        
                        // 确保文本不会溢出
                        if (pDispInfo->item.pszText != nullptr && pDispInfo->item.cchTextMax > 0) {
                            wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                (LPCTSTR)strValue, _TRUNCATE);
                            _stprintf_s(szDebug, _T("  Text copied successfully\n"));
                            ::OutputDebugString(szDebug);
                        } else {
                            _stprintf_s(szDebug, _T("  Warning: pszText is null or cchTextMax is 0\n"));
                            ::OutputDebugString(szDebug);
                        }
                    }
                } else {
                    _stprintf_s(szDebug, _T("  WARNING: GetItemAt returned null for index=%d\n"), pDispInfo->item.iItem);
                    ::OutputDebugString(szDebug);
                }
            } else {
                _stprintf_s(szDebug, _T("  WARNING: Invalid item index=%d (count=%d)\n"), pDispInfo->item.iItem, itemCount);
                ::OutputDebugString(szDebug);
            }
        } else {
            _stprintf_s(szDebug, _T("  WARNING: m_pDataManager is null\n"));
            ::OutputDebugString(szDebug);
        }

        return 0;
    }

private:
    TodoDataManager* m_pDataManager;
    bool m_isDoneList;
    std::wstring m_searchKeyword;
    std::wstring m_projectFilter;
    int m_timeFilter;  // 时间筛选（0=全部, 1=今天, 2=本周）

    // 获取选中的项目索引
    int GetSelectedIndex() {
        return GetNextItem(-1, LVNI_SELECTED);
    }

    // 通知父窗口完成任务
    void NotifyParentCompleteTask(int index) {
        ::PostMessage(GetParent(), WM_COMMAND, ID_TODO_COMPLETE,
            MAKELPARAM(index, m_isDoneList ? 1 : 0));
    }

    // 通知父窗口删除任务
    void NotifyParentDeleteTask(int index) {
        ::PostMessage(GetParent(), WM_COMMAND, ID_TODO_DELETE,
            MAKELPARAM(index, m_isDoneList ? 1 : 0));
    }

    // 通知父窗口编辑任务
    void NotifyParentEditTask(int index) {
        ::PostMessage(GetParent(), WM_COMMAND, ID_TODO_EDIT,
            MAKELPARAM(index, m_isDoneList ? 1 : 0));
    }

    // 通知父窗口显示右键菜单
    void NotifyParentShowContextMenu(int index, POINT pt) {
        ::PostMessage(GetParent(), WM_COMMAND, ID_TODO_CONTEXT_MENU,
            MAKELPARAM(index, m_isDoneList ? 1 : 0));
    }
};
