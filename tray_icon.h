#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

// Tray icon and menu IDs
#define TRAY_ICON_UID 1001
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_MENU_QUIT 2001
#define ID_TRAY_MENU_SUSPEND 2002
#define ID_TRAY_MENU_KILL 2003

class TrayIcon {
public:
    TrayIcon(HINSTANCE hInstance, HWND hWnd);
    ~TrayIcon();
    void Show();
    void Remove();
    void ShowContextMenu(POINT pt);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    NOTIFYICONDATAW nid_{};
    HWND hWnd_{};
    HINSTANCE hInstance_{};
};
