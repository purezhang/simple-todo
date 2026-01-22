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

    // 此处提供对 ATL 控件容器的支持:
    // 【关键修复】必须初始化 ICC_LISTVIEW_CLASSES 才能使用 ListView 分组功能
    AtlInitCommonControls(ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES);

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = 0;
    // BLOCK: 运行应用程序
    {
        CMessageLoop theLoop;
        _Module.AddMessageLoop(&theLoop);

        CMainFrame wndMain;

        if (wndMain.CreateEx() == NULL)
        {
            ATLTRACE(_T("主窗口创建失败!\r\n"));
            return 1;
        }

        // 设置窗口默认尺寸为 400x600
        wndMain.SetWindowPos(NULL, 0, 0, 400, 600, SWP_NOZORDER | SWP_NOMOVE);

        wndMain.ShowWindow(nCmdShow);

        nRet = theLoop.Run();

        _Module.RemoveMessageLoop();
    }

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}
