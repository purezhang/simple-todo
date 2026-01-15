// atlres.h - 简化版 WTL 资源定义
// 用于替代完整的 WTL atlres.h

#pragma once

// 编辑器资源值
#define IDC_EDITBAR                       100
#define ID_STATUS_BAR                     101

// 工具栏命令
#define ID_VIEW_TOOLBAR                   1000
#define ID_VIEW_STATUS_BAR                1001

// 其他常用 ID
#define ID_SEPARATOR                      0
#define IDW_FIRST_CHILD                   1000

// 字体支持
#ifndef RC_INVOKED
#ifndef _WIN32_WCE
#include <afxres.h>
#else
#include <aygshell.h>
#endif
#endif

// 确保 RICHEDIT 已定义
#ifndef __RICHEDIT_H__
DECLARE_HANDLE(HRICHEDIT);
#endif
