// SimpleTodo.cpp : main source file for SimpleTodo.exe
//

#include "stdafx.h"
#include "MainFrm.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpstrCmdLine);

    HRESULT hRes = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));

    // 此处提供对 ATL 控件容器的支持:
    AtlInitCommonControls(ICC_BAR_CLASSES);    // 添加标志以支持其他控件类

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
            ATLTRACE(_T("主窗口创建失败!\\r\\n"));
            return 1;
        }

        wndMain.ShowWindow(nCmdShow);

        nRet = theLoop.Run();

        _Module.RemoveMessageLoop();
    }

    _Module.Term();
    ::CoUninitialize();

    return nRet;
}
