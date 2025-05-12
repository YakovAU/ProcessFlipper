#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include "tray_icon.h"
#include "resource.h" // Include resource definitions
#include <shellapi.h> // For Shell_NotifyIcon
#include <wchar.h>    // For wcscpy_s
// #include <strsafe.h> // For StringCchCopyW (alternative)

TrayIcon::TrayIcon(HINSTANCE hInstance, HWND hWnd) : hWnd_(hWnd), hInstance_(hInstance) {
    ZeroMemory(&nid_, sizeof(NOTIFYICONDATAW));
    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = hWnd_;
    nid_.uID = TRAY_ICON_UID;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;


    nid_.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_APPICON));

    // When UNICODE is defined, use nid_.szTip for NOTIFYICONDATAW structure
    wcscpy_s(nid_.szTip, _countof(nid_.szTip), L"ProcessFlipper");
}

TrayIcon::~TrayIcon() {
    Remove();
}

void TrayIcon::Show() {
    Shell_NotifyIconW(NIM_ADD, &nid_);
}

void TrayIcon::Remove() {
    // Check if the notification icon is still valid
    if (nid_.hWnd != NULL && IsWindow(nid_.hWnd)) {
        // Use the original structure directly - copying may have caused issues
        Shell_NotifyIconW(NIM_DELETE, &nid_);
    }

    // Set to NULL to prevent double-removal
    nid_.hWnd = NULL;
}

void TrayIcon::ShowContextMenu(POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, ID_TRAY_MENU_SUSPEND, L"Suspend (Ctrl+Alt+F3)"); // Use L""
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, ID_TRAY_MENU_KILL, L"Kill (Ctrl+Alt+F4)");    // Use L""
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_MENU_QUIT, L"Quit");                             // Use L""
    SetForegroundWindow(hWnd_);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd_, nullptr);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK TrayIcon::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            TrayIcon* pThis = reinterpret_cast<TrayIcon*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (pThis) pThis->ShowContextMenu(pt);
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
