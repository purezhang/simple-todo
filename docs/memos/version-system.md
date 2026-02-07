# 版本号系统设计备忘

## 版本格式

```
v{MAJOR}.{MINOR}.{PATCH}-{COMMIT_HASH} (Build {YYYY-MM-DD})
```

**示例**: `v1.0.1-abc1234 (Build 2026-02-07)`

## 版本号组成

| 组件 | 来源 | 说明 |
|------|------|------|
| `MAJOR.MINOR.PATCH` | Git Tag | 最近的版本标签，如 `v1.0.1` |
| `COMMIT_HASH` | Git | 当前 HEAD 的短 commit hash (7位) |
| `Build Date` | 系统时间 | 编译时的日期 |

## Git Tag 规范

- 使用语义化版本号: `v1.0.0`, `v1.0.1`, `v1.1.0`, `v2.0.0`
- Tag 必须以 `v` 开头
- 创建 Tag 命令: `git tag -a v1.0.0 -m "Release v1.0.0"`
- 推送 Tag: `git push origin v1.0.0`

## 实现机制

### 1. 构建时自动生成 `version.h`

`build.ps1` 在编译前执行：

```powershell
# 获取版本信息
$gitDescribe = git describe --tags --always 2>$null
$buildDate = Get-Date -Format "yyyy-MM-dd"

# 生成 version.h
@"
#pragma once
#define APP_VERSION_STRING L"$gitDescribe"
#define APP_BUILD_DATE L"$buildDate"
#define APP_VERSION_FULL L"$gitDescribe (Build $buildDate)"
"@ | Out-File -FilePath "src\version.h" -Encoding utf8
```

### 2. 代码中使用

```cpp
#include "version.h"

// 在关于对话框中使用
CString aboutText;
aboutText.Format(_T("Simple Todo %s\n..."), APP_VERSION_FULL);
```

## 无 Tag 时的行为

如果仓库中没有 tag，`git describe --always` 会返回短 commit hash，版本显示为：
```
abc1234 (Build 2026-02-07)
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `src/version.h` | 自动生成，包含版本宏定义，已加入 `.gitignore` |
| `scripts/build.ps1` | 编译脚本，负责生成 `version.h` |

## 注意事项

1. `version.h` 不应提交到 Git（自动生成）
2. 每次编译都会重新生成 `version.h`
3. 发布前应先打 Tag，确保版本号正确
