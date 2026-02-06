// SimpleTodo.cpp : main source file for SimpleTodo.exe
//

#include "stdafx.h"
#include "MainFrm.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpstrCmdLine);

    // 启用高DPI感知
    // 方式1: 使用SetProcessDpiAwareness (Windows 10 Anniversary Update及以后版本)
    #if defined(_MSC_VER) && (_MSC_VER >= 1900) // VS2015+
    typedef enum PROCESS_DPI_AWARENESS {
        PROCESS_DPI_UNAWARE = 0,
        PROCESS_DPI_SYSTEM_AWARE = 1,
        PROCESS_DPI_PER_MONITOR_AWARE = 2
    } PROCESS_DPI_AWARENESS;

    typedef HRESULT(WINAPI* LPSETPROCESSDPIAWARENESS)(PROCESS_DPI_AWARENESS);
    typedef BOOL(WINAPI* LPSETPROCESSDPIAWARE)();

    HMODULE hShcore = LoadLibrary(L"Shcore.dll");
    if (hShcore) {
        LPSETPROCESSDPIAWARENESS pSetProcessDpiAwareness =
            (LPSETPROCESSDPIAWARENESS)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSetProcessDpiAwareness) {
            pSetProcessDpiAwareness(PROCESS_DPI_PER_MONITOR_AWARE);
        }
        FreeLibrary(hShcore);
    } else {
        // 回退到传统方式 (Windows 8.1及更早版本)
        HMODULE hUser32 = LoadLibrary(L"user32.dll");
        if (hUser32) {
            LPSETPROCESSDPIAWARE pSetProcessDPIAware =
                (LPSETPROCESSDPIAWARE)GetProcAddress(hUser32, "SetProcessDPIAware");
            if (pSetProcessDPIAware) {
                pSetProcessDPIAware();
            }
            FreeLibrary(hUser32);
        }
    }
    #endif

    HRESULT hRes = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));

    // 初始化 ATL 控件
    // 关键修复：添加 ICC_WIN95_CLASSES 和 ICC_STANDARD_CLASSES 确保菜单和普通控件正常显示
    // ICC_BAR_CLASSES - 工具栏和状态栏
    // ICC_LISTVIEW_CLASSES - ListView 控件
    // ICC_WIN95_CLASSES - 菜单、按钮、编辑框等 Win95 风格控件
    // ICC_STANDARD_CLASSES - 标准控件类
    AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES);

    // 【关键修复】强制加载 comctl32.dll v6.0 确保工具栏和 ReBar 在所有设备上正确显示
    // 即使 manifest 已嵌入，手动加载可以解决某些旧系统上的兼容性问题
    HMODULE hComCtl = LoadLibrary(L"comctl32.dll");
    if (hComCtl) {
        // 不释放库，让程序在整个生命周期中使用 v6.0
        OutputDebugString(_T("Loaded comctl32.dll for v6.0 support\n"));
    }

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = 0;
    // BLOCK: 运行应用程序
    {
        CMessageLoop theLoop;
        _Module.AddMessageLoop(&theLoop);

        CMainFrame wndMain;

        // 自己加载菜单，确保使用正确的模块句柄
        HMODULE hModule = GetModuleHandle(NULL);
        HMENU hMenu = ::LoadMenu(hModule, MAKEINTRESOURCE(IDR_MAINFRAME));
        if (!hMenu) {
            // 备用方案
            hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
        }

        // 使用 Create() 方法，传递正确加载的菜单
        if (wndMain.Create(NULL, NULL, _T("Simple Todo"), WS_OVERLAPPEDWINDOW, 0, hMenu) == NULL)
        {
            ATLTRACE(_T("主窗口创建失败!\r\n"));
            return 1;
        }

        // 设置窗口默认尺寸为 500x600
        wndMain.SetWindowPos(NULL, 0, 0, 500, 600, SWP_NOZORDER | SWP_NOMOVE);

        wndMain.ShowWindow(nCmdShow);

        nRet = theLoop.Run();

        _Module.RemoveMessageLoop();
    }

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}
