# Issue-11: 实现任务分类（Project）功能

## 需求描述

为任务添加分类（Project）字段，支持：
1. 创建/编辑任务时设置分类
2. 按分类筛选和分组显示任务
3. 分类列表管理（添加、删除、重命名）
4. 导入导出时保留分类信息

## 调研结果

### 当前数据结构

**`TodoItem` 结构体** (`src/TodoModel.h`)
```cpp
struct TodoItem {
    UINT id;
    Priority priority;
    std::wstring title;
    std::wstring note;
    CTime createTime;
    CTime targetEndTime;
    CTime actualDoneTime;
    bool isDone;
    bool isPinned;
    // ❌ 没有 project 字段
};
```

**SQLite 数据库 schema** (`src/SQLiteManager.cpp`)
```cpp
const char* sql =
    "CREATE TABLE IF NOT EXISTS todos ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "priority INTEGER,"
    "title TEXT,"
    "note TEXT,"
    "create_time INTEGER,"
    "target_end_time INTEGER,"
    "actual_done_time INTEGER,"
    "is_done INTEGER);"
    // ❌ 没有 project 列
```

**导出格式文档注释** (`src/MainFrm.cpp:1024`)
```cpp
fwprintf_s(fp, L"# Format: (A-Z) YYYY-MM-DD Task description +project @context\n");
```
- ⚠️ 仅有文档说明，实际数据中没有 project 字段

### 结论

**无法复用现有字段**，需要新增 `project` 字段。

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

2. 更新所有 Save/Load 方法：
   - `SaveItem()`
   - `LoadItems()`
   - `UpdateItem()`
   - `DeleteItem()`

### 步骤3：修改导入导出

**`src/MainFrm.cpp`**
- `ExportToTxt()`: 导出时包含 `+project` 格式
- `ImportFromTxt()`: 解析 `+project` 并保存

### 步骤4：修改任务对话框

**`src/AddTodoDlg.h` / `AddTodoDlg.cpp`**

1. 添加分类选择/输入控件
2. 添加分类管理功能（新增、删除分类）

### 步骤5：修改主窗口 UI

**`src/MainFrm.h` / `src/MainFrm.cpp`**

1. 添加分类筛选下拉框（工具栏或列表头部）
2. 添加分类分组显示功能（可选）
3. 更新详情面板显示分类信息

### 步骤6：数据迁移

首次运行时，将现有任务的项目字段设置为默认值（如空字符串或"默认"）。

## 预期效果

1. **添加任务时**：可以输入或选择分类
2. **任务列表**：显示分类标签/图标
3. **筛选功能**：支持下拉框按分类筛选
4. **导入导出**：保留分类信息（`+project` 格式）
5. **分类管理**：支持添加、删除、重命名分类

## 修改的文件

| 文件 | 修改内容 |
|------|---------|
| `src/TodoModel.h` | `TodoItem` 添加 `project` 字段 |
| `src/SQLiteManager.cpp` | 数据库 schema 和 CRUD 操作添加 project |
| `src/AddTodoDlg.h/cpp` | 添加分类编辑和管理 UI |
| `src/MainFrm.h/cpp` | 筛选 UI、详情面板显示 |
| `src/TodoListCtrl.h/cpp` | 显示分类标签 |
| `docs/issues/issue-11.md` | 本文档 |

## 备注

- 参考 todo.txt 的 `+project` 和 `@context` 格式
- 分类名称建议：字符串，允许多级分类（如 `工作/项目A`）
- 分类列表可存储在单独配置或数据库中
