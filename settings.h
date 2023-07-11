#pragma once

#include <Windows.h>

typedef struct Settings {
    // Mic
    BOOL bShowLed;
    BOOL bInvertLed;
    UINT uHotkey, uModifiers;

    // Speakers
    UINT uUnmuteHotkey, uUnmuteModifiers;
    BOOL bUnmuteChangeVolume;
    UINT uUnmuteVolume;

    UINT uMuteHotkey, uMuteModifiers;
    BOOL bMuteChangeVolume;
    UINT uMuteVolume;
} Settings;

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void LoadSettings(Settings* settings);
