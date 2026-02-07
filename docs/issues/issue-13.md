# Issue-13: 数据模型层 - Project 分类字段

## 需求描述

为 TodoItem 添加 project 分类字段，修改数据库 schema，支持分类数据持久化。

## 实现步骤

### 步骤1：修改数据模型

**`src/TodoModel.h`**
```cpp
struct TodoItem {
    // ... 现有字段 ...
    std::wstring project;  // 新增：分类名称
};
```

### 步骤2：修改数据库

**`src/SQLiteManager.cpp`**

1. 更新 `CREATE TABLE`:
```cpp
const char* sql =
    "CREATE TABLE IF NOT EXISTS todos ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "priority INTEGER,"
    "title TEXT,"
    "note TEXT,"
    "project TEXT,"           // 新增
    "create_time INTEGER,"
    "target_end_time INTEGER,"
    "actual_done_time INTEGER,"
    "is_done INTEGER);"
```

2. 更新所有 CRUD 方法：
   - `SaveItem()` - 保存 project 字段
   - `LoadItems()` - 读取 project 字段
   - `UpdateItem()` - 更新 project 字段
   - `DeleteItem()` - 无需修改

3. 数据库迁移：
   - 检测 project 列是否存在
   - 不存在则添加

### 步骤3：修改导入导出

**`src/MainFrm.cpp`**
- `ExportToTxt()`: 导出时包含 `+project` 格式
- `ImportFromTxt()`: 解析 `+project` 并保存

## 预期效果

1. 数据库支持 project 字段
2. 任务创建/编辑时支持设置分类
3. 导入导出保留分类信息

## 修改的文件

| 文件 | 修改内容 |
|------|---------|
| `src/TodoModel.h` | TodoItem 添加 project 字段 |
| `src/SQLiteManager.cpp` | 数据库 schema 和 CRUD 操作 |
| `src/MainFrm.cpp` | 导入导出逻辑 |

## 备注

- 本 Issue 仅完成数据层
- UI 显示在 Issue-14/15 实现
