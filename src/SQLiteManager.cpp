#include "stdafx.h"
#include "SQLiteManager.h"

CSQLiteManager::CSQLiteManager()
    : m_db(nullptr)
{
}

CSQLiteManager::~CSQLiteManager()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool CSQLiteManager::Initialize()
{
    // 获取可执行文件路径
    wchar_t szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    std::wstring path(szPath);
    size_t pos = path.find_last_of(L"\\/");
    path = path.substr(0, pos);
    path += L"\\data";

    // 创建 data 目录
    CreateDirectory(path.c_str(), NULL);
    path += L"\\simpletodo.db";

    m_dbPath = path;

    int rc = sqlite3_open16(m_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        return false;
    }

    // 设置数据库编码为 UTF-16
    sqlite3_exec(m_db, "PRAGMA encoding = 'UTF-16';", nullptr, nullptr, nullptr);

    if (!CreateTables()) {
        return false;
    }

    // 尝试添加 is_pinned 列 (如果不存在)
    sqlite3_exec(m_db, "ALTER TABLE todos ADD COLUMN is_pinned INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);

    // 尝试添加 project 列 (如果不存在)
    sqlite3_exec(m_db, "ALTER TABLE todos ADD COLUMN project TEXT DEFAULT '';", nullptr, nullptr, nullptr);

    return true;
}

bool CSQLiteManager::CreateTables()
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS todos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "priority INTEGER,"
        "title TEXT,"
        "note TEXT,"
        "project TEXT,"
        "create_time INTEGER,"
        "target_end_time INTEGER,"
        "actual_done_time INTEGER,"
        "is_done INTEGER,"
        "is_pinned INTEGER);";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }

    return true;
}

bool CSQLiteManager::LoadAll(TodoDataManager& manager)
{
    manager.Clear();
    LoadItems(manager.todoItems, false);
    LoadItems(manager.doneItems, true);

    // 更新 nextId
    if (!manager.todoItems.empty() || !manager.doneItems.empty()) {
        UINT maxId = 0;
        for (const auto& item : manager.todoItems) {
            if (item.id > maxId) maxId = item.id;
        }
        for (const auto& item : manager.doneItems) {
            if (item.id > maxId) maxId = item.id;
        }
        manager.nextId = maxId + 1;
    }

    return true;
}

bool CSQLiteManager::LoadItems(std::vector<TodoItem>& items, bool isDone)
{
#ifdef _DEBUG
    TCHAR szDebug[256];
    _stprintf_s(szDebug, _T("LoadItems: isDone=%d, start loading...\n"), isDone);
    ::OutputDebugString(szDebug);
#endif

    const char* sql = isDone ?
        "SELECT id, priority, title, note, project, create_time, target_end_time, actual_done_time, is_pinned FROM todos WHERE is_done=1 ORDER BY create_time DESC;" :
        "SELECT id, priority, title, note, project, create_time, target_end_time, actual_done_time, is_pinned FROM todos WHERE is_done=0 ORDER BY create_time DESC;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
#ifdef _DEBUG
        _stprintf_s(szDebug, _T("LoadItems: sqlite3_prepare failed, rc=%d\n"), rc);
        ::OutputDebugString(szDebug);
#endif
        return false;
    }

    int itemCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TodoItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.priority = (Priority)sqlite3_column_int(stmt, 1);

        const void* pText = sqlite3_column_text16(stmt, 2);
        if (pText) item.title = (const wchar_t*)pText;

        pText = sqlite3_column_text16(stmt, 3);
        if (pText) item.note = (const wchar_t*)pText;

        // 读取 project
        pText = sqlite3_column_text16(stmt, 4);
        if (pText) item.project = (const wchar_t*)pText;

        sqlite3_int64 createTime = sqlite3_column_int64(stmt, 5);
        item.createTime = CTime(createTime);

        sqlite3_int64 endTime = sqlite3_column_int64(stmt, 6);
        if (endTime > 0) {
            item.targetEndTime = CTime(endTime);
        } else {
            item.targetEndTime = CTime::GetCurrentTime();
        }

        sqlite3_int64 doneTime = sqlite3_column_int64(stmt, 7);
        if (doneTime > 0) {
            item.actualDoneTime = CTime(doneTime);
        }

        // 读取 is_pinned
        item.isPinned = (sqlite3_column_int(stmt, 8) != 0);

        item.isDone = isDone;
        items.push_back(item);
        itemCount++;

