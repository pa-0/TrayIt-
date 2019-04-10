/* This file is a part of trayIt utility.
 Copyright © 2016-2019 Видершпан Евгений Сергеевич

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>

#define MY_MAIN_WND_CLASS "TRAY_IT_CLASS"
#define TRAYIT_PROP_ID "HidenByTrayIt"
#define MAIN_ICON_ID 1578
#define ID_LISTBOX 33

#define IDM_COMMON          WM_USER+1024
#define WM_NOTIFYICONMSG    IDM_COMMON+1
#define IDM_ADD             IDM_COMMON+2
#define IDM_MANAGE          IDM_COMMON+3
#define IDM_EXIT            IDM_COMMON+4

HMODULE hInstance;
HWND hMainWnd;
HWND hMainList;

LRESULT MainWndProc(HWND hWnd, UINT uMsg, LONG wParam, LONG lParam);

WNDCLASSEX MainWc = {
    sizeof(WNDCLASSEX),
    CS_HREDRAW+CS_VREDRAW+CS_BYTEALIGNWINDOW,
	(WNDPROC)MainWndProc,
    0, 0, 0, 0, 0,
    (HBRUSH)0,
    0,
    MY_MAIN_WND_CLASS,
    0
};


HICON GetWindowIcon(HWND hwnd)
{
	HICON icon;
	if (icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0))
        return icon;
	if (icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0))
        return icon;
	if (icon = (HICON)GetClassLong(hwnd, GCL_HICON))
        return icon;
	if (icon = (HICON)GetClassLong(hwnd, GCL_HICONSM))
        return icon;
	return LoadIcon(NULL, IDI_WINLOGO);
}


void IconTray()
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize           = sizeof(nid);
	nid.hWnd             = hMainWnd;
	nid.uID              = MAIN_ICON_ID;
	nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_NOTIFYICONMSG;
	nid.hIcon            = GetWindowIcon(hMainWnd);
	GetWindowText(hMainWnd, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]));
	Shell_NotifyIcon(NIM_ADD, &nid);
}


void UnIconTray()
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd   = hMainWnd;
	nid.uID    = MAIN_ICON_ID;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}


void HideWindow(HWND hwnd)
{
    DWORD style;
    
	if (hwnd == hMainWnd)
	{
		ShowWindow(hwnd, SW_HIDE);
		return;
	}
	style = GetWindowLong(hwnd, GWL_STYLE);
	if (style & WS_CHILD)
		hwnd = GetAncestor(hwnd, GA_ROOT);
	
	if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
		return;
	
	if (style & WS_SYSMENU)
	{
		if (!SetProp(hwnd, TRAYIT_PROP_ID, (HANDLE)1))
			return;
		ShowWindow(hwnd, SW_HIDE);
	}
}


void ShowMenu()
{
	HMENU hmenu;
	POINT pt;

	hmenu = CreatePopupMenu();
	if (!hmenu)
	{
		MessageBox(hMainWnd, "Error create menu!", "TrayIt", MB_OK|MB_ICONERROR);
		return;
	}
	AppendMenu(hmenu, MF_STRING|MF_DISABLED, 0,   "&Hide active window.\tCTRL+ALT+H");
	AppendMenu(hmenu, MF_STRING, IDM_MANAGE,   "&Manage hidden windows");
	AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hmenu, MF_STRING, IDM_EXIT,    "&Exit TrayIt");

	GetCursorPos(&pt);
	SetForegroundWindow(hMainWnd);

	TrackPopupMenu(hmenu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON|TPM_RIGHTALIGN|\
		TPM_BOTTOMALIGN, pt.x, pt.y, 0, hMainWnd, 0);

	PostMessage(hMainWnd, WM_USER, 0, 0);
	DestroyMenu(hmenu);
}


void CenterWindow( HWND hWnd )
{
	RECT rc;
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rc, 0))
	{
		RECT wrc;
		if (GetWindowRect(hWnd, &wrc))
		{
			int width = wrc.right-wrc.left;
			int height = wrc.bottom-wrc.top;
			SetWindowPos(hWnd, 0, 
				((rc.right-rc.left)-width)/2,
				((rc.bottom-rc.top)-height)/2,
                width, height,
				SWP_NOZORDER|SWP_NOREPOSITION);
		}
	}
}

BOOL CALLBACK PropEnumProc(HWND hwnd, LPCSTR lpszString, HANDLE hData)
{
	if (((DWORD)lpszString & 0xffff0000) && !lstrcmp(TRAYIT_PROP_ID, lpszString))
	{
		SendMessage(hMainList, LB_ADDSTRING, 0, (LPARAM)hwnd);
	}
	return TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	EnumProps(hwnd, (PROPENUMPROCA)&PropEnumProc);
	return TRUE;
}

void UpdateList()
{
	SendMessage(hMainList, LB_RESETCONTENT, 0, 0);
	EnumWindows((WNDENUMPROC)&EnumWindowsProc, 0);
}

void ManageList()
{
	CenterWindow(hMainWnd);
	ShowWindow(hMainWnd,SW_SHOW);
	SetForegroundWindow(hMainWnd);
	UpdateList();
	UpdateWindow(hMainWnd);
}

void DrawItem(LPDRAWITEMSTRUCT lpdis)
{
    char buf[256];
	HWND hwin;
    RECT rc;
    
    if (!lpdis)
        return;
    
	if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == ID_LISTBOX)
	{
		hwin = (HWND)lpdis->itemData;
		
		if (lpdis->itemState & ODS_SELECTED)
        {
            FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
        }
		if ( lpdis->itemState & ODS_FOCUS )
        {
            
        }
		SetBkMode(lpdis->hDC, TRANSPARENT);
		
		
		if (GetWindowText(hwin, buf, 256))
		{
			rc = lpdis->rcItem;
			rc.left += 34;
			DrawText(lpdis->hDC, buf, -1, &rc, DT_LEFT|DT_NOPREFIX|\
                DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
		}
		
		if (IsWindow(hwin))
		{
			DrawIcon(lpdis->hDC, lpdis->rcItem.left,
                lpdis->rcItem.top, GetWindowIcon(hwin));
		}
	}
}


LRESULT MainWndProc(HWND hWnd, UINT uMsg, LONG wParam, LONG lParam)
{
	switch (uMsg)
	{
		case WM_NOTIFYICONMSG:
		{
			switch (lParam)
			{
				case WM_RBUTTONUP:
				ShowMenu();
				break;
				
				case WM_LBUTTONDBLCLK:
				ManageList();
				break;
			}
      	} return 0;
      	
		case WM_HOTKEY:
		{
			HideWindow(GetForegroundWindow());
			UpdateList();
      	} return 0;
      	
      	case WM_SIZE:
      	{
			RECT rc;
			if (GetClientRect(hWnd, &rc))
			{
				SetWindowPos(hMainList, 0, 0, 0,
                    rc.right, rc.bottom, SWP_NOZORDER|SWP_NOMOVE);
			}
		} return 0;
		
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == ID_LISTBOX)
			{
				int id = SendMessage(hMainList, LB_GETCURSEL, 0, 0);
				if (id >= 0)
				{
					HWND hwin = (HWND)SendMessage(hMainList, LB_GETITEMDATA, id, 0);
					if (IsWindow(hwin))
					{
						if (!RemoveProp(hwin, TRAYIT_PROP_ID))
						{
							MessageBox(hWnd, "Error Deleting prop!", 0, MB_ICONERROR);
						}
						ShowWindow(hwin, SW_SHOW);
						SetForegroundWindow(hwin);
						UpdateList();
					}
				}
				return 0;
			}

			switch (LOWORD(wParam))
			{
				case IDM_MANAGE:
					ManageList();
				return 0;
				case IDM_EXIT:
					SendMessage(hWnd, WM_DESTROY, 0, 0);
				return 0;
			}
		} break;
		
		case WM_CLOSE:
			ShowWindow(hMainWnd, SW_HIDE);
		return 0;

		case WM_MEASUREITEM:
		{
			UINT idCtl = (UINT) wParam;
			if (idCtl == ID_LISTBOX)
			{
				LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
				if (!lpmis)
                    return 0;
				
				if (lpmis->CtlType == ODT_LISTBOX)
				{
					lpmis->itemHeight = 32;
					return 1;
				}
			}
		} break;
		
		case WM_DRAWITEM:
		{
			DrawItem((LPDRAWITEMSTRUCT) lParam);
		} break;

		case WM_SHOWWINDOW:
		{
			UpdateList();
		} return 0;
		
		case WM_DESTROY:
		{
			UnregisterHotKey(hWnd, IDM_ADD);
			UnIconTray();
			PostQuitMessage(0);
		} break;
		
		case WM_CREATE:
		{
			hMainWnd = hWnd;
			hMainList = CreateWindowEx(
                0,
                "LISTBOX",
                0,
                WS_CHILD|WS_VISIBLE|WS_HSCROLL|LBS_OWNERDRAWFIXED|\
                LBS_NOINTEGRALHEIGHT|LBS_NOTIFY,
                0, 0, 0, 0,
                hWnd, (HMENU)ID_LISTBOX,
                0, 0);
			
			RegisterHotKey(hWnd, IDM_ADD, MOD_CONTROL|MOD_ALT, 'H');
			SendMessage(hWnd, WM_SETICON, ICON_BIG,
                (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(1)));
			
			IconTray();
		} break;
	}
	return(DefWindowProc(hWnd,uMsg,wParam,lParam));
}



int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow)
{
	MSG msg;
    hMainWnd = FindWindow(MainWc.lpszClassName, 0);
	if (hMainWnd)
    {
        SendMessage(hMainWnd, WM_NOTIFYICONMSG, 0, WM_LBUTTONDBLCLK);
		return -1;
    }
	
 	hInstance = GetModuleHandle(0);
 	MainWc.hCursor = LoadCursor(0, IDC_ARROW);
 	MainWc.hInstance = hInstance;
	
    if (RegisterClassEx(&MainWc))
	{
		InitCommonControls();
		
		hMainWnd = CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE,
				MainWc.lpszClassName,
                "Tray It v 0.1",
                WS_CLIPCHILDREN | WS_TILEDWINDOW,
				0, 0, 400, 300,
                0, 0,
                hInstance,
                0);
					
		CenterWindow(hMainWnd);
		
		/*ShowWindow(hMainWnd,SW_SHOW);
		UpdateWindow(hMainWnd);*/

		if (hMainWnd)
		{
			while (GetMessage(&msg,0,0,0))
			{
				if (!IsDialogMessage(hMainWnd, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		UnregisterClass(MainWc.lpszClassName, hInstance);
	}
	return (msg.lParam);
}



