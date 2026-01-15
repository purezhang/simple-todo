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
        // 【调试】临时禁用自定义绘制，排除颜色问题
        // REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
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
    // 刷新列表 - 直接插入真实项目，不使用虚拟列表
    void RefreshList() {
        if (m_pDataManager) {
            TODO_DEBUG_LOGF(_T("RefreshList: isDoneList=%d, itemCount=%d\n"),
                m_isDoneList, m_pDataManager->GetItemCount(m_isDoneList));

            // 排序数据 (新增)
            // 规则：1. 日期(降序) 2. 优先级(升序) 3. 截止时间(升序)
            m_pDataManager->Sort(m_isDoneList);

            // 清空现有项目
            DeleteAllItems();
            
            // 确保禁用分组
            ListView_EnableGroupView(m_hWnd, FALSE);
            ListView_RemoveAllGroups(m_hWnd);

            // 直接插入真实项目
            int itemCount = m_pDataManager->GetItemCount(m_isDoneList);
            for (int i = 0; i < itemCount; i++) {
                const TodoItem* pItem = m_pDataManager->GetItemAt(i, m_isDoneList);
                if (pItem) {
                    // 插入项目
                    // 准备数据
                    CString strDate = pItem->createTime.Format(_T("%Y/%m/%d"));
                    CString strPriority = pItem->GetPriorityString();
                    CString strTitle(pItem->title.c_str());
                    CString strTime = m_isDoneList ? pItem->GetDoneTimeString() : pItem->GetCreateTimeString();

                    // 插入项目
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = i;
                    
                    if (!m_isDoneList) {
                        // Todo 列表：[0]日期 [1]优先级 [2]标题 [3]时间 [4]截止
                        lvi.iSubItem = 0;
                        lvi.pszText = (LPTSTR)(LPCTSTR)strDate;
                        int idx = InsertItem(&lvi);

                        SetItemText(idx, 1, (LPTSTR)(LPCTSTR)strPriority);
                        SetItemText(idx, 2, (LPTSTR)(LPCTSTR)strTitle);
                        SetItemText(idx, 3, (LPTSTR)(LPCTSTR)strTime);
                        
                        CString strEndTime = pItem->GetEndTimeString();
                        SetItemText(idx, 4, (LPTSTR)(LPCTSTR)strEndTime);
                    } 
                    else {
                        // Done 列表：保持原样 [0]优先级 [1]标题 [2]完成时间
                        lvi.iSubItem = 0;
                        lvi.pszText = (LPTSTR)(LPCTSTR)strPriority;
                        int idx = InsertItem(&lvi);

                        SetItemText(idx, 1, (LPTSTR)(LPCTSTR)strTitle);
                        SetItemText(idx, 2, (LPTSTR)(LPCTSTR)strTime);
                    }
                    
                    // 仅在 Debug 模式记录详细插入日志
                    TODO_DEBUG_LOGF(_T("  Inserted item %d: %s\n"), i, pItem->title.c_str());
                }
            }

            TODO_DEBUG_LOGF(_T("RefreshList complete: inserted %d items\n"), itemCount);
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
                                strValue = pItem->GetPriorityString();
                                break;
                            case 1:
                                strValue = pItem->title.c_str();
                                break;
                            case 2:
                                if (m_isDoneList) {
                                    strValue = pItem->GetDoneTimeString();
                                } else {
                                    strValue = pItem->GetCreateTimeString();
                                }
                                break;
                            case 3:
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
