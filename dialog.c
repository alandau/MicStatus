#include "dialog.h"

INT_PTR DialogBoxParamWithDefaultFont(HINSTANCE hInstance, LPCWSTR hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParam)
{
	LOGFONT lf;
	BOOL res = SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0);
	if (!res) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}
	int fontSizePt = lf.lfHeight >= 0 ?
		lf.lfHeight :
		MulDiv(-lf.lfHeight, 72, GetDeviceCaps(GetDC(NULL), LOGPIXELSY));

	HRSRC hRsrc = FindResource(hInstance, hDialogTemplate, RT_DIALOG);
	if (!hRsrc) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	DWORD size = SizeofResource(hInstance, hRsrc);
	if (!size) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	HGLOBAL hGlobal = LoadResource(hInstance, hRsrc);
	if (!hGlobal) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	BYTE* data = LockResource(hGlobal);
	if (!data) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	// We only support DIALOGEX templates
	if (!(size >= 4 && data[0] == 1 && data[1] == 0 && data[2] == 0xff && data[3] == 0xff)) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	// Skip initial static fields
	int offset = 26;
	// Skip 3 string fields
	offset += (lstrlen((wchar_t*)(data + offset)) + 1) * 2;
	offset += (lstrlen((wchar_t*)(data + offset)) + 1) * 2;
	offset += (lstrlen((wchar_t*)(data + offset)) + 1) * 2;

	// Now offset points to:
	// WORD wPointSize;
	// WORD wWeight;
	// BYTE bItalic;
	// BYTE bCharSet;
	// WCHAR szFontName[]; // variable-length
	
	int existingFontNameBytes = (lstrlen((wchar_t*)(data + offset + 6)) + 1) * 2;
	int newFontNameBytes = (lstrlen(lf.lfFaceName) + 1) * 2;

	BYTE* newTemplate = malloc(size - existingFontNameBytes + newFontNameBytes + 4); // +4 for DWORD alignment
	if (!newTemplate) {
		return DialogBoxParam(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParam);
	}

	// Copy initial bytes
	memcpy(newTemplate, data, offset);
	
	// Copy new font details
	newTemplate[offset] = fontSizePt & 0xff;
	newTemplate[offset + 1] = (fontSizePt >> 8) & 0xff;
	newTemplate[offset + 2] = lf.lfWeight & 0xff;
	newTemplate[offset + 3] = (lf.lfWeight >> 8) & 0xff;
	newTemplate[offset + 4] = lf.lfItalic;
	newTemplate[offset + 5] = lf.lfCharSet;
	memcpy(newTemplate + offset + 6, lf.lfFaceName, newFontNameBytes);

	// Align to DWORD
	int writeOffset = offset + 6 + newFontNameBytes;
	if (writeOffset % 4 != 0) {
		writeOffset += 4 - writeOffset % 4;
	}
	int readOffset = offset + 6 + existingFontNameBytes;
	if (readOffset % 4 != 0) {
		readOffset += 4 - readOffset % 4;
	}

	// Copy remaining template bytes
	memcpy(newTemplate + writeOffset, data + readOffset, size - readOffset);

	INT_PTR result = DialogBoxIndirectParam(hInstance, (DLGTEMPLATE*)newTemplate, hWndParent, lpDialogFunc, lParam);
	free(newTemplate);
	return result;
}

INT_PTR DialogBoxWithDefaultFont(HINSTANCE hInstance, LPCWSTR hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
	return DialogBoxParamWithDefaultFont(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, 0);
}
