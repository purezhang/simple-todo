// afxres.h - Minimal MFC resource definitions stub
// This provides essential definitions without requiring full MFC
// Only defines IDs that are not already in Windows headers

#pragma once

// Menu command IDs (only those not typically in Windows headers)
#define ID_FILE_NEW            0xE100
#define ID_FILE_OPEN           0xE101
#define ID_FILE_SAVE_AS        0xE103
#define ID_FILE_PAGE_SETUP     0xE104
#define ID_FILE_PRINT_SETUP    0xE105
#define ID_FILE_PRINT_PREVIEW  0xE107
#define ID_FILE_UPDATE         0xE108
#define ID_FILE_SEND_MAIL      0xE109

#define ID_EDIT_REDO           0xE205

#define ID_VIEW_REBAR          0xE802
#define ID_VIEW_DIGITAL_CLOCK  0xE803
#define ID_VIEW_MENU_BAR       0xE804

#define ID_WINDOW_SPLIT        0xF007

#define ID_HELP_INDEX          0xF180
#define ID_HELP_FINDER         0xF181
#define ID_HELP_USING          0xF182
#define ID_CONTEXT_HELP        0xF183
#define ID_HELP                0xF184

// Message box flags (these are safe to define)
#define MB_OK                      0x00000000L
#define MB_OKCANCEL                0x00000001L
#define MB_YESNO                   0x00000004L
#define MB_YESNOCANCEL             0x00000003L
#define MB_RETRYCANCEL             0x00000005L
#define MB_CANCELTRYCONTINUE       0x00000006L

#define MB_ICONHAND                0x00000010L
#define MB_ICONQUESTION            0x00000020L
#define MB_ICONEXCLAMATION         0x00000030L
#define MB_ICONASTERISK            0x00000040L

#define MB_DEFBUTTON1              0x00000000L
#define MB_DEFBUTTON2              0x00000100L
#define MB_DEFBUTTON3              0x00000200L
