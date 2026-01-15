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
    CTime createTime;
    CTime targetEndTime;
    CTime actualDoneTime;
    bool isDone = false;

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
        auto& items = isDoneList ? doneItems : todoItems;
        auto it = std::find_if(items.begin(), items.end(),
            [id](const TodoItem& item) { return item.id == id; });
        if (it != items.end()) {
            items.erase(it);
            return true;
        }
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

    void Clear() {
        todoItems.clear();
        doneItems.clear();
        nextId = 1;
    }
};