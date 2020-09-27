#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <string>
#include "Grid.h"
#include "Storage.h"
#include <iostream>

#define WM_EDITENTER (WM_APP + 0)
#define EM_CELLCLICKED (WM_APP + 1)

const int cellwidth = 240; //fixed width of each cell in pixels
const int cellheight = 30; //fixed height of each cell in pixels
const int sheetwidth = 50; //width of spreadsheet in cells
const int sheetheight = 100; //height of spreadsheet in cells

LRESULT CALLBACK editproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam);
static WNDPROC oldeditproc;
static HWND EditWindow;
static bool updateselect;


inline int CeilDiv(int n, int d) { //divison such that the ceiling of the quotient is returned (if numerator and denominator have the same sign)
	return ((n - 1) / d) + 1;
}

inline int FloorDiv(int n, int d) { //divison such that the floor of the quotient is returned (if numerator and denominator have the same sign)
	return n/d;
}

static WorkSheet OneWkst;
WorkSheet* WorkSheet::CurrentWorksheet = &OneWkst;


WCHAR* IndToCol(unsigned column,unsigned row) {
	column++; //convert from zero base to one base
	const unsigned buffsize = 20;
	WCHAR* Buffer = new WCHAR[buffsize];
	*(Buffer + buffsize - 1) = L'\0';
	WCHAR* Index = Buffer + buffsize - 2;
	
	do {
		*(Index--) = L'0' + row % 10;
		row /= 10;
	} while (row);

	 while (column) {
		 unsigned mod = (column - 1) % 26;
		 *(Index--) = L'A' + mod;
		column = (column - mod) / 26;
	 };
	
	unsigned long long offset = Index - Buffer + 1;
	Index = Buffer;
	
	do{
		*(Index) = *(Index + offset);
		Index++;
	} while (*Index != L'\0');
	
	return Buffer;
}


