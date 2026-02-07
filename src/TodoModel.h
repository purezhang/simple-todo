#pragma once
#include <vector>
#include <string>
#include <algorithm>

enum class Priority { P0 = 0, P1, P2, P3 };

struct TodoItem {
    UINT id;
    Priority priority;
    std::wstring title;
    std::wstring note;
    std::wstring project;      // 分类名称
    CTime createTime;
    CTime targetEndTime;
    CTime actualDoneTime;
    bool isDone = false;
    bool isPinned = false; // 置顶状态

    // 获取格式化的日期字符串，用于分组依据
    std::wstring GetGroupKey() const {
        struct tm tmTime;
        createTime.GetLocalTm(&tmTime);
        wchar_t buffer[32];
        swprintf_s(buffer, L"%04d%02d%02d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);
        return std::wstring(buffer);
    }

    // 获取分组ID，用于排序和分组显示
    int GetGroupId() const {
        struct tm tmTime;
        createTime.GetLocalTm(&tmTime);
        return (tmTime.tm_year + 1900) * 10000 +
               (tmTime.tm_mon + 1) * 100 +
               tmTime.tm_mday;
    }

    COLORREF GetPriorityColor() const {
        switch (priority) {
            case Priority::P0: return RGB(220, 53, 69);
            case Priority::P1: return RGB(255, 193, 7);
            case Priority::P2: return RGB(40, 167, 69);
            default: return RGB(108, 117, 125);
        }
    }

    CString GetPriorityString() const {
        switch (priority) {
            case Priority::P0: return _T("P0");
            case Priority::P1: return _T("P1");
            case Priority::P2: return _T("P2");
            default: return _T("P3");
        }
    }

    CString GetCreateTimeString() const {
        return createTime.Format(_T("%Y/%m/%d %H:%M"));
    }

    CString GetEndTimeString() const {
        return targetEndTime.Format(_T("%Y/%m/%d %H:%M"));
    }

    CString GetDoneTimeString() const {
        return actualDoneTime.Format(_T("%Y/%m/%d %H:%M"));
    }
};

class TodoDataManager {
public:
    std::vector<TodoItem> todoItems;
    std::vector<TodoItem> doneItems;
    UINT nextId = 1;

    void AddTodo(const TodoItem& item) {
        TodoItem newItem = item;
        newItem.id = nextId++;
        newItem.isDone = false;
        if (newItem.createTime == CTime()) {
            newItem.createTime = CTime::GetCurrentTime();
        }
        todoItems.push_back(newItem);
    }

    bool CompleteTodo(UINT id) {
        auto it = std::find_if(todoItems.begin(), todoItems.end(),
            [id](const TodoItem& item) { return item.id == id; });
        if (it != todoItems.end()) {
            it->isDone = true;
            it->actualDoneTime = CTime::GetCurrentTime();
            doneItems.push_back(*it);
            todoItems.erase(it);
            return true;
        }
        return false;
    }

    bool DeleteTodo(UINT id, bool isDoneList) {
#ifdef _DEBUG
    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("TodoDataManager::DeleteTodo: id=%d, isDoneList=%d\n"), id, isDoneList);
    ::OutputDebugString(szDebug);
    
    auto& items = isDoneList ? doneItems : todoItems;
    _stprintf_s(szDebug, _T("TodoDataManager::DeleteTodo: items.size()=%zu\n"), items.size());
    ::OutputDebugString(szDebug);
#else
    auto& items = isDoneList ? doneItems : todoItems;
#endif
    
    auto it = std::find_if(items.begin(), items.end(),
        [id](const TodoItem& item) { return item.id == id; });
    if (it != items.end()) {
#ifdef _DEBUG
        _stprintf_s(szDebug, _T("TodoDataManager::DeleteTodo: Found item at index=%zu, title='%s'\n"), 
            std::distance(items.begin(), it), it->title.c_str());
        ::OutputDebugString(szDebug);
#endif
        
        items.erase(it);
#ifdef _DEBUG
        _stprintf_s(szDebug, _T("TodoDataManager::DeleteTodo: After erase, items.size()=%zu\n"), items.size());
        ::OutputDebugString(szDebug);
#endif
        
        return true;
    }
#ifdef _DEBUG
    _stprintf_s(szDebug, _T("TodoDataManager::DeleteTodo: Item not found\n"));
    ::OutputDebugString(szDebug);
#endif
    return false;
}

