#define LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>
#include <Windows.h>
#include <windowsx.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

static const CLSID CLSID_MMDeviceEnumerator = { 0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e} }; // BCDE0395-E52F-467C-8E3D-C4579291692E
static const IID IID_IMMDeviceEnumerator = { 0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6} }; // A95664D2-9614-4F35-A746-DE8DB63617E6
static const IID IID_IAudioEndpointVolume = { 0x5cdf2c82, 0x841e, 0x4546, {0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a} }; // 5CDF2C82-841E-4546-9722-0CF74078229A
static const IID IID_IAudioEndpointVolumeCallback = { 0x657804fa, 0xd6ad, 0x4496, {0x8a, 0x60, 0x35, 0x27, 0x52, 0xaf, 0x4f, 0x89} }; // 657804FA-D6AD-4496-8A60-352752AF4F89

#define WM_APP_NOTIFYICON WM_APP
#define WM_APP_MUTE_STATE_CHANGED (WM_APP + 1)
#define WM_APP_DEVICE_LIST_CHANGED (WM_APP + 2)

typedef struct MicCallback {
    IAudioEndpointVolumeCallback parent; // Must be first
    LONG refcount;
    HWND hWnd;
} MicCallback;

static ULONG STDMETHODCALLTYPE MicCallbackAddRef(IAudioEndpointVolumeCallback* This) {
    MicCallback* self = (MicCallback*)This;
    return InterlockedIncrement(&self->refcount);
}

static ULONG STDMETHODCALLTYPE MicCallbackRelease(IAudioEndpointVolumeCallback* This) {
    MicCallback* self = (MicCallback*)This;
    LONG count = InterlockedDecrement(&self->refcount);
    if (count == 0) {
        HeapFree(GetProcessHeap(), 0, self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE MicCallbackQueryInterface(IAudioEndpointVolumeCallback* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioEndpointVolumeCallback)) {
        MicCallbackAddRef(This);
        *ppvObject = This;
        return NOERROR;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE MicCallbackOnNotify(IAudioEndpointVolumeCallback* This, PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) {
    MicCallback* self = (MicCallback*)This;
    PostMessage(self->hWnd, WM_APP_MUTE_STATE_CHANGED, pNotify->bMuted, 0);
    return 0;
}

static IAudioEndpointVolumeCallbackVtbl MicCallbackVtbl = {
    MicCallbackQueryInterface,
    MicCallbackAddRef,
    MicCallbackRelease,
    MicCallbackOnNotify,
};

static IAudioEndpointVolumeCallback* STDMETHODCALLTYPE MicCallbackCreate(HWND hWnd) {
    MicCallback* self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MicCallback));
    if (self == NULL) {
        return NULL;
    }
    self->parent.lpVtbl = &MicCallbackVtbl;
    self->refcount = 1;
    self->hWnd = hWnd;
    return (IAudioEndpointVolumeCallback*)self;
}

typedef struct DeviceListCallback {
    IMMNotificationClient parent; // Must be first
    LONG refcount;
    HWND hWnd;
} DeviceListCallback;

static ULONG STDMETHODCALLTYPE DeviceListCallbackAddRef(IMMNotificationClient* This) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    return InterlockedIncrement(&self->refcount);
}

static ULONG STDMETHODCALLTYPE DeviceListCallbackRelease(IMMNotificationClient* This) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    LONG count = InterlockedDecrement(&self->refcount);
    if (count == 0) {
        HeapFree(GetProcessHeap(), 0, self);
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DeviceListCallbackQueryInterface(IMMNotificationClient* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioEndpointVolumeCallback)) {
        DeviceListCallbackAddRef(This);
        *ppvObject = This;
        return NOERROR;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE DeviceListCallbackOnDeviceStateChanged(
    IMMNotificationClient* This, LPCWSTR pwstrDeviceId, DWORD dwNewState) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    PostMessage(self->hWnd, WM_APP_DEVICE_LIST_CHANGED, 0, 0);
    return 0;
}

HRESULT STDMETHODCALLTYPE DeviceListCallbackOnDeviceAdded(IMMNotificationClient* This, LPCWSTR pwstrDeviceId) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    PostMessage(self->hWnd, WM_APP_DEVICE_LIST_CHANGED, 0, 0);
    return 0;
}

HRESULT STDMETHODCALLTYPE DeviceListCallbackOnDeviceRemoved(IMMNotificationClient* This, LPCWSTR pwstrDeviceId) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    PostMessage(self->hWnd, WM_APP_DEVICE_LIST_CHANGED, 0, 0);
    return 0;
}

