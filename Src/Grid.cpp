#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <string>
#include "Grid.h"
#include "Storage.h"
#include <iostream>

#define WM_EDITENTER (WM_APP + 0)
#define EM_CELLCLICKED (WM_APP + 1)

const COLORREF headercolor = RGB(181,251,193);

const int sheetwidth = 32; //width of spreadsheet in cells
const int sheetheight = 64; //height of spreadsheet in cells

static LRESULT CALLBACK editproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam);
static WNDPROC oldeditproc;
static HWND EditWindow;
static int EditX;
static int EditY;
static bool updateselect;

static enum {none,horz,vert} dragmode;
static int dragindex;
static int initdragpos;
static int prevdragpos;
const int clickrange = 4; //range of position before or after line where click would detect

static bool rangedrag;
static enum { mergebegin, mergedrag, mergenone, mergesplit } mergemode;
static int initX; //initial drag position and index in worksheet space
static int initY;
static int initXpos;
static int initYpos;
static int prevX1; //previous position of drag square in screen space, used for erasing
static int prevX2;
static int prevY1;
static int prevY2;

static unsigned short rowwidth[sheetheight];
static unsigned short columnwidth[sheetwidth];
static int rowheaderwidth = 35;
static int columnheaderwidth = 35;

static inline int CeilDiv(int n, int d) { //divison such that the ceiling of the quotient is returned (if numerator and denominator have the same sign)
	return ((n - 1) / d) + 1;
}

static inline int FloorDiv(int n, int d) { //divison such that the floor of the quotient is returned (if numerator and denominator have the same sign)
	return n/d;
}

static WorkSheet OneWkst;
WorkSheet* WorkSheet::CurrentWorksheet = &OneWkst;


static WCHAR* IndToCol(unsigned column,unsigned row) {
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
	
	 WCHAR* offset = Index + 1;
	 Index = Buffer;
	
	while (true) {
		*(Index) = *(offset);
		if (*Index == '\0')break;
		Index++;
		offset++;
	};
	
	return Buffer;
}

static WCHAR* IndToCol(unsigned column) {
	column++; //convert from zero base to one base
	const size_t buffsize = 20;
	WCHAR *Buffer = new WCHAR[buffsize];
	*(Buffer + buffsize - 1) = L'\0';
	WCHAR* Index = Buffer + buffsize - 2;

	while (column) {
		unsigned mod = (column - 1) % 26;
		*(Index--) = L'A' + mod;
		column = (column - mod) / 26;
	};

	WCHAR* offset = Index + 1;
	Index = Buffer;
	
	while (true) {
		*(Index) = *(offset);
		if (*Index == '\0')break;
		Index++;
		offset++;	
	};
	return Buffer;
}

static void InvertRect(HDC dc, int x1, int y1, int x2, int y2) {
	const int t = 4; //thickness
	const int o = 4; //offset
	BitBlt(dc, x1 - o, y1    , t              , y2 - y1    , dc, x1 - o, y1    , NOTSRCCOPY); //left edge
	BitBlt(dc, x1 - o, y1 - o, x2 - x1 + o + o, t          , dc, x1 - o, y1 - o, NOTSRCCOPY); //top edge
	BitBlt(dc, x1 - o, y2    , x2 - x1 + o + o, t          , dc, x1 - o, y2    , NOTSRCCOPY); //bottom edge
	BitBlt(dc, x2    , y1    , t              , y2 - y1    , dc, x2    , y1    , NOTSRCCOPY); //right edge

	//note: we have to make sure that these edge not overlap but also completely surround the square to not have gaps in the corners
}


