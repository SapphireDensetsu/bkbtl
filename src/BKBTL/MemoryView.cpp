/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// MemoryView.cpp

#include "stdafx.h"
#include "BKBTL.h"
#include "Views.h"
#include "ToolWindow.h"
#include "Dialogs.h"
#include "Emulator.h"
#include "emubase\Emubase.h"


//////////////////////////////////////////////////////////////////////


HWND g_hwndMemory = (HWND) INVALID_HANDLE_VALUE;  // Memory view window handler
WNDPROC m_wndprocMemoryToolWindow = NULL;  // Old window proc address of the ToolWindow

int m_cyLineMemory = 0;  // Line height in pixels
int m_nPageSize = 0;  // Page size in lines

HWND m_hwndMemoryViewer = (HWND) INVALID_HANDLE_VALUE;

void MemoryView_OnDraw(HDC hdc);
BOOL MemoryView_OnKeyDown(WPARAM vkey, LPARAM lParam);
BOOL MemoryView_OnMouseWheel(WPARAM wParam, LPARAM lParam);
BOOL MemoryView_OnVScroll(WPARAM wParam, LPARAM lParam);
void MemoryView_ScrollTo(WORD wAddress);
void MemoryView_Scroll(int nDeltaLines);
void MemoryView_UpdateScrollPos();
void MemoryView_UpdateWindowText();
LPCTSTR MemoryView_GetMemoryModeName();

//enum MemoryViewMode {
//    MEMMODE_RAM0 = 0,  // RAM plane 0
//    MEMMODE_RAM1 = 1,  // RAM plane 1
//    MEMMODE_RAM2 = 2,  // RAM plane 2
//    MEMMODE_ROM  = 3,  // ROM
//    MEMMODE_CPU  = 4,  // CPU memory
//    MEMMODE_LAST = 5,  // Last mode number
//};

//int     m_Mode = MEMMODE_ROM;  // See MemoryViewMode enum
WORD    m_wBaseAddress = 0;


//////////////////////////////////////////////////////////////////////


