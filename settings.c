#include "settings.h"
#include "resource.h"
#include <CommCtrl.h>

static BOOL GetAppVersion(wchar_t* buf, DWORD len) {
    wchar_t filename[255];
    if (!GetModuleFileName(NULL, filename, ARRAYSIZE(filename))) {
        return FALSE;
    }
    DWORD size = GetFileVersionInfoSize(filename, NULL);
    if (!size) {
        return FALSE;
    }
    void* data = malloc(size);
    if (!data) {
        return FALSE;
    }
    if (!GetFileVersionInfo(filename, 0, size, data)) {
        free(data);
        return FALSE;
    }
    void* ver;
    UINT verlen;
    if (!VerQueryValue(data, L"\\StringFileInfo\\040904b0\\ProductVersion", &ver, &verlen)) {
        free(data);
        return FALSE;
    }
    if (verlen > len) {
        free(data);
        return FALSE;
    }
    memcpy(buf, ver, verlen * sizeof(wchar_t));
    free(data);
    return TRUE;
}

static void SaveSettings(const Settings* settings) {
}

static BYTE ConvertModifiersWmHotkeyToControl(UINT mods) {
    BYTE res = 0;
    if (mods & MOD_ALT) res |= HOTKEYF_ALT;
    if (mods & MOD_CONTROL) res |= HOTKEYF_CONTROL;
    if (mods & MOD_SHIFT) res |= HOTKEYF_SHIFT;
    if (mods & MOD_WIN) res |= HOTKEYF_EXT;
    return res;
}

static UINT ConvertModifiersControlToWmHotkey(BYTE mods) {
    UINT res = 0;
    if (mods & HOTKEYF_ALT) res |= MOD_ALT;
    if (mods & HOTKEYF_CONTROL) res |= MOD_CONTROL;
    if (mods & HOTKEYF_SHIFT) res |= MOD_SHIFT;
    if (mods & HOTKEYF_EXT) res |= MOD_WIN;
    return res;
}

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        Settings* settings = (Settings*)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)settings);
        wchar_t ver[20];
        if (GetAppVersion(ver, ARRAYSIZE(ver))) {
            wchar_t caption[100] = L"MicStatus ";
            wcscat_s(caption, ARRAYSIZE(caption), ver);
            SetWindowText(hDlg, caption);
        }
        CheckDlgButton(hDlg, IDC_LED_CHECK, settings->bShowLed ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_LED_INVERT_CHECK, settings->bInvertLed ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(hDlg, IDC_LED_INVERT_CHECK), settings->bShowLed);
        SendMessage(GetDlgItem(hDlg, IDC_HOTKEY), HKM_SETHOTKEY,
            MAKEWORD(settings->uHotkey, ConvertModifiersWmHotkeyToControl(settings->uModifiers)), 0);
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            Settings* settings = (Settings*)GetWindowLongPtr(hDlg, DWLP_USER);
            settings->bShowLed = IsDlgButtonChecked(hDlg, IDC_LED_CHECK);
            settings->bInvertLed = IsDlgButtonChecked(hDlg, IDC_LED_INVERT_CHECK);
            LRESULT hk = SendMessage(GetDlgItem(hDlg, IDC_HOTKEY), HKM_GETHOTKEY, 0, 0);
            settings->uHotkey = LOBYTE(LOWORD(hk));
            settings->uModifiers = ConvertModifiersControlToWmHotkey(HIBYTE(LOWORD(hk)));
            SaveSettings(settings);
            EndDialog(hDlg, 1);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        case IDC_LED_CHECK:
            if (HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hDlg, IDC_LED_INVERT_CHECK), IsDlgButtonChecked(hDlg, IDC_LED_CHECK));
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

void LoadSettings(Settings* settings) {
    settings->bShowLed = TRUE;
    settings->bInvertLed = FALSE;
    settings->uHotkey = VK_SCROLL;
    settings->uModifiers = 0;
}