HRESULT STDMETHODCALLTYPE DeviceListCallbackOnDefaultDeviceChanged(
    IMMNotificationClient* This,
    EDataFlow flow,
    ERole role,
    LPCWSTR pwstrDefaultDeviceId) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    PostMessage(self->hWnd, WM_APP_DEVICE_LIST_CHANGED, 0, 0);
    return 0;
}

HRESULT STDMETHODCALLTYPE DeviceListCallbackOnPropertyValueChanged(
    IMMNotificationClient* This,
    LPCWSTR pwstrDeviceId,
    const PROPERTYKEY key) {
    DeviceListCallback* self = (DeviceListCallback*)This;
    PostMessage(self->hWnd, WM_APP_DEVICE_LIST_CHANGED, 0, 0);
    return 0;
}

static IMMNotificationClientVtbl DeviceListCallbackVtbl = {
    DeviceListCallbackQueryInterface,
    DeviceListCallbackAddRef,
    DeviceListCallbackRelease,
    DeviceListCallbackOnDeviceStateChanged,
    DeviceListCallbackOnDeviceAdded,
    DeviceListCallbackOnDeviceRemoved,
    DeviceListCallbackOnDefaultDeviceChanged,
    DeviceListCallbackOnPropertyValueChanged,
};

static IMMNotificationClient* STDMETHODCALLTYPE DeviceListCallbackCreate(HWND hWnd) {
    DeviceListCallback* self = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MicCallback));
    if (self == NULL) {
        return NULL;
    }
    self->parent.lpVtbl = &DeviceListCallbackVtbl;
    self->refcount = 1;
    self->hWnd = hWnd;
    return (IMMNotificationClient*)self;
}

static struct G {
    HICON hActiveIcon, hMutedIcon;
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioEndpointVolume* pVolume;
    IAudioEndpointVolumeCallback* pMutedCallback;
    IMMNotificationClient* pDeviceListCallback;
} G;

static BOOL InitGlobals(HWND hWnd) {
    static const wchar_t icon_dll[] = L"%SystemRoot%\\System32\\SndVolSSO.dll";
    G.hActiveIcon = ExtractIcon(NULL, icon_dll, -141);
    if (G.hActiveIcon == NULL || G.hActiveIcon == (HICON)1) {
        return FALSE;
    }
    G.hMutedIcon = ExtractIcon(NULL, icon_dll, -140);
    if (G.hMutedIcon == NULL || G.hMutedIcon == (HICON)1) {
        return FALSE;
    }

    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, &G.pEnumerator))) {
        return FALSE;
    }
    G.pMutedCallback = MicCallbackCreate(hWnd);
    G.pDeviceListCallback = DeviceListCallbackCreate(hWnd);
    G.pEnumerator->lpVtbl->RegisterEndpointNotificationCallback(G.pEnumerator, G.pDeviceListCallback);

    return TRUE;
}

static BOOL InitMuteListener(void) {
    if (G.pVolume) {
        if (G.pMutedCallback) {
            G.pVolume->lpVtbl->UnregisterControlChangeNotify(G.pVolume, G.pMutedCallback);
        }
        G.pVolume->lpVtbl->Release(G.pVolume);
        G.pVolume = NULL;
    }
    if (G.pDevice) {
        G.pDevice->lpVtbl->Release(G.pDevice);
        G.pDevice = NULL;
    }
    if (FAILED(G.pEnumerator->lpVtbl->GetDefaultAudioEndpoint(G.pEnumerator, eCapture, eCommunications, &G.pDevice))) {
        return FALSE;
    }
    if (FAILED(G.pDevice->lpVtbl->Activate(G.pDevice, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, &G.pVolume))) {
        return FALSE;
    }
    if (FAILED(G.pVolume->lpVtbl->RegisterControlChangeNotify(G.pVolume, G.pMutedCallback))) {
        return FALSE;
    }
    return TRUE;
}

static BOOL IsMicActive(void) {
    BOOL muted;
    if (G.pVolume && SUCCEEDED(G.pVolume->lpVtbl->GetMute(G.pVolume, &muted))) {
        return !muted;
    }
    return FALSE;
}

static void ToggleMicMute(void) {
    if (G.pVolume) {
        BOOL muted;
        if (FAILED(G.pVolume->lpVtbl->GetMute(G.pVolume, &muted))) {
            return;
        }
        G.pVolume->lpVtbl->SetMute(G.pVolume, !muted, NULL);
    }
}