LRESULT CALLBACK gridwndproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam) {
	

	static HFONT spFont = CreateFontW(26, 10, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
	
	switch (msg) {
	case WM_CREATE: {
		//Configure Pen
		HDC DeviceContext = GetDC(windowhandle);
		HPEN line = CreatePen(PS_SOLID | PS_ENDCAP_FLAT, 2, RGB(0, 0, 0));
		SelectObject(DeviceContext, line);
		
		//Configure Scrolling
		RECT clientarea;
		GetClientRect(windowhandle, &clientarea);
		
		SCROLLINFO si;
		//config horizontal scroll
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE|SIF_PAGE;
		si.nMin = 0;
		si.nMax = cellwidth * sheetwidth;
		si.nPage = clientarea.right;
		SetScrollInfo(windowhandle, SB_HORZ, &si, TRUE);
		
		//config vertical scroll
		si.nMax = cellheight * sheetheight;
		si.nPage = clientarea.bottom;
		SetScrollInfo(windowhandle, SB_VERT, &si, TRUE);
		//Insert Placeholder data into spreadsheet

		OneWkst.SetCell(L"Hello", 3, 0);
		OneWkst.SetCell(L"lalal", 8, 8);
		OneWkst.SetCell(L"هلا بالخميس", 0, 0);

		//Config Font
		SelectObject(DeviceContext, spFont);
		SetBkMode(DeviceContext, TRANSPARENT);
		
		
		ReleaseDC(windowhandle, DeviceContext);
		break;
	}
	case WM_VSCROLL: {
		if (LOWORD(wparam) == SB_THUMBTRACK || LOWORD(wparam) == SB_THUMBPOSITION) {
			RECT clientarea;
			GetClientRect(windowhandle, &clientarea);

			SCROLLINFO vsi;
			vsi.cbSize = sizeof(vsi);
			vsi.fMask = SIF_TRACKPOS | SIF_POS;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);

			ScrollWindow(windowhandle, 0, vsi.nPos - vsi.nTrackPos, NULL, &clientarea);
			
			vsi.nPos = vsi.nTrackPos; //Update Scroll Position
			SetScrollInfo(windowhandle, SB_VERT, &vsi, FALSE);
		}
		break;
	}
	case WM_HSCROLL: {
		if (LOWORD(wparam) == SB_THUMBTRACK || LOWORD(wparam) == SB_THUMBPOSITION) {
			RECT clientarea;
			GetClientRect(windowhandle, &clientarea);

			SCROLLINFO hsi;
			hsi.cbSize = sizeof(hsi);
			hsi.fMask = SIF_TRACKPOS | SIF_POS;
			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

			ScrollWindow(windowhandle, hsi.nPos - hsi.nTrackPos, 0 , NULL, &clientarea);

			hsi.nPos = hsi.nTrackPos; //Update Scroll Position
			SetScrollInfo(windowhandle, SB_HORZ, &hsi, FALSE);
		}
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT pts;

		HDC PaintDC = BeginPaint(windowhandle, &pts);
		RECT updaterect = pts.rcPaint;
		int updatewidth = updaterect.right - updaterect.left;
		int updateheight = updaterect.bottom - updaterect.top;
		
		RECT clientarea;
		GetClientRect(windowhandle, &clientarea);
		
		
		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS;												hsi.fMask = SIF_POS;
		GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);


		//std::string toprint = '(' + std::to_string(updaterect.left) + ' ' + std::to_string(updaterect.top) + ' ' + std::to_string(updaterect.right) + ' ' + std::to_string(updaterect.bottom);
		//
		//MessageBoxA(NULL, toprint.c_str(), "UpdateRegion", MB_OK);
		
		int gridbaseY = FloorDiv(vsi.nPos + updaterect.top , cellheight) * cellheight; //integer division rounds downs
		int gridbaseX = FloorDiv(hsi.nPos + updaterect.left , cellwidth) * cellwidth; //integer division rounds downs
		
		int gridmaxY = FloorDiv( vsi.nPos + updaterect.bottom , cellheight) * cellheight; //integer division rounds downs
		int gridmaxX = FloorDiv( hsi.nPos + updaterect.right , cellwidth) * cellwidth; //integer division rounds downs
	
		//draw vertical lines
		for (int i = 0; i <= (updatewidth / cellwidth) + 1; ++i) {
			POINT line[2] = { {gridbaseX + cellwidth * i - hsi.nPos,clientarea.top - 3},{gridbaseX + cellwidth * i - hsi.nPos,clientarea.bottom + 3} };
			Polyline(PaintDC, line, 2);
		}

		//draw horizontal lines
		for (int i = 0; i <= (updateheight / cellheight) + 1; ++i) {
			POINT line[2] = { {clientarea.left - 3,gridbaseY + cellheight * i - vsi.nPos},{clientarea.right + 3, gridbaseY + cellheight * i - vsi.nPos} };
			Polyline(PaintDC, line, 2);
		}

		for (int tlx/*short for top-left x*/ = gridbaseX; tlx <= gridmaxX; tlx += cellwidth) {
			for (int tly/*short for top-left y*/ = gridbaseY;tly <= gridmaxY; tly += cellheight){
				
				unsigned xind = tlx/cellwidth; //x or column Index
				unsigned yind = tly/cellheight; //y or row Index

				const WorkSheet::cell ToDraw = OneWkst.GetCell(xind, yind);

				if (ToDraw.type != WorkSheet::cell::type::null) {
					RECT cellrect; //the rect that will contain the text
					cellrect.left = tlx - hsi.nPos + 1;
					cellrect.top = tly - vsi.nPos + 1;
					cellrect.right = cellrect.left + cellwidth - 1;
					cellrect.bottom = cellrect.top + cellheight - 1;

					DrawTextW(PaintDC, ToDraw.tt.c_str(), -1, &cellrect, DT_INTERNAL | DT_VCENTER | DT_SINGLELINE);
				}
			}
		}

		EndPaint(windowhandle, &pts);
		break;
	}
	case WM_LBUTTONDOWN: {
		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS;												hsi.fMask = SIF_POS;
		GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

		int x = GET_X_LPARAM(lparam); int y = GET_Y_LPARAM(lparam);
		x = ((x + hsi.nPos) / cellwidth) * cellwidth - hsi.nPos;
		y = ((y + vsi.nPos) / cellheight) * cellheight - vsi.nPos;
		//now x and y point to the top-left of the cell the mouse is on in screen space
		RECT tofill = { x + 1,y + 1,x + cellwidth - 1,y + cellheight - 1 };
		
		WCHAR* Buffer = new WCHAR[GetWindowTextLengthW(EditWindow) + (size_t)1/*null terminator*/];
		if (!Buffer) MessageBoxA(NULL, "Allocation Failure", "Allocation Failure", MB_ICONERROR);
		unsigned index;

		if (EditWindow == NULL || (index = GetWindowTextW(EditWindow, Buffer, INT_MAX), *Buffer != L'=')) {

			if (EditWindow != NULL) { //apply and destroy old window if it exists;
				RECT editcoord;
				GetWindowRect(EditWindow, &editcoord);
				MapWindowPoints(NULL, GetParent(EditWindow), (LPPOINT)&editcoord, 2);

				unsigned Xind = (editcoord.left + hsi.nPos) / cellwidth;
				unsigned Yind = (editcoord.top + vsi.nPos) / cellheight;

				(void)OneWkst.SetCell(Buffer, Xind, Yind);
				InvalidateRect(windowhandle, NULL, TRUE);

				DestroyWindow(EditWindow);
			}

			HWND editctrl = CreateWindowW(L"EDIT", L"Enter Text",
				WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
				tofill.left, tofill.top, tofill.right - tofill.left, tofill.bottom - tofill.top,
				windowhandle, (HMENU)0,
				(HINSTANCE)GetWindowLongPtrW(windowhandle, GWLP_HINSTANCE), NULL);
			
			oldeditproc = (WNDPROC)SetWindowLongPtrW(editctrl, GWLP_WNDPROC, (LONG_PTR)editproc);
			EditWindow = editctrl;

			unsigned Xind = (tofill.left + hsi.nPos) / cellwidth;
			unsigned Yind = (tofill.top + vsi.nPos) / cellheight;
			SetWindowTextW(editctrl, OneWkst.GetCell(Xind,Yind).ExactInput.c_str());
			SetFocus(editctrl);
			SendMessageW(editctrl, WM_SETFONT, (WPARAM)spFont, TRUE);

		}
		else {
			unsigned Start;
			unsigned End;
			SendMessageW(EditWindow, EM_GETSEL, (WPARAM)&Start,(LPARAM)&End);
			Start = std::min(Start, End);


			unsigned Xind = (tofill.left + hsi.nPos) / cellwidth;
			unsigned Yind = (tofill.top + vsi.nPos) / cellheight;
			WCHAR* strptr = IndToCol(Xind,Yind);
			SendMessageW(EditWindow, EM_REPLACESEL, FALSE, (LPARAM)strptr);
			SendMessageW(EditWindow, EM_SETSEL, Start, Start + std::wcslen(strptr));
			updateselect = true;
			delete[] strptr;
		}
		
		if(Buffer)delete[] Buffer;
		break;
	}
	case WM_EDITENTER: {
		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS;												hsi.fMask = SIF_POS;
		GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);



		HWND editctrl = (HWND)lparam;


		RECT editcoord;
		GetWindowRect(editctrl, &editcoord);
		MapWindowPoints(NULL, GetParent(editctrl), (LPPOINT)&editcoord, 2);

		unsigned Xind = (editcoord.left + hsi.nPos) / cellwidth;
		unsigned Yind = (editcoord.top + vsi.nPos) / cellheight;

		WCHAR* Buffer = new WCHAR[GetWindowTextLengthW(editctrl) + (size_t)1];
		GetWindowTextW(editctrl, Buffer, INT_MAX);


		if (OneWkst.SetCell(Buffer, Xind, Yind)) { //Set Cell fails
			MessageBoxW(NULL, L"The cell directly or indirectly refrences itself in the formula", L"Self Reference", MB_OK | MB_ICONERROR);
			SetFocus(editctrl);
		}
		else { //Set Cell Succeedes
			InvalidateRect(windowhandle, NULL, TRUE);
			DestroyWindow(editctrl);
			EditWindow = NULL;
		}
		delete[] Buffer;
		break;
	}
	}

	return DefWindowProcW(windowhandle, msg, wparam, lparam);
}


LRESULT CALLBACK editproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_KEYDOWN ) {
		if (wparam == VK_RETURN) {
			PostMessageW((HWND)GetWindowLongPtrW(windowhandle, GWLP_HWNDPARENT), WM_EDITENTER, NULL, (LPARAM)windowhandle);
		}
		if (updateselect) {
			CallWindowProcW(oldeditproc, windowhandle, EM_SETSEL, -1,NULL); //deselect then type
		}
	}
	if (msg == WM_LBUTTONDOWN) {
		updateselect = false;
	}
	return CallWindowProcW(oldeditproc, windowhandle, msg, wparam, lparam);
}