#pragma once
#include<Windows.h>
#define WM_SAVE (WM_USER)
#define WM_OPEN (WM_USER + 1)
#define WM_MERGE (WM_USER + 2)
#define WM_SPLIT (WM_USER + 3)

LRESULT CALLBACK gridwndproc(HWND Winhand, UINT msg, WPARAM wparam, LPARAM lparam);

