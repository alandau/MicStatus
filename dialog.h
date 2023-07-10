#pragma once

#include <Windows.h>

INT_PTR DialogBoxWithDefaultFont(HINSTANCE hInstance, LPCWSTR hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc);
INT_PTR DialogBoxParamWithDefaultFont(HINSTANCE hInstance, LPCWSTR hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParam);
