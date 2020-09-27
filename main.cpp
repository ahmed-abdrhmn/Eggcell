#include <Windows.h>
#include <string>
#include "Grid.h"


LRESULT CALLBACK mainwndproc(HWND,UINT,WPARAM,LPARAM);

const int RibbonHeight = 100;
const int MainWidth = 1280;
const int MainHeight = 720;

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
	wc.hCursor = LoadCursorW(NULL,(LPCWSTR) IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wc.hIcon = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(5),IMAGE_ICON, 0 , 0, LR_DEFAULTSIZE);
	wc.lpszClassName = L"Main";

	ATOM mainclass = RegisterClassExW(&wc);

	wc.lpfnWndProc = gridwndproc;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wc.lpszClassName = L"Grid";
	
	ATOM gridclass = RegisterClassExW(&wc);


	if (!mainclass) { return 120; }
	if (!gridclass) { return 120; }

	RECT windowrect = { 0,0,MainWidth,MainHeight };
	windowrect.right +=  GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXVSCROLL) + 10;
	windowrect.left -= GetSystemMetrics(SM_CXFIXEDFRAME);
	windowrect.bottom += GetSystemMetrics(SM_CYHSCROLL) + GetSystemMetrics(SM_CYFIXEDFRAME) + 10;
	windowrect.top -= GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME);

	
	HWND mainwin = CreateWindowW((LPCWSTR)(mainclass), L"Eggcell",
		WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top,
		NULL, NULL, 
		inst, NULL);

	GetClientRect(mainwin, &windowrect);
	windowrect.top += RibbonHeight;
	
	/*HWND gridwin =*/ CreateWindowW((LPCWSTR)(gridclass), L"Grid",
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
	}
	return DefWindowProcW(windowhandle, msg, wparam, lparam);
}

