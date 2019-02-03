/* stub_winuser.c*/
#include "windows.h"

LRESULT WINAPI
SendMessageW(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return SendMessage(hwnd, Msg, wParam, lParam);
}

BOOL WINAPI
SetPropW( HWND hWnd, LPCWSTR lpString, HANDLE hData)
{
	printf("SetPropW IMPLEMENT ME!");
	return FALSE;
}

BOOL WINAPI
SystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	return SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}

BOOL WINAPI
SystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	return SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}
