#pragma once
#include "stdafx.h"

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

    // 获取是否为 Done 列表
    bool IsDoneList() const {
        return m_isDoneList;
    }

    // 获取数据管理器
    TodoDataManager* GetDataManager() const {
        return m_pDataManager;
    }

    // 刷新列表
    void RefreshList() {
        if (m_pDataManager) {
            TCHAR szDebug[256];
            _stprintf_s(szDebug, _T("RefreshList: isDoneList=%d, itemCount=%d\n"),
                m_isDoneList, m_pDataManager->GetItemCount(m_isDoneList));
            ::OutputDebugString(szDebug);

            // 重要：对于虚拟列表，必须先创建分组，再设置项目数量
            // 否则分组视图无法正确显示
            if (m_pDataManager->GetItemCount(m_isDoneList) > 0) {
                RefreshGroups();
            } else {
                // 没有项目时，清除所有分组
                RemoveAllGroups();
            }

            // 然后再设置项目数量
            int itemCount = m_pDataManager->GetItemCount(m_isDoneList);
            SetItemCountEx(itemCount, LVSICF_NOSCROLL);

            // 确保启用分组视图
            EnableGroupView(TRUE);

            // 强制刷新列表视图
            RedrawItems(0, -1);
        }
        Invalidate(FALSE);
    }

    // 刷新分组 - 智能更新，只添加新分组，不移除现有分组
    void RefreshGroups() {
        if (!m_pDataManager) {
            ::OutputDebugString(_T("RefreshGroups: m_pDataManager is null!\n"));
            return;
        }

        ::OutputDebugString(_T("RefreshGroups: Start\n"));

        // 获取所有需要的组 ID
        std::vector<int> groupIds = m_pDataManager->GetAllGroupIds(m_isDoneList);

        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("  Required groupIds count=%zu, isDoneList=%d\n"), groupIds.size(), m_isDoneList);
        ::OutputDebugString(szDebug);

        // 使用正确的 API 获取组数量
        int currentGroupCount = ListView_GetGroupCount(m_hWnd);
        _stprintf_s(szDebug, _T("  Current groupCount before cleanup=%d\n"), currentGroupCount);
        ::OutputDebugString(szDebug);

        // 移除不需要的分组（从后往前遍历）
        for (int i = currentGroupCount - 1; i >= 0; i--) {
            int existingGroupId = GetGroupIdByIndex(i);
            bool found = false;
            for (int newGroupId : groupIds) {
                if (existingGroupId == newGroupId) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                _stprintf_s(szDebug, _T("  Removing group: index=%d, id=%d\n"), i, existingGroupId);
                ::OutputDebugString(szDebug);
                RemoveGroup(i);
            }
        }

        // 添加新分组 - 使用 std::set 来跟踪已存在的组，避免重复
        std::set<int> existingGroupSet;
        currentGroupCount = ListView_GetGroupCount(m_hWnd);
        _stprintf_s(szDebug, _T("  Group count after removal=%d, adding new groups...\n"), currentGroupCount);
        ::OutputDebugString(szDebug);

        // 先收集所有已存在的组 ID
        for (int i = 0; i < currentGroupCount; i++) {
            existingGroupSet.insert(GetGroupIdByIndex(i));
        }

        // 为每个需要的组 ID 创建新分组（如果不存在）
        for (int groupId : groupIds) {
            if (existingGroupSet.find(groupId) == existingGroupSet.end()) {
                // 组不存在，需要创建
                LVGROUP group = {0};
                group.cbSize = sizeof(LVGROUP);
                group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE | LVGF_ALIGN;
                group.iGroupId = groupId;

                // 格式化组标题
                int year = groupId / 10000;
                int month = (groupId % 10000) / 100;
                int day = groupId % 100;

                CString strHeader;
                strHeader.Format(_T("%04d/%02d/%02d (%zu项)"), year, month, day,
                    m_pDataManager->GetGroupItemCount(groupId, m_isDoneList));

                group.pszHeader = strHeader.GetBuffer();
                group.state = LVGS_COLLAPSIBLE;
                group.uAlign = LVGA_HEADER_LEFT;

                _stprintf_s(szDebug, _T("  Inserting NEW group: id=%d, header='%s'\n"), groupId, strHeader.GetString());
                ::OutputDebugString(szDebug);

                int result = InsertGroup(-1, &group);
                _stprintf_s(szDebug, _T("  InsertGroup result=%d, error=%d\n"), result, GetLastError());
                ::OutputDebugString(szDebug);

                strHeader.ReleaseBuffer();
            } else {
                _stprintf_s(szDebug, _T("  Group %d already exists, skipping\n"), groupId);
                ::OutputDebugString(szDebug);
            }
        }

        // 设置分组视图样式
        EnableGroupView(TRUE);

        int finalCount = ListView_GetGroupCount(m_hWnd);
        _stprintf_s(szDebug, _T("RefreshGroups: End. Final groupCount=%d\n"), finalCount);
        ::OutputDebugString(szDebug);
    }

    // 获取指定索引的分组ID
    int GetGroupIdByIndex(int index) {
        LVGROUP group = {0};
        group.cbSize = sizeof(LVGROUP);
        group.mask = LVGF_GROUPID;
        BOOL bResult = GetGroupInfo(index, &group);
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

        if (pItemAct->iItem >= 0 && !m_isDoneList) {
            // Todo 列表：点击复选框完成任务
            LVHITTESTINFO hti = {0};
            hti.pt = pItemAct->ptAction;
            SubItemHitTest(&hti);

            if (hti.iItem == pItemAct->iItem && hti.iSubItem == 1) {
                // 点击的是描述列，可以切换完成状态
                NotifyParentCompleteTask(pItemAct->iItem);
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
                const TodoItem* pItem = m_pDataManager->GetItemAt(
                    static_cast<int>(pLVCD->nmcd.dwItemSpec), m_isDoneList);
                if (pItem) {
                    // 根据优先级设置颜色
                    if (m_isDoneList) {
                        // 已完成的任务使用灰色
                        pLVCD->clrText = RGB(100, 100, 100);
                    } else {
                        pLVCD->clrText = pItem->GetPriorityColor();
                    }

                    // 设置选中行的背景色
                    if (pLVCD->nmcd.uItemState & CDIS_SELECTED) {
                        pLVCD->clrTextBk = RGB(200, 220, 240);
                    } else {
                        pLVCD->clrTextBk = RGB(255, 255, 255);
                    }
                }
                return CDRF_DODEFAULT;
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
            const TodoItem* pItem = m_pDataManager->GetItemAt(pDispInfo->item.iItem, m_isDoneList);
            if (pItem) {
                pDispInfo->item.iGroupId = pItem->GetGroupId();

                if (pDispInfo->item.mask & LVIF_TEXT) {
                    switch (pDispInfo->item.iSubItem) {
                        case 0:
                            wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                (LPCTSTR)pItem->GetPriorityString(), _TRUNCATE);
                            break;
                        case 1:
                            wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                pItem->title.c_str(), _TRUNCATE);
                            break;
                        case 2:
                            if (m_isDoneList) {
                                CString str = pItem->GetDoneTimeString();
                                wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                    (LPCTSTR)str, _TRUNCATE);
                            } else {
                                CString str = pItem->GetCreateTimeString();
                                wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                    (LPCTSTR)str, _TRUNCATE);
                            }
                            break;
                        case 3:
                            if (!m_isDoneList) {
                                CString str = pItem->GetEndTimeString();
                                wcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax,
                                    (LPCTSTR)str, _TRUNCATE);
                            }
                            break;
                    }
                }
            }
        }

        return 0;
    }

private:
    TodoDataManager* m_pDataManager;
    bool m_isDoneList;

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
