#pragma once
#include "stdafx.h"

// SQLite 数据库管理类
class CSQLiteManager
{
public:
    CSQLiteManager();
    ~CSQLiteManager();

    // 初始化数据库
    bool Initialize();

    // 加载所有数据
    bool LoadAll(TodoDataManager& manager);

    // 保存所有数据
    bool SaveAll(const TodoDataManager& manager);

    // 保存单个 Todo
    bool SaveTodo(const TodoItem& item);

    // 更新 Todo
    bool UpdateTodo(const TodoItem& item);

    // 删除 Todo
    bool DeleteTodo(UINT id);

    // 移动 Todo（从 Todo 到 Done 或反之）
    bool MoveTodo(UINT id, bool isDone);

private:
    sqlite3* m_db;
    std::wstring m_dbPath;

    // 创建表
    bool CreateTables();

    // 加载条目
    bool LoadItems(std::vector<TodoItem>& items, bool isDone);

    // 执行 SQL 语句
    bool ExecuteSQL(const char* sql);
};