static BOOL CreateOrSetTrayIcon(HWND hWnd, BOOL bMicActive, BOOL bCreate) {
    NOTIFYICONDATA ni = { 0 };
    ni.cbSize = sizeof(ni);
    ni.hWnd = hWnd;
    ni.uID = 0;
    ni.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    ni.uCallbackMessage = WM_APP_NOTIFYICON;
    ni.hIcon = bMicActive ? G.hActiveIcon : G.hMutedIcon;
    ni.uVersion = NOTIFYICON_VERSION_4;
    wcscpy_s(ni.szTip, ARRAYSIZE(ni.szTip), bMicActive ? L"Microphone: Active" : L"Microphone: Muted");

    if (bCreate) {
        return Shell_NotifyIcon(NIM_ADD, &ni) && Shell_NotifyIcon(NIM_SETVERSION, &ni);
    } else {
        return Shell_NotifyIcon(NIM_MODIFY, &ni);
    }
}

static void DestroyTrayIcon(HWND hWnd) {
    NOTIFYICONDATA ni = { 0 };
    ni.cbSize = sizeof(ni);
    ni.hWnd = hWnd;
    ni.uID = 0;
    Shell_NotifyIcon(NIM_DELETE, &ni);
}

static void FillInputStruct(INPUT* input, WORD vk, BOOL press) {
    memset(input, 0, sizeof(INPUT));
    input->type = INPUT_KEYBOARD;
    input->ki.wVk = vk;
    if (!press) {
        input->ki.dwFlags = KEYEVENTF_KEYUP;
    }
}
static void SetLedState(BOOL active) {
    BOOL led = !!(GetKeyState(VK_SCROLL) & 1);
    if (led == !!active) {
        return;
    }
    // Press scroll lock, with shift to avoid triggering hotkey.
    // If SetLedState is called due to a press on scroll lock, the key might still be down,
    // so we need to release it (inputs[0], inputs[1]) before pressing it again for the
    // press to take effect.
    INPUT inputs[6];
    FillInputStruct(&inputs[0], VK_SCROLL, FALSE);
    FillInputStruct(&inputs[1], VK_SHIFT, FALSE);
    FillInputStruct(&inputs[2], VK_SHIFT, TRUE);
    FillInputStruct(&inputs[3], VK_SCROLL, TRUE);
    FillInputStruct(&inputs[4], VK_SCROLL, FALSE);
    FillInputStruct(&inputs[5], VK_SHIFT, FALSE);
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

static LRESULT CALLBACK MicWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        RegisterHotKey(hWnd, 1, 0, VK_SCROLL);
        //s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
        if (!InitGlobals(hWnd) || !InitMuteListener()) {
            return -1;
        }
        BOOL active = IsMicActive();
        if (!CreateOrSetTrayIcon(hWnd, active, TRUE)) {
            return -1;
        }
        SetLedState(active);
        return 0;
    case WM_HOTKEY:
        ToggleMicMute();
        return 0;
    case WM_DESTROY:
        DestroyTrayIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    case WM_APP_NOTIFYICON:
        switch (LOWORD(lParam)) {
        case NIN_SELECT:
            ToggleMicMute();
            break;
        case WM_CONTEXTMENU: {
            // Set our window as foreground so the menu disappears when focus is lost
            SetForegroundWindow(hWnd);
            HMENU hMenu = CreatePopupMenu();
            InsertMenu(hMenu, -1, MF_BYPOSITION, 1, L"E&xit");
            int id = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), 0, hWnd, NULL);
            switch (id) {
            case 0:
                // Showing the menu failed
                break;
            case 1:
                DestroyWindow(hWnd);
                break;
            }
            DestroyMenu(hMenu);
            break;
        }
        }
        return 0;
    case WM_APP_MUTE_STATE_CHANGED: {
        BOOL active = !wParam;
        CreateOrSetTrayIcon(hWnd, active, FALSE);
        SetLedState(active);
        return 0;
    }
    case WM_APP_DEVICE_LIST_CHANGED: {
        if (!InitMuteListener()) {
            return 0;
        }
        BOOL active = IsMicActive();
        if (!CreateOrSetTrayIcon(hWnd, active, FALSE)) {
            return 0;
        }
        SetLedState(active);
        return 0;
    }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (FAILED(CoInitialize(NULL))) {
        return 1;
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MicWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MicMute";
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(L"MicMute", L"", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();

    return (int)msg.wParam;
}
