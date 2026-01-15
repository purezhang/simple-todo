//======================================================================================
// SIMPLIFIED TodoListCtrl::RefreshList() - Group View Removed
//======================================================================================
void RefreshList() {
    if (m_pDataManager) {
        TCHAR szDebug[256];
        _stprintf_s(szDebug, _T("RefreshList: isDoneList=%d, itemCount=%d\n"),
            m_isDoneList, m_pData Manager->GetItemCount(m_isDoneList));
        ::OutputDebugString(szDebug);

        // 【架构简化】完全禁用分组视图
        ListView_EnableGroupView(m_hWnd, FALSE);
        ListView_RemoveAllGroups(m_hWnd);
        ::OutputDebugString(_T("  [SIMPLIFIED MODE] Group view DISABLED\n"));

        // 设置项目数量
        int itemCount = m_pDataManager->GetItemCount(m_isDoneList);
        SetItemCountEx(itemCount, LVSICF_NOSCROLL);
        _stprintf_s(szDebug, _T("  SetItemCountEx=%d\n"), itemCount);
        ::OutputDebugString(szDebug);

        // 强制刷新
        if (itemCount > 0) {
            RedrawItems(0, itemCount - 1);
        }
        InvalidateRect(NULL, FALSE);
        UpdateWindow();
    }
}
