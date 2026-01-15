#pragma once

// 以下宏定义要求的最低平台。要求的最低平台
// 是具有该功能的应用程序可运行的 Windows、Internet Explorer 等产品的
// 最早版本。通过在指定版本及更低版本上启用所有可用的功能，此宏
// 可以作用于该版本及更高版本上。

// 如果需要针对以下平台之前的平台指定应用程序，请修改下面的定义。
// 有关不同平台对应值的更多信息，请参阅 MSDN。
#ifndef WINVER              // 允许使用特定于 Windows XP 或更高版本的功能。
#define WINVER 0x0A00       // 将此值更改为相应的 Windows 版本值。
#endif

#ifndef _WIN32_WINNT        // 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0A00 // 将此值更改为相应的 Windows 版本值。
#endif

#ifndef _WIN32_WINDOWS      // 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0A00 // 将此值更改为适当的 Windows 版本值。
#endif

#ifndef _WIN32_IE           // 允许使用特定于 IE 6.0 或更高版本的功能。
#define _WIN32_IE 0x0A00    // 将此值更改为适当的 IE 版本值。
#endif
