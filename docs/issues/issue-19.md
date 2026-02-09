# Issue-19: 完整国际化（中英文切换）重构

## 问题描述

1. **语言切换功能失效**：`MainFrm.cpp` 中 `OnLanguageChinese/English` 函数只设置了标志位，没有实际切换功能
2. **大量中文硬编码**：代码中散布约30-50处中文字符串（菜单、消息框、列标题、右键菜单等）
3. **语言设置不持久化**：程序重启后语言始终恢复为中文

## 问题表现

- 点击"帮助 → Language → English"菜单后，界面无任何变化
- 除菜单外的所有 UI 文字（列标题、右键菜单、消息框等）始终显示中文
- 程序重启后语言设置丢失

## 解决方案

采用**枚举 + 数组**方式，在单一文件中集中管理所有 UI 字符串：

1. 添加 `StringID` 枚举，定义所有需要国际化的字符串 ID
2. 添加 `g_strings_chinese[]` 和 `g_strings_english[]` 字符串表
3. 添加 `GetString(ID)` 函数获取当前语言对应字符串
4. 添加 `LoadLanguageSetting()`, `SaveLanguageSetting()`, `ApplyLanguage()` 函数
5. 修改注册表操作，保存/读取语言设置
6. 替换所有硬编码字符串为函数调用

## 实现步骤

### 步骤1：添加字符串枚举和声明

在 `MainFrm.h` 中添加：
- `StringID` 枚举（所有需要国际化的字符串 ID）
- `REG_KEY_LANGUAGE` 常量声明
- `LoadLanguageSetting()`, `SaveLanguageSetting()`, `ApplyLanguage()` 声明

### 步骤2：添加字符串表和辅助函数

在 `MainFrm.cpp` 中添加：
- `g_strings_chinese[]` 字符串数组
- `g_strings_english[]` 字符串数组
- `GetString()` 辅助函数
- `FormatString()` 辅助函数（处理格式化字符串）

### 步骤3：实现语言切换核心函数

添加以下函数实现：
- `LoadLanguageSetting()` - 从注册表读取语言设置
- `SaveLanguageSetting()` - 保存语言设置到注册表
- `ApplyLanguage()` - 应用语言设置，刷新菜单、列标题、详情面板等

### 步骤4：修改注册表操作

修改 `LoadWindowSettings()` 和 `SaveWindowSettings()` 函数：
- 添加读取 `Language` 注册表值
- 添加保存 `Language` 注册表值

### 步骤5：修改语言切换命令处理

修改 `OnLanguageChinese()` 和 `OnLanguageEnglish()` 函数：
- 添加保存和刷新调用
- 确保只在状态真正改变时才执行操作

### 步骤6：替换硬编码字符串

替换以下位置的硬编码字符串（约30-50处）：
- `AddTodoDlg.cpp` 中的消息框文字
- `MainFrm.cpp` 中的列标题、右键菜单、消息框、提示文字等

### 步骤7：在 OnCreate 中调用 LoadLanguageSetting()

在窗口创建时加载语言设置。

## 预期效果

1. **菜单切换**：点击语言菜单后，菜单栏立即切换为对应语言
2. **所有 UI 文字切换**：
   - 任务列表列标题（优先级、任务、描述等）
   - 右键菜单（标记完成、编辑、删除等）
   - 消息框提示文字
   - 详情面板文字
3. **持久化**：语言设置保存到注册表，程序重启后保持上次选择
4. **代码结构**：字符串集中管理，便于维护和扩展新语言

## 影响范围

### 修改的文件

| 文件 | 修改内容 |
|------|----------|
| `src/MainFrm.h` | 添加枚举、常量声明、函数声明 |
| `src/MainFrm.cpp` | 添加字符串表、辅助函数、核心逻辑、替换硬编码字符串 |

### 不新增文件

遵循单一文件原则，所有改动在现有 `.h` 和 `.cpp` 文件中完成。

## 备注

### 注册表路径

语言设置将保存在：`HKEY_CURRENT_USER\Software\SimpleTodo\Language`
- 值类型：REG_SZ
- 可能值：`"Chinese"` 或 `"English"`

### 后续扩展

添加新语言步骤：
1. 在 `MainFrm.cpp` 添加新字符串数组（如 `g_strings_japanese[]`）
2. 修改 `GetString()` 函数添加分支
3. 在菜单资源中添加新语言选项

### 字符串清单

需要国际化的字符串分类：
- 通用（5个）：提示、关于标题、全部、是否等
- 任务相关（6个）：标题必填、任务标题、描述等
- 右键菜单（5个）：标记完成、编辑、删除等
- 菜单（6个）：帮助、语言、关于等
- 列标题（6个）：优先级、任务、描述、项目等
- 导出说明（7个）：导出成功、格式说明等

总计约 35 个字符串 ID。