    bool ChangePriority(UINT id, Priority newPriority, bool isDoneList) {
        auto& items = isDoneList ? doneItems : todoItems;
        auto it = std::find_if(items.begin(), items.end(),
            [id](const TodoItem& item) { return item.id == id; });
        if (it != items.end()) {
            it->priority = newPriority;
            return true;
        }
        return false;
    }

    const TodoItem* GetItemAt(int index, bool isDoneList) const {
        const auto& items = isDoneList ? doneItems : todoItems;
        if (index >= 0 && index < static_cast<int>(items.size())) {
            return &items[index];
        }
        return nullptr;
    }

    int GetItemCount(bool isDoneList) const {
        return static_cast<int>(isDoneList ? doneItems.size() : todoItems.size());
    }

    std::vector<int> GetAllGroupIds(bool isDoneList) const {
        std::set<int> groupIds;
        const auto& items = isDoneList ? doneItems : todoItems;
        for (const auto& item : items) {
            groupIds.insert(item.GetGroupId());
        }
        return std::vector<int>(groupIds.rbegin(), groupIds.rend());
    }

    size_t GetGroupItemCount(int groupId, bool isDoneList) const {
        const auto& items = isDoneList ? doneItems : todoItems;
        return std::count_if(items.begin(), items.end(),
            [groupId](const TodoItem& item) { return item.GetGroupId() == groupId; });
    }

    void Sort(bool isDoneList) {
        auto& items = isDoneList ? doneItems : todoItems;
        std::sort(items.begin(), items.end(), [isDoneList](const TodoItem& a, const TodoItem& b) {
            // 0. 置顶排序 (仅 Todo 列表)
            if (!isDoneList) {
                if (a.isPinned != b.isPinned) {
                    return a.isPinned > b.isPinned; // true (1) > false (0) -> pinned first
                }
            }

            // 1. 日期排序
            int dateA = a.GetGroupId();
            int dateB = b.GetGroupId();
            
            if (dateA != dateB) {
                if (!isDoneList) {
                    // Todo列表：最早的排前面 (升序)
                    return dateA < dateB;
                } else {
                    // Done列表：最新的排前面 (降序 - 保持原有逻辑)
                    return dateA > dateB;
                }
            }

            // 2. 优先级排序 (P0在前 - 升序)
            if (a.priority != b.priority) {
                return a.priority < b.priority;
            }

            // 3. 第三级排序
            if (!isDoneList) {
                // Todo列表：按截止时间排序 (早的在前)
                LONGLONG tA = a.targetEndTime.GetTime();
                LONGLONG tB = b.targetEndTime.GetTime();
                if (tA <= 0) tA = _I64_MAX;
                if (tB <= 0) tB = _I64_MAX;
                return tA < tB;
            } else {
                // Done列表：按完成时间排序 (晚/最近完成的在前?)
                // 或者保持原逻辑按截止时间? 用户只提了Todo栏。
                // 暂时保持原逻辑：按截止时间
                LONGLONG tA = a.targetEndTime.GetTime();
                LONGLONG tB = b.targetEndTime.GetTime();
                if (tA <= 0) tA = _I64_MAX;
                if (tB <= 0) tB = _I64_MAX;
                return tA < tB;
            }
        });
    }

    bool UpdateTodo(const TodoItem& updatedItem, bool isDoneList) {
        auto& items = isDoneList ? doneItems : todoItems;
        auto it = std::find_if(items.begin(), items.end(),
            [id = updatedItem.id](const TodoItem& item) { return item.id == id; });
        if (it != items.end()) {
            // 更新任务数据（除了ID和创建时间）
            it->priority = updatedItem.priority;
            it->title = updatedItem.title;
            it->note = updatedItem.note;
            it->project = updatedItem.project;
            it->targetEndTime = updatedItem.targetEndTime;
            return true;
        }
        return false;
    }

    void Clear() {
        todoItems.clear();
        doneItems.clear();
        nextId = 1;
    }