#ifdef _DEBUG
        _stprintf_s(szDebug, _T("  Loaded item: id=%d, title='%s'\n"), item.id, item.title.c_str());
        ::OutputDebugString(szDebug);
#endif
    }

    sqlite3_finalize(stmt);

#ifdef _DEBUG
    _stprintf_s(szDebug, _T("LoadItems: Completed. Loaded %d items\n"), itemCount);
    ::OutputDebugString(szDebug);
#endif
    return true;
}

bool CSQLiteManager::SaveAll(const TodoDataManager& manager)
{
    // 先清空表
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, "DELETE FROM todos;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    // 保存 Todo 项
    for (const auto& item : manager.todoItems) {
        if (!SaveTodo(item)) return false;
    }

    // 保存 Done 项
    for (const auto& item : manager.doneItems) {
        if (!SaveTodo(item)) return false;
    }

    return true;
}

bool CSQLiteManager::SaveTodo(const TodoItem& item)
{
    const char* sql =
        "INSERT INTO todos (priority, title, note, project, create_time, target_end_time, actual_done_time, is_done, is_pinned) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    // 绑定参数
    sqlite3_bind_int(stmt, 1, (int)item.priority);

    std::wstring title = item.title;
    sqlite3_bind_text16(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);

    std::wstring note = item.note;
    sqlite3_bind_text16(stmt, 3, note.c_str(), -1, SQLITE_TRANSIENT);

    std::wstring project = item.project;
    sqlite3_bind_text16(stmt, 4, project.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int64(stmt, 5, item.createTime.GetTime());
    sqlite3_bind_int64(stmt, 6, item.targetEndTime.GetTime());

    // 对于未完成的任务，actualDoneTime 为 0 (表示 NULL)
    // 使用 SQLITE_NULL 来处理未完成的任务
    if (item.isDone && item.actualDoneTime.GetTime() > 0) {
        sqlite3_bind_int64(stmt, 7, item.actualDoneTime.GetTime());
    } else {
        sqlite3_bind_null(stmt, 7);
    }

    sqlite3_bind_int(stmt, 8, item.isDone ? 1 : 0);
    sqlite3_bind_int(stmt, 9, item.isPinned ? 1 : 0);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool CSQLiteManager::UpdateTodo(const TodoItem& item)
{
    const char* sql =
        "UPDATE todos SET priority=?, title=?, note=?, project=?, target_end_time=?, actual_done_time=?, is_done=?, is_pinned=? "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    // 绑定参数
    sqlite3_bind_int(stmt, 1, (int)item.priority);

    std::wstring title = item.title;
    sqlite3_bind_text16(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);

    std::wstring note = item.note;
    sqlite3_bind_text16(stmt, 3, note.c_str(), -1, SQLITE_TRANSIENT);

    std::wstring project = item.project;
    sqlite3_bind_text16(stmt, 4, project.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int64(stmt, 5, item.targetEndTime.GetTime());
    sqlite3_bind_int64(stmt, 6, item.actualDoneTime.GetTime());
    sqlite3_bind_int(stmt, 7, item.isDone ? 1 : 0);
    sqlite3_bind_int(stmt, 8, item.isPinned ? 1 : 0);
    sqlite3_bind_int(stmt, 9, item.id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool CSQLiteManager::DeleteTodo(UINT id)
{
    const char* sql = "DELETE FROM todos WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool CSQLiteManager::MoveTodo(UINT id, bool isDone)
{
    const char* sql = "UPDATE todos SET is_done=?, actual_done_time=? WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    CTime now = CTime::GetCurrentTime();
    sqlite3_bind_int(stmt, 1, isDone ? 1 : 0);
    sqlite3_bind_int64(stmt, 2, now.GetTime());
    sqlite3_bind_int(stmt, 3, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool CSQLiteManager::MarkDone(UINT id)
{
    return MoveTodo(id, true);
}
