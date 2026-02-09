# Issue-18: 截止时间和优先级颜色显示

## 问题描述

需要在任务列表中显示截止时间过期和优先级标记：
1. **截止时间大于当前时间**：截止时间列显示红色（表示紧急/即将过期）
2. **P0 优先级**：P0 字符显示红色

## 解决方案

修改 CTodoListCtrl 的自定义绘制逻辑（OnCustomDraw），针对特定列设置不同的文字颜色：
- 截止时间列（第3列）过期 → 红色
- P0 优先级（第1列） → 红色

## 实现步骤

### 步骤1：修改截止时间列颜色

在 CDDS_SUBITEM | CDDS_ITEMPREPAINT 阶段处理第3列（截止时间列）：
```cpp
case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
    const TodoItem* pItem = m_pDataManager->GetItemAt(
        static_cast<int>(pLVCD->nmcd.dwItemSpec), m_isDoneList);

    if (pItem && !m_isDoneList && pLVCD->iSubItem == 3) {
        // 第3列：截止时间
        if (pItem->endTime.GetTime() > 0) {
            CTime currentTime = CTime::GetCurrentTime();
            if (pItem->endTime < currentTime) {
                pLVCD->clrText = RGB(255, 0, 0);  // 截止时间过期，红色
            }
        }
    }
    return CDRF_NOTIFYSUBITEMDRAW;
}
```

### 步骤2：修改 P0 优先级颜色

在 CDDS_ITEMPREPAINT 阶段检查 P0 优先级：
```cpp
case CDDS_ITEMPREPAINT: {
    // 如果被选中，使用系统默认绘制
    if (pLVCD->nmcd.uItemState & CDIS_SELECTED) {
        return CDRF_DODEFAULT;
    }

    const TodoItem* pItem = m_pDataManager->GetItemAt(
        static_cast<int>(pLVCD->nmcd.dwItemSpec), m_isDoneList);

    if (pItem) {
        if (m_isDoneList) {
            pLVCD->clrText = RGB(128, 128, 128);
        } else {
            // P0 优先级：红色
            if (pItem->priority == Priority::P0) {
                pLVCD->clrText = RGB(255, 0, 0);
            }
            // P1 优先级：深橙
            else if (pItem->priority == Priority::P1) {
                pLVCD->clrText = RGB(180, 100, 0);
            }
            // P3 优先级：灰色
            else if (pItem->priority == Priority::P3) {
                pLVCD->clrText = RGB(100, 100, 100);
            }
            // P2 优先级：黑色
            else {
                pLVCD->clrText = RGB(0, 0, 0);
            }
        }
    }
    return CDRF_DODEFAULT;
}
```

## 预期效果

- **截止时间列**（第3列）：截止时间过期时显示红色
- **P0 优先级**：P0 字符显示红色（P1/P2/P3 按原逻辑）
- 已完成的任务保持灰色
- 选中项目使用系统默认颜色

## 备注

- 截止时间判断：使用 `pItem->endTime < CTime::GetCurrentTime()`
- 优先级颜色只影响第1列（优先级列）
- 已完成的列表不应用这些颜色规则