    // 搜索过滤（支持关键词、项目筛选和时间筛选）
    // timeFilter: 0=全部, 1=今天, 2=本周
    std::vector<int> Search(const std::wstring& keyword, const std::wstring& project, bool isDoneList, int timeFilter = 0) const {
        const auto& items = isDoneList ? doneItems : todoItems;
#ifdef _DEBUG
        TCHAR szDebug[512];
        _stprintf_s(szDebug, _T("[搜索] START: keyword='%s', project='%s', timeFilter=%d, total=%zu\n"),
            keyword.c_str(), project.c_str(), timeFilter, items.size());
        ::OutputDebugString(szDebug);
#endif

        std::vector<int> indices;

        // 无筛选条件，返回所有
        if (keyword.empty() && project.empty() && timeFilter == 0) {
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                indices.push_back(i);
            }
#ifdef _DEBUG
            _stprintf_s(szDebug, _T("[搜索] END (无筛选): 返回 %zu 条\n"), indices.size());
            ::OutputDebugString(szDebug);
#endif
            return indices;
        }

        // 有筛选条件，逐条检查
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const auto& item = items[i];
            bool failed = false;
            CString failReason;

            // 时间筛选
            if (timeFilter != 0) {
                // 根据任务类型选择正确的时间字段：已完成任务使用实际完成时间，待办任务使用创建时间
                CTime itemDate;
                if (isDoneList && item.actualDoneTime.GetTime() > 0) {
                    // 已完成列表：使用实际完成时间进行筛选
                    itemDate = item.actualDoneTime;
                } else {
                    // 待办列表 或 actualDoneTime 无效：使用创建时间
                    itemDate = item.createTime;
                }

                CTime now = CTime::GetCurrentTime();
                CTime todayStart(now.GetYear(), now.GetMonth(), now.GetDay(), 0, 0, 0);
                CTime weekStart = todayStart - CTimeSpan(now.GetDayOfWeek() - 1, 0, 0, 0);

                bool timeMatch = false;
                if (timeFilter == 1) {  // 今天
                    timeMatch = (itemDate.GetYear() == todayStart.GetYear() &&
                                  itemDate.GetMonth() == todayStart.GetMonth() &&
                                  itemDate.GetDay() == todayStart.GetDay());
                    if (!timeMatch) {
                        failed = true;
                        failReason.Format(_T("时间[今天]: %s != %s"), itemDate.Format(_T("%Y/%m/%d")), todayStart.Format(_T("%Y/%m/%d")));
                    }
                } else if (timeFilter == 2) {  // 本周
                    timeMatch = (itemDate >= weekStart);
                    if (!timeMatch) {
                        failed = true;
                        failReason.Format(_T("时间[本周]: %s < %s"), itemDate.Format(_T("%Y/%m/%d")), weekStart.Format(_T("%Y/%m/%d")));
                    }
                }
            }

            // 项目筛选
            if (!failed && !project.empty() && item.project != project) {
                failed = true;
                failReason.Format(_T("项目: '%s' != '%s'"), item.project.c_str(), project.c_str());
            }

            // 关键词筛选
            if (!failed && !keyword.empty()) {
                std::wstring lowerKeyword = keyword;
                std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
                std::wstring lowerTitle = item.title;
                std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
                if (lowerTitle.find(lowerKeyword) == std::wstring::npos) {
                    failed = true;
                    failReason = _T("关键词不匹配");
                }
            }

            // 输出每条记录的检查结果
#ifdef _DEBUG
            if (failed) {
                _stprintf_s(szDebug, _T("[搜索] SKIP [%d]: %s - %s\n"), i, item.title.c_str(), (LPCTSTR)failReason);
            } else {
                _stprintf_s(szDebug, _T("[搜索] PASS [%d]: %s\n"), i, item.title.c_str());
            }
            ::OutputDebugString(szDebug);
#endif
            if (!failed) {
                indices.push_back(i);
            }
        }

#ifdef _DEBUG
        _stprintf_s(szDebug, _T("[搜索] END: 通过 %zu / %zu 条\n"), indices.size(), items.size());
        ::OutputDebugString(szDebug);
#endif
        return indices;
    }
};