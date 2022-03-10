#pragma once
#include<Windows.h>
#define WM_SAVE (WM_USER)
#define WM_OPEN (WM_USER + 1)

LRESULT CALLBACK gridwndproc(HWND Winhand, UINT msg, WPARAM wparam, LPARAM lparam);

