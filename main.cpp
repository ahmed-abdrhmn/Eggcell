#include <Windows.h>
#include<string>
#include <string>
#include "Grid.h"
#include "resource1.h"


LRESULT CALLBACK mainwndproc(HWND,UINT,WPARAM,LPARAM);

const int RibbonHeight = 0;
const int MainWidth = 1280;
const int MainHeight = 720;

static HWND gridwin;
static std::wstring currentfile = L"Untitiled.egg";

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int cmdshow) {
	
	//SystemParametersInfo(SPI_SETFONTSMOOTHING,
	//	TRUE,
	//	0,
	//	SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	//SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE,
	//	0,
	//	(PVOID)FE_FONTSMOOTHINGCLEARTYPE,
	//	SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	SetProcessDPIAware(); //Disable DPI scaling so that window is not blurry

	WNDCLASSEXW wc = {};
	wc.lpfnWndProc = mainwndproc;
	wc.cbSize = sizeof(wc);
	wc.hInstance = inst;
	wc.style = CS_OWNDC;
	wc.cbClsExtra = 0;
	wc.hCursor = LoadCursorW(inst,MAKEINTRESOURCEW(MyCursor_IDC));
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(Fav),IMAGE_ICON, 0 , 0, LR_DEFAULTSIZE);
	wc.lpszClassName = L"Main";
	ATOM mainclass = RegisterClassExW(&wc);

	wc.lpfnWndProc = gridwndproc;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = L"Grid";
	
	ATOM gridclass = RegisterClassExW(&wc);
	
	HMENU mainmenu = CreateMenu();
	InsertMenuW(mainmenu, -1, MF_BYPOSITION | MF_STRING, 0, L"Open");
	InsertMenuW(mainmenu, -1, MF_BYPOSITION | MF_STRING, 1, L"Save");

	if (!mainclass) { return 120; }
	if (!gridclass) { return 120; }

	RECT windowrect = { 0,0,MainWidth,MainHeight };
	windowrect.right +=  GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXVSCROLL) + 10;
	windowrect.left -= GetSystemMetrics(SM_CXFIXEDFRAME);
	windowrect.bottom += GetSystemMetrics(SM_CYHSCROLL) + GetSystemMetrics(SM_CYFIXEDFRAME) + 10;
	windowrect.top -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYMENU);
	
	HWND mainwin = CreateWindowW((LPCWSTR)(mainclass), L"Eggcell - Untitled.egg",
		WS_VISIBLE|WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top,
		NULL, mainmenu,
		inst, NULL);

	GetClientRect(mainwin, &windowrect);
	windowrect.top += RibbonHeight;
	
	gridwin =CreateWindowW((LPCWSTR)(gridclass), L"Grid",
		WS_CHILD| WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
		windowrect.left, windowrect.top, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top,
		mainwin, (HMENU)1,
		inst, NULL);

	GetClientRect(mainwin, &windowrect);
	
	//std::string ToPrint = "(" + std::to_string(windowrect.left) + ", " + std::to_string(windowrect.right) + ", " + std::to_string(windowrect.top) + ", " + std::to_string(windowrect.bottom) + ")";

	//MessageBoxA(NULL, ToPrint.c_str(), "WindowRect", MB_OK);

	//ToPrint.clear();
	
	if (!mainwin) { return 123; }

	MSG msg;
	while (GetMessageW(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	
	UnregisterClassW((LPCWSTR)(mainclass), inst);
	UnregisterClassW((LPCWSTR)(gridclass), inst);
	
	return 0;
}


LRESULT CALLBACK mainwndproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_CREATE: {
		HDC DeviceContext = GetDC(windowhandle);
		HPEN line = CreatePen(PS_SOLID|PS_ENDCAP_FLAT,2, RGB(0, 0, 0));
		SelectObject(DeviceContext, line);
		break;
	}
	case WM_CLOSE: {
		PostQuitMessage(0);
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT pts;
		HDC PaintDC = BeginPaint(windowhandle, &pts);
		RECT clientarea;
		GetClientRect(windowhandle, &clientarea);
		
		POINT line[2] = { {clientarea.left,RibbonHeight},{clientarea.right,RibbonHeight} };
		Polyline(PaintDC, line, 2);
		EndPaint(windowhandle, &pts);
		break;
	}
	case WM_KEYDOWN: {
		if (wparam == 'R' || (lparam & (1u<<30)) == 1){
			RedrawWindow(windowhandle,NULL,NULL, RDW_INVALIDATE);
		}
		break;
	}
	case WM_SIZE: {
		RECT clientrect;
		GetClientRect(windowhandle, &clientrect);
		MoveWindow(gridwin, 0, RibbonHeight, clientrect.right, clientrect.bottom - RibbonHeight, TRUE);
		break;
	}
	case WM_COMMAND: {
		if (HIWORD(wparam) == 0) { //comes from a menu
			switch (LOWORD(wparam)) {
			case 0: {
				OPENFILENAMEW ofn = { 0 };
				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = windowhandle;
				ofn.lpstrFile = new WCHAR[256];     
				ofn.lpstrFile[0] = L'\0';
				ofn.nMaxFile = 256;
				ofn.lpstrFilter = L"Egg Files (*.egg)\0*.egg\0";
				ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
				BOOL open = GetOpenFileNameW(&ofn);
				if (open) {	
					LRESULT success = SendMessageW(gridwin, WM_OPEN, NULL, (LPARAM)&ofn);
					if (success) {
						currentfile = std::wstring(ofn.lpstrFile + ofn.nFileOffset);
						SetWindowTextW(windowhandle, (std::wstring(L"Eggcell - ") + currentfile).c_str()); //Changing the window title to reflect opened file
					}
				}
				delete[] ofn.lpstrFile;
				break;
			}
			case 1: {
				OPENFILENAMEW ofn = { 0 };
				ofn.lStructSize = sizeof(OPENFILENAMEW);
				ofn.hwndOwner = windowhandle;
				ofn.lpstrFile = new WCHAR[256];
				wcsncpy_s(ofn.lpstrFile, 256, currentfile.c_str(), 255);
				ofn.nMaxFile = 256;
				ofn.lpstrFilter = L"Egg Files (*.egg)\0*.egg\0";
				ofn.Flags = OFN_ENABLESIZING| OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt = L"egg";
				BOOL save = GetSaveFileNameW(&ofn);
				if (save) {
					SendMessageW(gridwin, WM_SAVE, NULL, (LPARAM)&ofn);
				}
				break;
			}
			}
		}
		break;
	}
	}
	return DefWindowProcW(windowhandle, msg, wparam, lparam);
}