LRESULT CALLBACK gridwndproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam) {

	static HFONT spFont = CreateFontW(26, 10, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Times New Roman");
	
	switch (msg) {
	case WM_CREATE: {
		dragmode = none;
		rangedrag = false;
		mergemode = mergenone;
		for (unsigned i = 0; i < sizeof(rowwidth) / sizeof(unsigned short); i++) {
			rowwidth[i] = 35;
		}

		for (unsigned i = 0; i < sizeof(columnwidth) / sizeof(unsigned short); i++) {
			columnwidth[i] = 125;
		}

		columnwidth[0] = 27;
		columnwidth[5] = 300;
		columnwidth[sheetwidth - 1] = 300;

		rowwidth[0] = 27;
		rowwidth[5] = 50;
		rowwidth[sheetheight - 1] = 50;

		int Totalheight = 0;
		for (unsigned i = 0; i < sizeof(rowwidth) / sizeof (unsigned short); i++) {
			Totalheight += rowwidth[i];
		}

		int Totalwidth = 0;
		for (unsigned i = 0; i < sizeof(columnwidth) / sizeof(unsigned short); i++) {
			Totalwidth += columnwidth[i];
		}

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
		si.nMax = Totalwidth;
		si.nPage = clientarea.right - rowheaderwidth;
		SetScrollInfo(windowhandle, SB_HORZ, &si, TRUE);
		
		//config vertical scroll
		si.nMax = Totalheight;
		si.nPage = clientarea.bottom - columnheaderwidth;
		SetScrollInfo(windowhandle, SB_VERT, &si, TRUE);
		
		//Insert Placeholder data into spreadsheet
		OneWkst.SetCell(L"Hello", 6, 0);
		OneWkst.SetCell(L"This is a merged Cell", 7, 0);
		OneWkst.SetCell(L"lalal", 8, 8);
		OneWkst.SetCell(L"هلا بالخميس", 0, 0);
		OneWkst.SetCell(L"Another Merged Cell!",0,20);
		OneWkst.SetCell(L"Yet Another Merged Cell!", 0, 21);

		//merge some cells
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 0,5,0,0 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 0,5,1,1 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 0,5,4,4 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 0,5,20,20 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 0,5,21,21 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 6,6,0,5 });
		OneWkst.MergedCells.push_back(WorkSheet::MergedCell{ 11,11,1,6 });

		//Config Font
		SelectObject(DeviceContext, spFont);
		SetBkMode(DeviceContext, TRANSPARENT);
		
		ReleaseDC(windowhandle, DeviceContext);
		break;
	}
	case WM_VSCROLL: {
		const WORD loword = LOWORD(wparam);
		if (loword == SB_THUMBTRACK || loword == SB_THUMBPOSITION || loword == SB_BOTTOM || loword == SB_LINEDOWN || loword == SB_LINEUP) {
			RECT clientarea;
			GetClientRect(windowhandle, &clientarea);
			clientarea.top += columnheaderwidth;

			SCROLLINFO vsi;
			vsi.cbSize = sizeof(vsi);
			vsi.fMask = SIF_TRACKPOS | SIF_POS | SIF_PAGE | SIF_RANGE;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);

			switch (loword) {
			case SB_BOTTOM: {
				vsi.nTrackPos = vsi.nPos - (int)(wparam >> 16);
				break;
			}
			case SB_LINEDOWN: {
				vsi.nTrackPos = vsi.nPos + 25;
				break;
			}
			case SB_LINEUP: {
				vsi.nTrackPos = vsi.nPos - 25;
				break;
			}
			}

			if (vsi.nTrackPos > vsi.nMax - (int)vsi.nPage - 1) {
				vsi.nTrackPos = vsi.nMax - (int)vsi.nPage - 1;
			}
			else if (vsi.nTrackPos < 0) {
				vsi.nTrackPos = 0;
			}

			//before applying the vertical scroll, erase the drag line, if it exits
			SCROLLINFO hsi;
			hsi.cbSize = sizeof(hsi);
			hsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

			HDC dc = GetDC(windowhandle);
			if (dragmode == vert) {
				int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth;
				BitBlt(dc, 0, csprevdragpos, clientarea.right, 2, dc, 0, csprevdragpos, NOTSRCCOPY);
			}
			else if (dragmode == horz) {
				int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth;
				BitBlt(dc, csprevdragpos, 0, 2, clientarea.bottom, dc, csprevdragpos, 0, NOTSRCCOPY);
			}
			else if (rangedrag == true) {
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
			}
			
			//apply the vertical scroll
			ScrollWindow(windowhandle, 0, vsi.nPos - vsi.nTrackPos, NULL, &clientarea);
			
			vsi.nPos = vsi.nTrackPos; //Update Scroll Position
			SetScrollInfo(windowhandle, SB_VERT, &vsi, TRUE);

			//now draw the drag line again
			if (dragmode == vert) {
				int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth;
				BitBlt(dc, 0, csprevdragpos, clientarea.right, 2, dc, 0, csprevdragpos, NOTSRCCOPY);
			}
			else if (dragmode == horz) {
				int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth;
				BitBlt(dc, csprevdragpos, 0, 2, clientarea.bottom, dc, csprevdragpos, 0, NOTSRCCOPY);
			}
			else if (rangedrag == true) {
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
			}
			ReleaseDC(windowhandle, dc);
			
			if(dragmode != none || rangedrag == true){ //update the "position" of the cursor if the anything is being draged
				POINT p;
				GetCursorPos(&p);
				MapWindowPoints(NULL, windowhandle, &p, 1);
				PostMessageW(windowhandle, WM_MOUSEMOVE, NULL, (p.y << 16) | p.x);
			}

		}
		break;
	}
	case WM_HSCROLL: {
		const WORD loword = LOWORD(wparam);
		if (loword == SB_THUMBTRACK || loword == SB_THUMBPOSITION || loword == SB_RIGHT || loword == SB_LINELEFT || loword == SB_LINERIGHT) {
			RECT clientarea;
			GetClientRect(windowhandle, &clientarea);
			clientarea.left += rowheaderwidth;

			SCROLLINFO hsi;
			hsi.cbSize = sizeof(hsi);
			hsi.fMask = SIF_TRACKPOS | SIF_POS | SIF_RANGE | SIF_PAGE;
			GetScrollInfo(windowhandle, SB_HORZ, &hsi);
			
			switch (loword) {
			case SB_RIGHT: {
				hsi.nTrackPos = hsi.nPos - (int)(wparam >> 16);
				break;
			}
			case SB_LINERIGHT: {
				hsi.nTrackPos = hsi.nPos + 25;
				break;
			}
			case SB_LINELEFT: {
				hsi.nTrackPos = hsi.nPos - 25;
				break;
			}
			}

			if (hsi.nTrackPos > hsi.nMax - (int)hsi.nPage - 1) {
				hsi.nTrackPos = hsi.nMax - (int)hsi.nPage - 1;
			}
			else if(hsi.nTrackPos < 0){
				hsi.nTrackPos = 0;
			}

			//before applying the vertical scroll, erase the drag line, if it exits
			SCROLLINFO vsi;
			vsi.cbSize = sizeof(vsi);
			vsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);

			HDC dc = GetDC(windowhandle);
			if (dragmode == vert) {
				int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth;
				BitBlt(dc, 0, csprevdragpos, clientarea.right, 2, dc, 0, csprevdragpos, NOTSRCCOPY);
			}
			else if (dragmode == horz) {
				int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth;
				BitBlt(dc, csprevdragpos, 0, 2, clientarea.bottom, dc, csprevdragpos, 0, NOTSRCCOPY);
			}
			else if (rangedrag == true) {
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
			}
			
			//apply the horizontal scroll
			ScrollWindow(windowhandle, hsi.nPos - hsi.nTrackPos, 0 , NULL, &clientarea);

			hsi.nPos = hsi.nTrackPos; //Update Scroll Position
			SetScrollInfo(windowhandle, SB_HORZ, &hsi, TRUE);

			//now draw the drag line again
			if (dragmode == vert) {
				int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth;
				BitBlt(dc, 0, csprevdragpos, clientarea.right, 2, dc, 0, csprevdragpos, NOTSRCCOPY);
			}
			else if (dragmode == horz) {
				int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth;
				BitBlt(dc, csprevdragpos, 0, 2, clientarea.bottom, dc, csprevdragpos, 0, NOTSRCCOPY);
			}
			else if (rangedrag == true) {
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
			}
			ReleaseDC(windowhandle, dc);
			
			if(dragmode != none || rangedrag == true){ //update the "position" of the cursor if the anything is being draged
				POINT p;
				GetCursorPos(&p);
				MapWindowPoints(NULL, windowhandle, &p, 1);
				PostMessageW(windowhandle, WM_MOUSEMOVE, NULL, (p.y << 16) | p.x);
			}
		}
		break;
	}
	case WM_PAINT: {	
		RECT clientarea;
		GetClientRect(windowhandle, &clientarea);
		
		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS;												hsi.fMask = SIF_POS;
		GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);
		
		/*Handling the resizing of the row header based on scrolling*/
		{
			unsigned rowind = 0;
			int ypos = 0;
			while (ypos < clientarea.bottom + vsi.nPos - columnheaderwidth) {
				ypos += rowwidth[rowind];
				rowind++;
			}

			const unsigned digitwidth = 14; //width in pixel to be added per digit
			unsigned newrowheaderwidth = (35 - digitwidth) + digitwidth * (unsigned)log10((double)rowind); //compute what is supposed to be the width of the row header currently

			if (newrowheaderwidth != rowheaderwidth) {
				const int scrollamt = newrowheaderwidth - rowheaderwidth;
				rowheaderwidth = newrowheaderwidth;

				//decrease the size of the page horizontally
				hsi.fMask = SIF_PAGE;
				GetScrollInfo(windowhandle, SB_HORZ, &hsi);
				hsi.nPage -= scrollamt;
				SetScrollInfo(windowhandle, SB_HORZ, &hsi, TRUE);

				//basically make it so the entire screen is redrawn
				InvalidateRect(windowhandle, NULL, TRUE); //not sure why TRUE is needed here...

				//move edit Control if exsists
				if (EditWindow) {
					RECT er; GetWindowRect(EditWindow, &er);
					MapWindowPoints(HWND_DESKTOP, windowhandle, (LPPOINT)&er, 2); //appearantly GetWindowRect returns coordiates of window in screen space so must convert to client space
					er.left += scrollamt;
					er.right += scrollamt;
					MoveWindow(EditWindow, er.left, er.top, er.right - er.left, er.bottom - er.top, FALSE);
				}
			}

		}
		
		/*Officially Begin Painting*/
		PAINTSTRUCT pts;
		HDC PaintDC = BeginPaint(windowhandle, &pts);
		RECT updaterect = pts.rcPaint;

		/*Drawing The text in cells*/
		{
			unsigned minXi = 0, minYi = 0; //indexes
			int minXp = 0, minYp = 0; //positions
			while (minXp + columnwidth[minXi] < hsi.nPos + updaterect.left - rowheaderwidth) {
				minXp += columnwidth[minXi];
				minXi++;
			}
			minXp = minXp - hsi.nPos + rowheaderwidth;

			while (minYp + rowwidth[minYi] < vsi.nPos + updaterect.top - columnheaderwidth) {
				minYp += rowwidth[minYi];
				minYi++;
			}
			minYp = minYp - vsi.nPos + columnheaderwidth;

			for (int Yp = minYp, Yi = minYi; Yp <= updaterect.bottom; Yp += rowwidth[Yi], Yi++) {
				for (int Xp = minXp, Xi = minXi; Xp <= updaterect.right; Xp += columnwidth[Xi], Xi++) {
					bool intersectmerge = false;
					int Xi2;
					int Yi2;
					
					for (auto i : OneWkst.MergedCells) {
						if ((int)i.indX1 <= Xi && Xi <= (int)i.indX2 && (int)i.indY1 <= Yi && Yi <= (int)i.indY2) {
							intersectmerge = true;
							Xi2 = i.indX2;
							Yi2 = i.indY2;
							break;
						}
					}

					WorkSheet::cell read = OneWkst.GetCell(Xi, Yi);
					
					if (!intersectmerge) {
						if (read.type != WorkSheet::cell::type::null) {
							RECT cellRect = { Xp,Yp,Xp + columnwidth[Xi] ,Yp + rowwidth[Yi] };
							DrawTextW(PaintDC, read.tt.c_str(), -1, &cellRect, DT_LEFT | DT_SINGLELINE | DT_BOTTOM);
						}
					}
					else {
						for (; Xi < Xi2; Xi++) { //horizontally skip the mergecell
							Xp += columnwidth[Xi];
						}
					}
				}
			}

			//draw the text to be displayed in merged cells
			for (auto i : OneWkst.MergedCells) {
				if (i.indX2 >= minXi && i.indY2 >= minYi) {
					int trackX1p, trackX2p;
					int trackY1p, trackY2p;
					unsigned trackXi, trackYi;

					//---Right---
					trackX2p = minXp;
					trackXi = minXi;
					if (trackXi < i.indX2 + 1) {
						while (trackXi != i.indX2 + 1) {
							trackX2p += columnwidth[trackXi];
							trackXi++;
						}
					}
					else {
						while (trackXi != i.indX2 + 1) {
							trackX2p -= columnwidth[trackXi - 1];
							trackXi--;
						}
					}

					//---Left---
					trackX1p = trackX2p;
					while (trackXi > i.indX1) {
						trackX1p -= columnwidth[trackXi - 1];
						trackXi--;
					}

					//---Bottom---
					trackY2p = minYp;
					trackYi = minYi;
					if (trackYi < i.indY2 + 1) {
						while (trackYi != i.indY2 + 1) {
							trackY2p += rowwidth[trackYi];
							trackYi++;
						}
					}
					else {
						while (trackYi != i.indY2 + 1) {
							trackY2p -= rowwidth[trackYi - 1];
							trackYi--;
						}
					}

					//---Top---
					trackY1p = trackY2p;
					while (trackYi > i.indY1) {
						trackY1p -= rowwidth[trackYi - 1];
						trackYi--;
					}

					//--Draw---
					WorkSheet::cell read = OneWkst.GetCell(trackXi, trackYi);
					if (read.type != WorkSheet::cell::type::null) {
						RECT cellRect = { trackX1p, trackY1p, trackX2p, trackY2p };
						DrawTextW(PaintDC, read.tt.c_str(), -1, &cellRect, DT_CENTER | DT_SINGLELINE | DT_BOTTOM);
					}
				}
			}
		}

		/*Borders of the row/column headers*/
		{ 
			
			if(updaterect.top < columnheaderwidth - 1 && updaterect.bottom > columnheaderwidth - 1){
				POINT Horzline[2] = { {clientarea.left,columnheaderwidth - 1},{clientarea.right,columnheaderwidth - 1} };
				Polyline(PaintDC, Horzline, 2);
			}

			if (updaterect.left < rowheaderwidth - 1 && updaterect.right > rowheaderwidth - 1) {
				POINT Vertline[2] = { {rowheaderwidth - 1,clientarea.top},{rowheaderwidth - 1,clientarea.bottom} };
				Polyline(PaintDC, Vertline, 2);
			}
		}
		/*Color of the row/column headings*/ 
		HBRUSH headerbrush = CreateSolidBrush(headercolor);
		RECT headerarea;
		if (updaterect.top < columnheaderwidth) { //coloring column header if in update rect
			headerarea.left = rowheaderwidth;
			headerarea.top = 0;
			headerarea.right = clientarea.right;
			headerarea.bottom = columnheaderwidth - 2;
			FillRect(PaintDC, &headerarea, headerbrush);
		}

		if (updaterect.left < rowheaderwidth) { //coloring row header if in update rect
			headerarea.left = 0;
			headerarea.top = columnheaderwidth;
			headerarea.right = rowheaderwidth - 2;
			headerarea.bottom = clientarea.bottom;
			FillRect(PaintDC, &headerarea, headerbrush);
		}
		/*drawing column headers*/{
			int posY = 0;
			int indY = 0;
			while (posY + rowwidth[indY] < vsi.nPos) {
				posY += rowwidth[indY];
				indY++;
			}
			
			int sumpos = columnwidth[0] + rowheaderwidth;
			int index = 1;
			while (sumpos <= hsi.nPos + clientarea.right && index < sheetwidth) {
				if (sumpos - hsi.nPos >= rowheaderwidth) {
					//Drawing the line
					{
						POINT ColHeaderSep[2] = { {sumpos - hsi.nPos,0},{sumpos - hsi.nPos,columnheaderwidth - 1} };
						Polyline(PaintDC, ColHeaderSep, 2);
					}
					
					std::vector<unsigned> intersect; /*Get list of Merged Cells that intersect*/
					for (int i = 0; i < OneWkst.MergedCells.size(); i++) {
						if (OneWkst.MergedCells[i].indX1 < (unsigned)index && (unsigned)index <= OneWkst.MergedCells[i].indX2) {
							intersect.push_back(i);
						}
					}

					//the following variables will be used to keep track while drawing the line
					int currinkp = posY - vsi.nPos + columnheaderwidth; //in screen space
					int previnkp = columnheaderwidth; //in screen space
					unsigned int currinki = indY;
					bool intersects = true; //redundant initialization
					bool previntersects = false;
					
					while (true) {
						if (currinkp < clientarea.bottom) { //this also checks for intersection at the bottom/right of the screen
							intersects = false;
							for (auto i : intersect) {
								if (OneWkst.MergedCells[i].indY1 <= currinki && currinki <= OneWkst.MergedCells[i].indY2) {
									intersects = true;
									break;
								}
							}
						}
						else {
							intersects = true;
						}
						
						if (currinkp > columnheaderwidth) { //this check is done since currinkp begins ABOVE the column header, might be able to optimize this by making a version of the loop that only executes once before this one
							if (!intersects && previntersects) { //the if else must execute in this order or else merged cells at the bottom of the screen will be drawn unmerged 😔
								previnkp = currinkp;
							}
							else if (intersects && !previntersects) {
								POINT ColHeaderSep[2] = { {sumpos - hsi.nPos,previnkp},{sumpos - hsi.nPos,currinkp} };
								Polyline(PaintDC, ColHeaderSep, 2);
							}
						}

						if (currinkp > clientarea.bottom) break;
						currinkp += rowwidth[currinki];
						currinki++;
						previntersects = intersects;
					}

					if (updaterect.top < columnheaderwidth) { //only draw the column letters if they are in the update region
						RECT TextRect = { sumpos - hsi.nPos - columnwidth[index - 1],0,sumpos - hsi.nPos,columnheaderwidth - 1 };
						WCHAR* ColumnName = IndToCol(index - 1);
						DrawTextW(PaintDC, ColumnName, -1, &TextRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
						delete[] ColumnName;
					}
				}

				sumpos += columnwidth[index];
				index++;
			}

			if (updaterect.top < columnheaderwidth) { //only draw the column letters if they are in the update region
				RECT TextRect = { sumpos - hsi.nPos - columnwidth[index - 1],0,sumpos - hsi.nPos,columnheaderwidth - 1 };
				WCHAR* ColumnName = IndToCol(index - 1);
				DrawTextW(PaintDC, ColumnName, -1, &TextRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				delete[] ColumnName;
			}

		}
		/*drawing row headers*/ {
			int posX = 0;
			int indX = 0;
			while (posX + columnwidth[indX] < hsi.nPos) {
				posX += columnwidth[indX];
				indX++;
			}
			
			int sumpos = rowwidth[0] + columnheaderwidth;
			int index = 1;
			while (sumpos <= vsi.nPos + clientarea.bottom && index < sheetheight) {
				if (sumpos - vsi.nPos >= columnheaderwidth) {
					//Drawing the line
					{
						POINT ColHeaderSep[2] = { {0, sumpos - vsi.nPos},{rowheaderwidth - 1, sumpos - vsi.nPos} };
						Polyline(PaintDC, ColHeaderSep, 2);
					}

					std::vector<unsigned> intersect; /*Get list of Merged Cells that intersect*/
					for (int i = 0; i < OneWkst.MergedCells.size(); i++) {
						if (OneWkst.MergedCells[i].indY1 < (unsigned)index && (unsigned)index <= OneWkst.MergedCells[i].indY2) {
							intersect.push_back(i);
						}
					}

					//the following variables will be used to keep track while drawing the line
					int currinkp = posX - hsi.nPos + rowheaderwidth; //in screen space
					int previnkp = rowheaderwidth - 1; //in screen space (for some reason the horizontal version needs the minus -1)
					unsigned int currinki = indX;
					bool intersects = true; //redundant initialization
					bool previntersects = false;

					while (true) {
						if (currinkp < clientarea.right) { //this also checks for intersection at the bottom/right of the screen
							intersects = false;
							for (auto i : intersect) {
								if (OneWkst.MergedCells[i].indX1 <= currinki && currinki <= OneWkst.MergedCells[i].indX2) {
									intersects = true;
									break;
								}
							}
						}
						else {
							intersects = true;
						}

						if (currinkp > rowheaderwidth) { //this check is done since currinkp begins ABOVE the column header, might be able to optimize this by making a version of the loop that only executes once before this one
							if (!intersects && previntersects) {
								previnkp = currinkp;
							}
							else if (intersects && !previntersects) {
								POINT ColHeaderSep[2] = { {previnkp, sumpos - vsi.nPos},{currinkp, sumpos - vsi.nPos} };
								Polyline(PaintDC, ColHeaderSep, 2);
							}
						}

						if (currinkp > clientarea.right) break;
						currinkp += columnwidth[currinki];
						currinki++;
						previntersects = intersects;
					}

					if (updaterect.left < rowheaderwidth) { //only draw the row numbers if they are in the update region
						RECT TextRect = { 0,sumpos - vsi.nPos - rowwidth[index - 1],rowheaderwidth - 1, sumpos - vsi.nPos };
						DrawTextW(PaintDC, std::to_wstring(index - 1).c_str(), -1, &TextRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
					}
				}

				sumpos += rowwidth[index];
				index++;
			}

			if (updaterect.left < rowheaderwidth) { //only draw the row numbers if they are in the update region
				RECT TextRect = { 0,sumpos - vsi.nPos - rowwidth[index - 1],rowheaderwidth - 1, sumpos - vsi.nPos };
				DrawTextW(PaintDC, std::to_wstring(index - 1).c_str(), -1, &TextRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}

		}
		/*coloring that top-left corner(we do it after drawing the row and column headers so the letters dont "overflow" to the top left area)*/
		if (updaterect.left < rowheaderwidth && updaterect.top < columnheaderwidth) { //coloring row header if in update rect
			headerarea.left = 0;
			headerarea.top = 0;
			headerarea.right = rowheaderwidth - 2;
			headerarea.bottom = columnheaderwidth - 2;
			FillRect(PaintDC, &headerarea, headerbrush);
		}
		DeleteObject(headerbrush);

		/*draw drag line if exists*/
		{
			if (dragmode == vert) {
				int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth;
				BitBlt(PaintDC, 0, csprevdragpos, clientarea.right, 2, PaintDC, 0, csprevdragpos, NOTSRCCOPY);
			}
			else if (dragmode == horz) {
				int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth;
				BitBlt(PaintDC, csprevdragpos, 0, 2, clientarea.bottom, PaintDC, csprevdragpos, 0, NOTSRCCOPY);
			}
			else if (rangedrag == true) {
				InvertRect(PaintDC, prevX1, prevY1, prevX2, prevY2);
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

		int mx = GET_X_LPARAM(lparam), my = GET_Y_LPARAM(lparam); //mousex and mousey
		
		bool clickedwithingrid = mx > rowheaderwidth && my > columnheaderwidth;
		int Xind = 0,Yind = 0,Xpos = 0,Ypos = 0;
		RECT tofill;
		int mergeIndex = -1; //index of intersected merged cell in the OneWkst.MergedCells list,
		if (clickedwithingrid) {
			while (Xpos + columnwidth[Xind] < mx + hsi.nPos - rowheaderwidth) {
				Xpos += columnwidth[Xind];
				Xind++;
			}
			Xpos -= hsi.nPos - rowheaderwidth;

			while (Ypos + rowwidth[Yind] < my + vsi.nPos - columnheaderwidth) {
				Ypos += rowwidth[Yind];
				Yind++;
			}
			Ypos -= vsi.nPos - columnheaderwidth;

			//checking for merged cells
			int Xpos2 = Xpos + columnwidth[Xind];
			int Ypos2 = Ypos + rowwidth[Yind];

			for (unsigned i = 0; i < OneWkst.MergedCells.size(); i++) {
				auto& mcell = OneWkst.MergedCells[i];
				if (mcell.indX1 <= Xind && mcell.indX2 >= Xind && mcell.indY1 <= Yind && mcell.indY2 >= Yind) {
					mergeIndex = i;
					while (Xind > mcell.indX1) {
						Xpos -= columnwidth[Xind - 1];
						Xind--;
					}

					while (Yind > mcell.indY1) {
						Ypos -= rowwidth[Yind - 1];
						Yind--;
					}

					int Xind2 = Xind;
					int Yind2 = Yind;
					Xpos2 = Xpos;
					Ypos2 = Ypos;
					while (Xind2 <= mcell.indX2) {
						Xpos2 += columnwidth[Xind2];
						Xind2++;
					}

					while (Yind2 <= mcell.indY2) {
						Ypos2 += rowwidth[Yind2];
						Yind2++;
					}

					break;
				}
			}

			tofill = { Xpos + 1,Ypos + 1,Xpos2 - 1,Ypos2 - 1 };
		}
		
		WCHAR* WindowContent = new WCHAR[GetWindowTextLengthW(EditWindow) + (size_t)1/*null terminator*/];
		if (!WindowContent) MessageBoxA(NULL, "Allocation Failure", "Allocation Failure", MB_ICONERROR);
		if (EditWindow != NULL) { GetWindowTextW(EditWindow, WindowContent, INT_MAX); }

		if (clickedwithingrid) {			
			unsigned start;
			unsigned end;
			SendMessageW(EditWindow, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			start = std::min(start, end);

			if (mergemode == mergebegin) {
				initX = Xind;
				initY = Yind;
				initXpos = Xpos + hsi.nPos - rowheaderwidth; //should convert to worksheet space
				initYpos = Ypos + vsi.nPos - columnheaderwidth;
				mergemode = mergedrag;
				//Draw rectangle and set the prev coords
				prevX1 = tofill.left;
				prevY1 = tofill.top;
				prevX2 = tofill.right;
				prevY2 = tofill.bottom;

				HDC dc = GetDC(windowhandle);
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
				ReleaseDC(windowhandle, dc);
			}
			else if (mergemode == mergesplit) {
				if (mergeIndex != -1) {
					OneWkst.MergedCells.erase(std::next(OneWkst.MergedCells.begin(), mergeIndex));
					RECT clientrect; GetClientRect(windowhandle, &clientrect);
					clientrect.top += columnheaderwidth;
					clientrect.left += rowheaderwidth;
					InvalidateRect(windowhandle, &clientrect, TRUE);
				}
				mergemode = mergenone; //reset after clicking
			}
			else if ((EditWindow == NULL || WindowContent[0] != L'=' || start == 0)) {
				if (EditWindow != NULL) { //apply and destroy old window if there exists an edit window;
					(void)OneWkst.SetCell(WindowContent, EditX, EditY);
					RECT clientrect; GetClientRect(windowhandle, &clientrect);
					clientrect.top += columnheaderwidth;
					clientrect.left += rowheaderwidth;
					InvalidateRect(windowhandle, &clientrect, TRUE);
					DestroyWindow(EditWindow);
				}

				HWND editctrl = CreateWindowW(L"EDIT", L"Enter Text",
					WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
					tofill.left, tofill.top, tofill.right - tofill.left, tofill.bottom - tofill.top,
					windowhandle, (HMENU)0,
					(HINSTANCE)GetWindowLongPtrW(windowhandle, GWLP_HINSTANCE), NULL);

				oldeditproc = (WNDPROC)SetWindowLongPtrW(editctrl, GWLP_WNDPROC, (LONG_PTR)editproc);
				EditWindow = editctrl;
				EditX = Xind;
				EditY = Yind;

				SetWindowTextW(editctrl, OneWkst.GetCell(Xind, Yind).ExactInput.c_str());
				SetFocus(editctrl);
				SendMessageW(editctrl, WM_SETFONT, (WPARAM)spFont, TRUE);
			}
			else {
				unsigned Start;
				unsigned End;
				SendMessageW(EditWindow, EM_GETSEL, (WPARAM)&Start,(LPARAM)&End);
				Start = std::min(Start, End);

				initX = Xind;
				initY = Yind;
				initXpos = Xpos + hsi.nPos - rowheaderwidth; //should convert to worksheet space
				initYpos = Ypos + vsi.nPos - columnheaderwidth;
				rangedrag = true;
				//Draw rectangle and set the prev coords
				prevX1 = tofill.left;
				prevY1 = tofill.top;
				prevX2 = tofill.right;
				prevY2 = tofill.bottom;

				HDC dc = GetDC(windowhandle);
				InvertRect(dc, prevX1, prevY1, prevX2, prevY2);
				ReleaseDC(windowhandle, dc);

				WCHAR* strptr = IndToCol(Xind,Yind);
				SendMessageW(EditWindow, EM_REPLACESEL, FALSE, (LPARAM)strptr);
				SendMessageW(EditWindow, EM_SETSEL, Start, Start + std::wcslen(strptr));
				updateselect = true;
				delete[] strptr;
			}
		}
		else {
			RECT clientrect; GetClientRect(windowhandle, &clientrect);
			if (EditWindow != NULL) { //apply edit if exist
				(void)OneWkst.SetCell(WindowContent, EditX, EditY);
				clientrect.top += columnheaderwidth;
				clientrect.left += rowheaderwidth;
				InvalidateRect(windowhandle, &clientrect, TRUE);
				DestroyWindow(EditWindow);
				EditWindow = NULL;
			}
			//test for column drag
			if (my < columnheaderwidth && mx > rowheaderwidth) {
				Xpos = columnwidth[0];
				Xind = 0;
				while (true) {
					if (abs(Xpos - (mx - rowheaderwidth + hsi.nPos)) <= clickrange) {
						dragmode = horz;
						SetCapture(windowhandle);
						const int LineX = Xpos - hsi.nPos + rowheaderwidth; //X position in client space
						dragindex = Xind;
						initdragpos = Xpos;
						prevdragpos = Xpos;
						HDC devicecontext = GetDC(windowhandle);
						BitBlt(devicecontext, LineX, 0, 2, clientrect.bottom, devicecontext, LineX, 0, NOTSRCCOPY);
						ReleaseDC(windowhandle, devicecontext);
						break;
					}
					else if (Xpos > (mx - rowheaderwidth + hsi.nPos) || Xind == sheetwidth) {
						break;
					}
					Xind++;
					Xpos += columnwidth[Xind];
				}
			}

			//test for row drag
			if (mx < rowheaderwidth && my > columnheaderwidth) {
				Ypos = rowwidth[0];
				Yind = 0;
				while (true) {
					if (abs(Ypos - (my - columnheaderwidth + vsi.nPos)) <= clickrange) {
						dragmode = vert;
						SetCapture(windowhandle);
						const int LineY = Ypos - vsi.nPos + columnheaderwidth; //Y position in client space
						dragindex = Yind;
						initdragpos = Ypos;
						prevdragpos = Ypos;
						HDC devicecontext = GetDC(windowhandle);
						BitBlt(devicecontext, 0, LineY, clientrect.right, 2, devicecontext, 0, LineY, NOTSRCCOPY);
						ReleaseDC(windowhandle, devicecontext);
						break;
					}
					else if (Ypos > (my - columnheaderwidth + vsi.nPos) || Yind == sheetheight) {
						break;
					}
					Yind++;
					Ypos += rowwidth[Yind];
				}
			}
			
		}
		if (WindowContent)delete[] WindowContent;
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

		WCHAR* Buffer = new WCHAR[GetWindowTextLengthW(editctrl) + (size_t)1];
		GetWindowTextW(editctrl, Buffer, INT_MAX);

		unsigned SetCellInfo = OneWkst.SetCell(Buffer, EditX, EditY);

		if (SetCellInfo == SET_CELL_ERR_CIRCULAR_REF) { //Set Cell fails
			MessageBoxW(windowhandle, L"The cell directly or indirectly refrences itself in the formula", L"Self Reference", MB_OK | MB_ICONERROR);
			SetFocus(editctrl);
		}
		else { //Set Cell Succeedes
			SetFocus(windowhandle);
			RECT clientrect; GetClientRect(windowhandle, &clientrect);
			clientrect.top += columnheaderwidth;
			clientrect.left += rowheaderwidth;
			InvalidateRect(windowhandle, &clientrect, TRUE);
			DestroyWindow(editctrl);
			EditWindow = NULL;
		}
		delete[] Buffer;
		break;
	}
	case WM_SIZE: {
		RECT clientrect;
		GetClientRect(windowhandle, &clientrect);

		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);							hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_PAGE;							hsi.fMask = SIF_PAGE;
		
		vsi.nPage = clientrect.bottom - columnheaderwidth;
		hsi.nPage = clientrect.right - rowheaderwidth;

		SetScrollInfo(windowhandle,SB_HORZ,&hsi,FALSE);
		SetScrollInfo(windowhandle, SB_VERT, &vsi, FALSE);
		InvalidateRect(windowhandle, &clientrect, FALSE);
		break;
	}
	case WM_MOUSEMOVE: {
		int mx, my; //mousex and mousey
		RECT clientrect; GetClientRect(windowhandle, &clientrect);
		if (dragmode == horz) {
			mx = GET_X_LPARAM(lparam);//mousex
			
			SCROLLINFO hsi; //vertical scroll info
			hsi.cbSize = sizeof(hsi);
			hsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

			int csinitdragpos = initdragpos - hsi.nPos + rowheaderwidth; //init drag pos in client space
			int csprevdragpos = prevdragpos - hsi.nPos + rowheaderwidth; //prev drag pos in client space
			
			if (mx < csinitdragpos - columnwidth[dragindex] + clickrange) mx = csinitdragpos - columnwidth[dragindex] + clickrange; //so that the line isnt drawn at the area which has the row/column header
			HDC devicecontext = GetDC(windowhandle);
			BitBlt(devicecontext, csprevdragpos, 0, 2, clientrect.bottom, devicecontext, csprevdragpos, 0, NOTSRCCOPY); //erase previous position
			BitBlt(devicecontext, mx, 0, 2, clientrect.bottom, devicecontext, mx, 0, NOTSRCCOPY); //draw at current position
			ReleaseDC(windowhandle, devicecontext);
			prevdragpos = mx + hsi.nPos - rowheaderwidth;
		}
		else if (dragmode == vert) {
			my = GET_Y_LPARAM(lparam);//mousey
			
			SCROLLINFO vsi; //vertical scroll info
			vsi.cbSize = sizeof(vsi);
			vsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);
			
			int csinitdragpos = initdragpos - vsi.nPos + columnheaderwidth; //init drag pos in client space
			int csprevdragpos = prevdragpos - vsi.nPos + columnheaderwidth; //prev drag pos in client space

			if (my < csinitdragpos - rowwidth[dragindex] + clickrange) my = csinitdragpos - rowwidth[dragindex] + clickrange; //so that the line isnt drawn at the area which has the row/column header
			HDC devicecontext = GetDC(windowhandle);
			BitBlt(devicecontext, 0, csprevdragpos, clientrect.right, 2, devicecontext, 0, csprevdragpos, NOTSRCCOPY); //erase previous position
			BitBlt(devicecontext, 0, my, clientrect.right, 2, devicecontext, 0, my, NOTSRCCOPY);  //draw at current position
			ReleaseDC(windowhandle, devicecontext);
			prevdragpos = my + vsi.nPos - columnheaderwidth;
		}
		else if (rangedrag == true || mergemode == mergedrag) {
			mx = GET_X_LPARAM(lparam); //mouse x
			my = GET_Y_LPARAM(lparam); //mouse y

			SCROLLINFO vsi; //vertical scroll info
			SCROLLINFO hsi; //horizontal scroll info
			vsi.cbSize = sizeof(vsi);							hsi.cbSize = sizeof(hsi);
			vsi.fMask = SIF_POS;								hsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

			int Xind = 0, Yind = 0, Xpos = 0, Ypos = 0;
			while (Xpos + columnwidth[Xind] < mx + hsi.nPos - rowheaderwidth) {
				Xpos += columnwidth[Xind];
				Xind++;
			}

			while (Ypos + rowwidth[Yind] < my + vsi.nPos - columnheaderwidth) {
				Ypos += rowwidth[Yind];
				Yind++;
			}


			//check if overlapping any merged cells and expand range selection to encompass the whole merge cell
			int defactoInitXi = initX;
			int defactoInitXpos = initXpos;
			int defactoInitYi = initY;
			int defactoInitYpos = initYpos;

			//inits should be less or equal Xind
			if (initX > Xind) {
				std::swap(defactoInitXi, Xind);
				std::swap(defactoInitXpos, Xpos);
			}

			if (initY > Yind) {
				std::swap(defactoInitYi, Yind);
				std::swap(defactoInitYpos, Ypos);
			}

			//selected region should be expanded such that no merged cells are partially selected
			//This is a naive algorithm, maybe improve it later
			while (true) {
				bool regionAdjusted = false;

				for (auto& i : OneWkst.MergedCells) {
					if ((Xind >= i.indX1 && defactoInitXi <= i.indX2) && (Yind >= i.indY1 && defactoInitYi <= i.indY2)) {
						while (i.indX1 < defactoInitXi) {
							defactoInitXpos -= columnwidth[defactoInitXi - 1];
							defactoInitXi--;
							regionAdjusted = true;
						}

						while (i.indX2 > Xind) {
							Xpos += columnwidth[Xind];
							Xind++;
							regionAdjusted = true;
						}

						while (i.indY1 < defactoInitYi) {
							defactoInitYpos -= rowwidth[defactoInitYi - 1];
							defactoInitYi--;
							regionAdjusted = true;
						}

						while (i.indY2 > Yind) {
							Ypos += rowwidth[Yind];
							Yind++;
							regionAdjusted = true;
						}
					}
				}

				if (!regionAdjusted) break;
			}

			//begin handling blitting stuff
			HDC dc = GetDC(windowhandle);
			InvertRect(dc, prevX1, prevY1, prevX2, prevY2); //erase

			Ypos -= vsi.nPos - columnheaderwidth; //convert to client space
			Xpos -= hsi.nPos - rowheaderwidth; //convert to client space
			int csinitXpos = defactoInitXpos - hsi.nPos + rowheaderwidth;
			int csinitYpos = defactoInitYpos - vsi.nPos + columnheaderwidth;

			prevX1 = std::min(csinitXpos, Xpos);
			prevY1 = std::min(csinitYpos, Ypos);
			prevX2 = std::max(csinitXpos + columnwidth[initX], Xpos + columnwidth[Xind]);
			prevY2 = std::max(csinitYpos + rowwidth[initY], Ypos + rowwidth[Yind]);

			InvertRect(dc, prevX1, prevY1, prevX2, prevY2); //draw
			ReleaseDC(windowhandle, dc);
			//end handling blitting stuff

			if (rangedrag == true) {
				//note: if rangedrag is true then there is no need to check of EditWindow is null or not.
				unsigned Start;
				unsigned End;
				SendMessageW(EditWindow, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End);
				Start = std::min(Start, End);

				if (Xind == defactoInitXi && Yind == defactoInitYi) {
					WCHAR* strptr = IndToCol(Xind, Yind);

					SendMessageW(EditWindow, EM_REPLACESEL, FALSE, (LPARAM)strptr);
					SendMessageW(EditWindow, EM_SETSEL, Start, Start + std::wcslen(strptr));

					delete[] strptr;
				}
				else {
					WCHAR* strptr1 = IndToCol(defactoInitXi, defactoInitYi);
					WCHAR* strptr2 = IndToCol(Xind, Yind);
					WCHAR* strptrc = new WCHAR[128];

					wcscpy_s(strptrc, 128, strptr1);
					wcscat_s(strptrc, 128, L":");
					wcscat_s(strptrc, 128, strptr2);

					SendMessageW(EditWindow, EM_REPLACESEL, FALSE, (LPARAM)strptrc);
					SendMessageW(EditWindow, EM_SETSEL, Start, Start + std::wcslen(strptrc));

					delete[] strptr1;
					delete[] strptr2;
					delete[] strptrc;
				}
			}
		}
		break;
	}
	case WM_LBUTTONUP: {
		RECT clientrect; GetClientRect(windowhandle, &clientrect);
		if (dragmode == horz) {
			dragmode = none;
			ReleaseCapture();
			int sizechange = prevdragpos - initdragpos;

			SCROLLINFO hsi; //horizontal scroll info
			hsi.cbSize = sizeof(hsi);
			hsi.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
			GetScrollInfo(windowhandle, SB_HORZ, &hsi);
			hsi.nMax += sizechange;
			hsi.fMask = SIF_RANGE;
			SetScrollInfo(windowhandle, SB_HORZ, &hsi, TRUE);
			
			if (/*TEMPORARY FIX*//*hsi.nPos < hsi.nMax - (int)hsi.nPage*/false) { //checks if the scroll position is less than the maximum after the the scroll info is changed 
				hsi.fMask = SIF_POS; //take into account the fact that vertical scroll postion might change after resizing the cell
				GetScrollInfo(windowhandle, SB_HORZ, &hsi);
				clientrect.left = (initdragpos - hsi.nPos + rowheaderwidth) - columnwidth[dragindex]; //only invalidate to the right of changed cell
			}
			else {
				clientrect.left = rowheaderwidth; //invalidate whole grid area except row header
			}
			columnwidth[dragindex] += sizechange;
			InvalidateRect(windowhandle, &clientrect, TRUE);
		}
		else if (dragmode == vert) {
			dragmode = none;
			ReleaseCapture();
			int sizechange = prevdragpos - initdragpos;

			SCROLLINFO vsi; //vertical scroll info
			vsi.cbSize = sizeof(vsi);
			vsi.fMask = SIF_RANGE|SIF_POS|SIF_PAGE;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);
			vsi.nMax += sizechange;
			vsi.fMask = SIF_RANGE;
			SetScrollInfo(windowhandle, SB_VERT, &vsi, TRUE);
			
			if (false/*TEMPORARY FIX*//*vsi.nPos < vsi.nMax - (int)vsi.nPage*/) { //checks if the scroll position is less than the maximum after the the scroll info is changed 
				vsi.fMask = SIF_POS; //take into account the fact that vertical scroll postion might change after resizing the cell
				GetScrollInfo(windowhandle, SB_VERT, &vsi);
				clientrect.top = (initdragpos - vsi.nPos + columnheaderwidth) - rowwidth[dragindex]; //only invalidate to the bottom of changed cell
			}
			else {
				clientrect.top = columnheaderwidth; //invalidate whole grid area except column header
			}
			rowwidth[dragindex] += sizechange;
			InvalidateRect(windowhandle, &clientrect, TRUE);
		}
		else if (rangedrag == true) {
			HDC dc = GetDC(windowhandle);
			InvertRect(dc, prevX1, prevY1, prevX2, prevY2); //remove the drag rectangle
			ReleaseDC(windowhandle, dc);
			rangedrag = false;
		}
		else if (mergemode == mergedrag) {
			HDC dc = GetDC(windowhandle);
			InvertRect(dc, prevX1, prevY1, prevX2, prevY2); //remove the drag rectangle
			ReleaseDC(windowhandle, dc);
			mergemode = mergenone;

			int mx = GET_X_LPARAM(lparam); //mouse x
			int my = GET_Y_LPARAM(lparam); //mouse y

			SCROLLINFO vsi; //vertical scroll info
			SCROLLINFO hsi; //horizontal scroll info
			vsi.cbSize = sizeof(vsi);							hsi.cbSize = sizeof(hsi);
			vsi.fMask = SIF_POS;								hsi.fMask = SIF_POS;
			GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

			int Xind = 0, Yind = 0, Xpos = 0, Ypos = 0;
			while (Xpos + columnwidth[Xind] < mx + hsi.nPos - rowheaderwidth) {
				Xpos += columnwidth[Xind];
				Xind++;
			}

			while (Ypos + rowwidth[Yind] < my + vsi.nPos - columnheaderwidth) {
				Ypos += rowwidth[Yind];
				Yind++;
			}

			//dont merge if the selected region is 1x1 (i.e. a single cell)
			if (initX == Xind && initY == Yind) {
				goto skipmerge;
			}

			//inits should be less or equal Xind
			if (initX > Xind) {
				std::swap(initX, Xind);
			}

			if (initY > Yind) {
				std::swap(initY, Yind);
			}

			//check if selected region overlaps any previously existing merged cells
			for (auto& i : OneWkst.MergedCells) {
				if ((Xind >= i.indX1 && initX <= i.indX2) && (Yind >= i.indY1 && initY <= i.indY2)) {
					MessageBoxA(NULL, "The range you want to merge already contains merged cells", "Merge Error", MB_ICONERROR);
					goto skipmerge;
				}
			}

			OneWkst.MergedCells.push_back(
				WorkSheet::MergedCell{
					(unsigned)initX, (unsigned)Xind,
					(unsigned)initY, (unsigned)Yind
				}
			);

			InvalidateRect(windowhandle, NULL, TRUE);
		skipmerge:;
		}
		break;
	}
	case WM_MOUSEWHEEL: {
		int scrollamt = GET_WHEEL_DELTA_WPARAM(wparam);
		if (wparam & MK_SHIFT) {
			SendMessageW(windowhandle, WM_HSCROLL, SB_RIGHT | scrollamt << 16, NULL);
		}
		else {
			SendMessageW(windowhandle, WM_VSCROLL, SB_BOTTOM | scrollamt << 16, NULL);
		}
		break;
	}
	case WM_SAVE: {
		OPENFILENAMEW* ofn = (OPENFILENAMEW*)lparam;
		DWORD byteswritten;
		HANDLE filehandle = CreateFileW(ofn->lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN,NULL);
		WriteFile(filehandle, "eggfile",7,&byteswritten,NULL);
		
		int buffer[4];

		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS;												hsi.fMask = SIF_POS;
		GetScrollInfo(windowhandle, SB_VERT, &vsi);			GetScrollInfo(windowhandle, SB_HORZ, &hsi);

		buffer[0] = _byteswap_ulong(hsi.nPos); //little to big endian
		buffer[1] = _byteswap_ulong(vsi.nPos);
		buffer[2] = _byteswap_ulong(sheetwidth);
		buffer[3] = _byteswap_ulong(sheetheight);

		WriteFile(filehandle, buffer, 4 * 4, &byteswritten, NULL);
		
		/*RLE encoding column widths*/ {
			unsigned i = 0;
			while (i < sheetwidth) {
				unsigned short width = columnwidth[i];
				unsigned count = 0u;
				while (i < sheetwidth  && count <= UINT_MAX && columnwidth[i] == width) {
					count++;
					i++;
				}
				width = _byteswap_ushort(width); //little to big endian
				count = _byteswap_ulong(count);
				WriteFile(filehandle, &width, sizeof(width), &byteswritten, NULL);
				WriteFile(filehandle, &count, sizeof(count), &byteswritten, NULL);
			}
		}

		/*RLE encoding row widths*/ {
			unsigned i = 0;
			while (i < sheetheight) {
				unsigned short width = rowwidth[i];
				unsigned count = 0u;
				while (i < sheetheight && count <= UINT_MAX  && rowwidth[i] == width) {
					count++;
					i++;
				}
				width = _byteswap_ushort(width); //little to big endian
				count = _byteswap_ulong(count);
				WriteFile(filehandle, &width, sizeof(width), &byteswritten, NULL);
				WriteFile(filehandle, &count, sizeof(count), &byteswritten, NULL);
			}
		}

		/*encoding the input data in the cells*/ {
			std::vector<WorkSheet::CellIndexPair> cipvector = OneWkst.SerializeCells();
			for (auto& i : cipvector) {
				unsigned column = _byteswap_ulong( i.column); //little to big endian
				unsigned row = _byteswap_ulong( i.row );
				
				WriteFile(filehandle, &column,sizeof(column),&byteswritten,NULL); //write column
				WriteFile(filehandle, &row, sizeof(row), &byteswritten, NULL); //write row
				
				for (auto& j : i.cellitem) { //write exact input string to file
					j = _byteswap_ushort(j); //again, little to big endian
					WriteFile(filehandle, &j, sizeof(j), &byteswritten, NULL);
				}
				WriteFile(filehandle, L"\0", sizeof(wchar_t), &byteswritten, NULL); //null terminator
			}
		}

		CloseHandle(filehandle);
		break;
	}
	case WM_OPEN: {
		if (EditWindow != NULL) { //make edit window dissappear if currently editing acell
			DestroyWindow(EditWindow);
		}
		OPENFILENAMEW* ofn = (OPENFILENAMEW*)lparam;
		DWORD bytesread;
		HANDLE filehandle = CreateFileW(ofn->lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		LRESULT toret = TRUE;
		
		//read header
		char string[7];
		//reading XScroll, YScroll, Sheetwidth and sheetheight
		unsigned buffer[4];


		std::vector<unsigned short> loadedcolumnwidths;
		loadedcolumnwidths.reserve(256);
		std::vector<unsigned short> loadedrowwidths;
		loadedrowwidths.reserve(256);
		std::vector<WorkSheet::CellIndexPair> cipvector; //loaded worksheet data

		unsigned scrollwidth = 0u, scrollheight = 0u; //width and height of worksheet in pixels


		(void)ReadFile(filehandle, string, 7, &bytesread, NULL);
		if (bytesread < 7) { //Checking the file header
			goto abort;
		}
		else if (memcmp(string, "eggfile", 7) != 0) {
			goto abort;
		}

		(void)ReadFile(filehandle, buffer, 4 * sizeof(unsigned), &bytesread, NULL);
		if (bytesread < 4 * sizeof(unsigned)) { //Checking the file header
			goto abort;
		}
		buffer[2] = _byteswap_ulong(buffer[2]); //big to small endian
		buffer[3] = _byteswap_ulong(buffer[3]);


		/*Decompress RLE Column widths*/ {
			while (loadedcolumnwidths.size() < buffer[2]) {
				unsigned short width;
				unsigned count;
				
				(void)ReadFile(filehandle, &width, sizeof(width), &bytesread, NULL);
				if (bytesread < sizeof(width)) { goto abort;}
				
				(void)ReadFile(filehandle, &count, sizeof(count), &bytesread, NULL);
				if (bytesread < sizeof(count)) { goto abort; }
				
				width = _byteswap_ushort(width); //big endian to small
				count = _byteswap_ulong(count);
				for (unsigned j = 0u; j < count; j++) {
					loadedcolumnwidths.push_back(width);
					scrollwidth += width;
				}
			}
			if (loadedcolumnwidths.size() > buffer[2]) { goto abort; }
		}

		/*Decompress RLE Row widths*/ {
			while (loadedrowwidths.size() < buffer[3]) {
				unsigned short width;
				unsigned count;
				
				(void)ReadFile(filehandle, &width, sizeof(width), &bytesread, NULL);
				if (bytesread < sizeof(width)) { goto abort; }
				
				(void)ReadFile(filehandle, &count, sizeof(count), &bytesread, NULL);
				if (bytesread < sizeof(count)) { goto abort; }

				width = _byteswap_ushort(width); //big endian to small
				count = _byteswap_ulong(count);
				for (unsigned j = 0u; j < count; j++) {
					loadedrowwidths.push_back(width);
					scrollheight += width;
				}
			}
			if (loadedrowwidths.size() > buffer[3]) { goto abort; }
		}

		/*Loading the data in the cells*/
		while (1) {
			cipvector.push_back(WorkSheet::CellIndexPair());
			WorkSheet::CellIndexPair& temp = cipvector.back();
			unsigned column;
			unsigned row;

			(void)ReadFile(filehandle, &column, sizeof(column), &bytesread, NULL);
			if (bytesread == 0) { break; } //we have reached the end of the file
			else if (bytesread < sizeof(column)) { goto abort; }

			(void)ReadFile(filehandle, &row, sizeof(row), &bytesread, NULL);
			if (bytesread < sizeof(row)) { goto abort; }

			temp.column = _byteswap_ulong(column); //big to small endian
			temp.row = _byteswap_ulong(row);

			std::wstring data;
			while (1) {
				wchar_t character;
				
				(void)ReadFile(filehandle, &character, sizeof(character), &bytesread, NULL);
				if(bytesread < sizeof(character)) { goto abort; }
				
				character = _byteswap_ushort(character);
				if (character == L'\0') break;
				
				data.push_back(character);
			}
			temp.cellitem = std::move(data);
		}

		//The loading process has succeeded, now change current states

		//update row and column widths
		memcpy(rowwidth, loadedrowwidths.data(), loadedrowwidths.size() * sizeof(unsigned short));
		memcpy(columnwidth, loadedcolumnwidths.data(), loadedcolumnwidths.size() * sizeof(unsigned short));
		
		//update scroll
		SCROLLINFO vsi; //vertical scroll info
		SCROLLINFO hsi; //horizontal scroll info
		vsi.cbSize = sizeof(vsi);											       hsi.cbSize = sizeof(hsi);
		vsi.fMask = SIF_POS|SIF_RANGE;							       hsi.fMask = SIF_POS|SIF_RANGE;
		vsi.nMin = 0;                                                                    hsi.nMin = 0;
		vsi.nMax = scrollheight;                                                   hsi.nMax = scrollwidth;
		vsi.nPos = _byteswap_ulong(buffer[1]);                           hsi.nPos = _byteswap_ulong(buffer[0]); //big to little endian
		SetScrollInfo(windowhandle, SB_VERT, &vsi,TRUE);	   SetScrollInfo(windowhandle, SB_HORZ, &hsi,TRUE);
		
		//update spreadsheet
		OneWkst.DeSerializeCells(cipvector);

		InvalidateRect(windowhandle, NULL, TRUE);
		
		goto notabort; //File loaded successfully so no need for the error
	abort:
		MessageBoxA(NULL, "Eggfile is invalid", "Invalid File", NULL);
		toret = FALSE;
	notabort:
		
		CloseHandle(filehandle);
		return toret;
		break;
	}
	case WM_MERGE: {
		rangedrag = false;
		dragmode = none;

		WCHAR* WindowContent = new WCHAR[GetWindowTextLengthW(EditWindow) + (size_t)1/*null terminator*/];
		if (!WindowContent) MessageBoxA(NULL, "Allocation Failure", "Allocation Failure", MB_ICONERROR);

		if (EditWindow != NULL) { //apply and destroy old window if there exists an edit window;
			GetWindowTextW(EditWindow, WindowContent, INT_MAX);
			(void)OneWkst.SetCell(WindowContent, EditX, EditY);
			RECT clientrect; GetClientRect(windowhandle, &clientrect);
			clientrect.top += columnheaderwidth;
			clientrect.left += rowheaderwidth;
			InvalidateRect(windowhandle, &clientrect, TRUE);
			DestroyWindow(EditWindow);
		}
		mergemode = mergebegin;
		break;
	}
	case WM_SPLIT: {
		rangedrag = false;
		dragmode = none;

		WCHAR* WindowContent = new WCHAR[GetWindowTextLengthW(EditWindow) + (size_t)1/*null terminator*/];
		if (!WindowContent) MessageBoxA(NULL, "Allocation Failure", "Allocation Failure", MB_ICONERROR);

		if (EditWindow != NULL) { //apply and destroy old window if there exists an edit window;
			GetWindowTextW(EditWindow, WindowContent, INT_MAX);
			(void)OneWkst.SetCell(WindowContent, EditX, EditY);
			RECT clientrect; GetClientRect(windowhandle, &clientrect);
			clientrect.top += columnheaderwidth;
			clientrect.left += rowheaderwidth;
			InvalidateRect(windowhandle, &clientrect, TRUE);
			DestroyWindow(EditWindow);
		}
		mergemode = mergesplit;
		break;
	}
	}

	return DefWindowProcW(windowhandle, msg, wparam, lparam);
}


LRESULT CALLBACK editproc(HWND windowhandle, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_CHAR ) {
		if (wparam == VK_RETURN) {
			SendMessageW((HWND)GetWindowLongPtrW(windowhandle, GWLP_HWNDPARENT), WM_EDITENTER, NULL, (LPARAM)windowhandle);
			return 0;
		}
		if (updateselect) {
			CallWindowProcW(oldeditproc, windowhandle, EM_SETSEL, -1,NULL); //deselect then type
		}
	}
	else if (msg == WM_LBUTTONDOWN) {
		updateselect = false;
	}
	return CallWindowProcW(oldeditproc, windowhandle, msg, wparam, lparam);
}