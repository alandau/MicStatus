#pragma once

#include <Windows.h>

typedef struct Settings {
    BOOL bShowLed;
    BOOL bInvertLed;
    UINT uHotkey;
    UINT uModifiers;
} Settings;

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void LoadSettings(Settings* settings);