void MemoryView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= MemoryViewViewerWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_MEMORYVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void CreateMemoryView(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndMemory = CreateWindow(
            CLASSNAME_TOOLWINDOW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
    MemoryView_UpdateWindowText();

    // ToolWindow subclassing
    m_wndprocMemoryToolWindow = (WNDPROC) LongToPtr( SetWindowLongPtr(
            g_hwndMemory, GWLP_WNDPROC, PtrToLong(MemoryViewWndProc)) );

    RECT rcClient;  GetClientRect(g_hwndMemory, &rcClient);

    m_hwndMemoryViewer = CreateWindowEx(
            WS_EX_STATICEDGE,
            CLASSNAME_MEMORYVIEW, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP,
            0, 0, rcClient.right, rcClient.bottom,
            g_hwndMemory, NULL, g_hInst, NULL);

    MemoryView_ScrollTo(0);
}

// Adjust position of client windows
void MemoryView_AdjustWindowLayout()
{
    RECT rc;  GetClientRect(g_hwndMemory, &rc);

    if (m_hwndMemoryViewer != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndMemoryViewer, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

LRESULT CALLBACK MemoryViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LRESULT lResult;
    switch (message)
    {
    case WM_DESTROY:
        g_hwndMemory = (HWND) INVALID_HANDLE_VALUE;  // We are closed! Bye-bye!..
        return CallWindowProc(m_wndprocMemoryToolWindow, hWnd, message, wParam, lParam);
    case WM_SIZE:
        lResult = CallWindowProc(m_wndprocMemoryToolWindow, hWnd, message, wParam, lParam);
        MemoryView_AdjustWindowLayout();
        return lResult;
    default:
        return CallWindowProc(m_wndprocMemoryToolWindow, hWnd, message, wParam, lParam);
    }
    //return (LRESULT)FALSE;
}

LRESULT CALLBACK MemoryViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            MemoryView_OnDraw(hdc);  // Draw memory dump

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
    case WM_KEYDOWN:
        return (LRESULT) MemoryView_OnKeyDown(wParam, lParam);
    case WM_MOUSEWHEEL:
        return (LRESULT) MemoryView_OnMouseWheel(wParam, lParam);
    case WM_VSCROLL:
        return (LRESULT) MemoryView_OnVScroll(wParam, lParam);
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        ::InvalidateRect(hWnd, NULL, TRUE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}


void MemoryView_OnDraw(HDC hdc)
{
    ASSERT(g_pBoard != NULL);

    // Create and select font
    HFONT hFont = CreateMonospacedFont();
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorText = GetSysColor(COLOR_WINDOWTEXT);
    COLORREF colorOld = SetTextColor(hdc, colorText);
    COLORREF colorBkOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    m_cyLineMemory = cyLine;

    const TCHAR* ADDRESS_LINE = _T("address  0      2      4      6      10     12     14     16");
    TextOut(hdc, cxChar * 4, 0, ADDRESS_LINE, (int) _tcslen(ADDRESS_LINE));

    RECT rcClip;
    GetClipBox(hdc, &rcClip);
    RECT rcClient;
    GetClientRect(m_hwndMemoryViewer, &rcClient);
    m_nPageSize = rcClient.bottom / cyLine - 1;

    WORD address = m_wBaseAddress;
    int y = 1 * cyLine;
    for (;;)    // Draw lines
    {
        DrawOctalValue(hdc, 5 * cxChar, y, address);

        int x = 13 * cxChar;
        TCHAR wchars[16];
        for (int j = 0; j < 8; j++)    // Draw words as octal value
        {
            // Get word from memory
            WORD word = 0;
            int addrtype;
            BOOL okHalt = FALSE;
            WORD wChanged = 0;
            //switch (m_Mode) {
            //    case MEMMODE_RAM0:
            //    case MEMMODE_RAM1:
            //    case MEMMODE_RAM2:
            //        word = g_pBoard->GetRAMWord(m_Mode, address);
            //        wChanged = Emulator_GetChangeRamStatus(m_Mode, address);
            //        break;
            //    case MEMMODE_ROM:  // ROM - only 32 Kbytes
            //        if (address < 0100000)
            //            okValid = FALSE;
            //        else
            //            word = g_pBoard->GetROMWord(address - 0100000);
            //        break;
            //    case MEMMODE_CPU:
            okHalt = g_pBoard->GetCPU()->IsHaltMode();
            word = g_pBoard->GetWordView(address, okHalt, FALSE, &addrtype);
            wChanged = Emulator_GetChangeRamStatus(address);
            //        break;
            //}

            if ((addrtype & (ADDRTYPE_IO | ADDRTYPE_DENY)) == 0)
            {
                ::SetTextColor(hdc, (wChanged != 0) ? RGB(255, 0, 0) : colorText);
                DrawOctalValue(hdc, x, y, word);
            }

            // Prepare characters to draw at right
            BYTE ch1 = LOBYTE(word);
            TCHAR wch1 = Translate_BK_Unicode(ch1);
            if (ch1 < 32) wch1 = _T('�');
            wchars[j * 2] = wch1;
            BYTE ch2 = HIBYTE(word);
            TCHAR wch2 = Translate_BK_Unicode(ch2);
            if (ch2 < 32) wch2 = _T('�');
            wchars[j * 2 + 1] = wch2;

            address += 2;
            x += 7 * cxChar;
        }
        ::SetTextColor(hdc, colorText);

        // Draw characters at right
        int xch = x + cxChar;
        TextOut(hdc, xch, y, wchars, 16);

        y += cyLine;
        if (y > rcClip.bottom) break;
    }

    SetTextColor(hdc, colorOld);
    SetBkColor(hdc, colorBkOld);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    if (::GetFocus() == m_hwndMemoryViewer)
    {
        RECT rcFocus = rcClient;
        rcFocus.left += cxChar * 4;
        rcFocus.top += cyLine;
        rcFocus.right = cxChar * (63 + 24);
        DrawFocusRect(hdc, &rcFocus);
    }
}

LPCTSTR MemoryView_GetMemoryModeName()
{
    //  switch (m_Mode) {
    //      case MEMMODE_RAM0:  return _T("RAM0");
    //      case MEMMODE_RAM1:  return _T("RAM1");
    //      case MEMMODE_RAM2:  return _T("RAM2");
    //      case MEMMODE_ROM:   return _T("ROM");
    //      case MEMMODE_CPU:   return _T("CPU");
    //default:
    //	return _T("UKWN");  // Unknown mode
    //  }
    return _T("");  //STUB
}

void MemoryView_UpdateWindowText()
{
    //TCHAR buffer[64];
    //swprintf_s(buffer, 64, _T("Memory - %s"), MemoryView_GetMemoryModeName());
    ::SetWindowText(g_hwndMemory, _T("Memory"));
}

BOOL MemoryView_OnKeyDown(WPARAM vkey, LPARAM lParam)
{
    switch (vkey)
    {
    case VK_ESCAPE:
        ConsoleView_Activate();
        break;
    case VK_HOME:
        MemoryView_ScrollTo(0);
        break;
    case VK_LEFT:
        MemoryView_Scroll(-2);
        break;
    case VK_RIGHT:
        MemoryView_Scroll(2);
        break;
    case VK_UP:
        MemoryView_Scroll(-16);
        break;
    case VK_DOWN:
        MemoryView_Scroll(16);
        break;
        //  case VK_SPACE:
        //      if (m_Mode == MEMMODE_LAST)
        //          m_Mode = 0;
        //      else
        //          m_Mode++;
        //      InvalidateRect(m_hwndMemoryViewer, NULL, TRUE);
        //MemoryView_UpdateWindowText();
        //      break;
    case VK_PRIOR:
        MemoryView_Scroll(-m_nPageSize * 16);
        break;
    case VK_NEXT:
        MemoryView_Scroll(m_nPageSize * 16);
        break;
    case 0x47:  // G - Go To Address
        {
            WORD value = m_wBaseAddress;
            if (InputBoxOctal(m_hwndMemoryViewer, _T("Go To Address"), _T("Address (octal):"), &value))
                MemoryView_ScrollTo(value);
            SetFocus(m_hwndMemoryViewer);
            break;
        }
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL MemoryView_OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

    int nDelta = zDelta / 120;
    if (nDelta > 5) nDelta = 5;
    if (nDelta < -5) nDelta = -5;

    MemoryView_Scroll(-nDelta * 2 * 16);

    return FALSE;
}

BOOL MemoryView_OnVScroll(WPARAM wParam, LPARAM lParam)
{
    WORD scrollpos = HIWORD(wParam);
    WORD scrollcmd = LOWORD(wParam);
    switch (scrollcmd)
    {
    case SB_LINEDOWN:
        MemoryView_Scroll(16);
        break;
    case SB_LINEUP:
        MemoryView_Scroll(-16);
        break;
    case SB_PAGEDOWN:
        MemoryView_Scroll(m_nPageSize * 16);
        break;
    case SB_PAGEUP:
        MemoryView_Scroll(-m_nPageSize * 16);
        break;
    case SB_THUMBPOSITION:
        MemoryView_ScrollTo(scrollpos * 16);
        break;
    }

    return FALSE;
}

// Scroll window to given base address
void MemoryView_ScrollTo(WORD wAddress)
{
    m_wBaseAddress = wAddress & ((WORD)~1);
    InvalidateRect(m_hwndMemoryViewer, NULL, TRUE);

    MemoryView_UpdateScrollPos();
}
// Scroll window on given lines forward or backward
void MemoryView_Scroll(int nDelta)
{
    if (nDelta == 0) return;

    m_wBaseAddress += nDelta;
    m_wBaseAddress = m_wBaseAddress & ((WORD)~1);
    InvalidateRect(m_hwndMemoryViewer, NULL, TRUE);

    MemoryView_UpdateScrollPos();
}
void MemoryView_UpdateScrollPos()
{
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    si.nPage = m_nPageSize;
    si.nPos = m_wBaseAddress / 16;
    si.nMin = 0;
    si.nMax = 0x10000 / 16 - 1;
    SetScrollInfo(m_hwndMemoryViewer, SB_VERT, &si, TRUE);
}

//////////////////////////////////////////////////////////////////////
