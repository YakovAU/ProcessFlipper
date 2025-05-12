#include <iostream>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <unordered_set> // Added for tracking multiple suspended processes
#include <cstdlib> // For _set_abort_behavior
#include "hotkey_manager.h"
#include "process_manager.h"
#include "tray_icon.h"

std::atomic<bool> running(true);

// At the top of your main.cpp file, before WinMain
#include <cstdio> // For freopen_s
#include <fstream>
#include <filesystem>
#include <eh.h> // For _set_se_translator
#include <dbghelp.h> // For stack trace functionality

// Global file stream for logging
std::ofstream logFile;

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#pragma comment(lib, "dbghelp.lib")

// Forward declaration
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int);

// Custom structured exception handler
LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* ep) {
    // Log the exception if logFile is open
    if (logFile.is_open()) {
        logFile << "Structured Exception: 0x" << std::hex << ep->ExceptionRecord->ExceptionCode << " at address 0x"
                << ep->ExceptionRecord->ExceptionAddress << std::dec << std::endl;

        // Log some basic register information
        logFile << "Register state:" << std::endl;
        logFile << "  EAX: 0x" << std::hex << ep->ContextRecord->Rax << std::endl;
        logFile << "  EBX: 0x" << std::hex << ep->ContextRecord->Rbx << std::endl;
        logFile << "  ECX: 0x" << std::hex << ep->ContextRecord->Rcx << std::endl;
        logFile << "  EDX: 0x" << std::hex << ep->ContextRecord->Rdx << std::endl;
        logFile << "  ESI: 0x" << std::hex << ep->ContextRecord->Rsi << std::endl;
        logFile << "  EDI: 0x" << std::hex << ep->ContextRecord->Rdi << std::endl;
        logFile << "  EBP: 0x" << std::hex << ep->ContextRecord->Rbp << std::endl;
        logFile << "  ESP: 0x" << std::hex << ep->ContextRecord->Rsp << std::endl;
        logFile << "  EIP: 0x" << std::hex << ep->ContextRecord->Rip << std::dec << std::endl;

        logFile.flush();
    }

    // Terminate the process gracefully
    return EXCEPTION_EXECUTE_HANDLER;
}


// Custom structured exception translator
void SETranslator(unsigned int code, EXCEPTION_POINTERS* ep) {
    // Log the exception if logFile is open
    if (logFile.is_open()) {
        logFile << "Structured Exception: 0x" << std::hex << code << " at address 0x"
                << ep->ExceptionRecord->ExceptionAddress << std::dec << std::endl;
                
        // Log some basic register information
        logFile << "Register state:" << std::endl;
        logFile << "  EAX: 0x" << std::hex << ep->ContextRecord->Rax << std::endl;
        logFile << "  EBX: 0x" << std::hex << ep->ContextRecord->Rbx << std::endl;
        logFile << "  ECX: 0x" << std::hex << ep->ContextRecord->Rcx << std::endl;
        logFile << "  EDX: 0x" << std::hex << ep->ContextRecord->Rdx << std::endl;
        logFile << "  ESI: 0x" << std::hex << ep->ContextRecord->Rsi << std::endl;
        logFile << "  EDI: 0x" << std::hex << ep->ContextRecord->Rdi << std::endl;
        logFile << "  EBP: 0x" << std::hex << ep->ContextRecord->Rbp << std::endl;
        logFile << "  ESP: 0x" << std::hex << ep->ContextRecord->Rsp << std::endl;
        logFile << "  EIP: 0x" << std::hex << ep->ContextRecord->Rip << std::dec << std::endl;
        
        logFile.flush();
    }
    
    // Terminate the process gracefully
    ExitProcess(code);
}

// Main entry point
int main(int argc, char* argv[]) {
    // Instead of _set_abort_behavior, we just use SetErrorMode
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

    // Instead of _set_se_translator, use SetUnhandledExceptionFilter
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

    return WinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineA(), SW_SHOWDEFAULT);
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Prevent system dialogs from appearing
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    
    try {
        // Set up logging to file instead of console
        // Create the log file in the same directory as the executable
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::path logPath = std::filesystem::path(exePath).parent_path() / "process_flipper.log";
        
        // Open log file and redirect std::cout/std::cerr to it
        logFile.open(logPath.wstring().c_str(), std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            // Redirect stdout and stderr to our log file
            // First, save original buffer pointers
            std::streambuf* coutBuf = std::cout.rdbuf();
            std::streambuf* cerrBuf = std::cerr.rdbuf();
            
            // Redirect to logFile
            std::cout.rdbuf(logFile.rdbuf());
            std::cerr.rdbuf(logFile.rdbuf());
        }
        
        std::cout << "=======================================================" << std::endl;
        std::cout << "ProcessFlipper started at " << __DATE__ << " " << __TIME__ << std::endl;
        std::cout << "=======================================================" << std::endl;
    }
    catch (const std::exception& e) {
        // Can't log since logging might not be set up yet
        // But we don't want to crash at startup
        // Just continue
    }


    WNDCLASSA wc = {0};
    wc.lpfnWndProc = TrayIcon::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ProcessFlipperTrayClass";
    RegisterClassA(&wc);
    HWND hWnd = CreateWindowA(wc.lpszClassName, "ProcessFlipper", 0, 0,0,0,0, HWND_MESSAGE, NULL, hInstance, NULL);
    SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));

    TrayIcon trayIcon(hInstance, hWnd);
    SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&trayIcon));
    trayIcon.Show();

    ProcessManager processManager;
    HotkeyManager hotkeyManager;
    // UserInterface ui; // Removed as app_main_loop is removed

    // Register hotkeys
    int suspendHotkeyId = hotkeyManager.registerHotkey(MOD_ALT | MOD_CONTROL, VK_F5);
    int terminateHotkeyId = hotkeyManager.registerHotkey(MOD_ALT | MOD_CONTROL, VK_F4);

    // Set callbacks for hotkeys
    // static DWORD lastSuspendedPid = 0; // Replaced by a set for better multi-process toggle
    static std::unordered_set<DWORD> suspendedByAppPids; // Tracks PIDs suspended by this app

    if (suspendHotkeyId != -1) {
        hotkeyManager.setCallback(suspendHotkeyId, [&processManager]() { // Removed suspendedByAppPids from capture
            std::cout << "Suspend/Resume Hotkey (Alt+Ctrl+F5) callback triggered." << std::endl;
            ProcessManager::ProcessInfo focusedProcess = processManager.getFocusedProcessInfo();
            if (focusedProcess.pid != 0) {
                std::wcout << L"Focused process for Suspend/Resume: " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")" << std::endl;
                if (suspendedByAppPids.count(focusedProcess.pid)) {
                    std::wcout << L"Attempting to RESUME process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")" << std::endl;
                    if (processManager.resumeProcess(focusedProcess.pid)) {
                        std::wcout << L"Callback: Successfully RESUMED process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                        suspendedByAppPids.erase(focusedProcess.pid);
                    } else {
                        std::wcout << L"Callback: Failed to RESUME process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                    }
                } else {
                    std::wcout << L"Attempting to SUSPEND process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")" << std::endl;
                    if (!processManager.isCriticalProcess(focusedProcess.name)) {
                        if (processManager.suspendProcess(focusedProcess.pid)) {
                            std::wcout << L"Callback: Successfully SUSPENDED process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                            suspendedByAppPids.insert(focusedProcess.pid);
                        } else {
                            std::wcout << L"Callback: Failed to SUSPEND process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                        }
                    } else {
                        std::wcout << L"Callback: Cannot suspend critical process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                    }
                }
            } else {
                std::cout << "Suspend/Resume Hotkey: No process in foreground." << std::endl;
            }
        });
    } else {
        std::cerr << "Main: Failed to register suspend hotkey (Alt+Ctrl+F5)." << std::endl;
    }

    if (terminateHotkeyId != -1) {
        hotkeyManager.setCallback(terminateHotkeyId, [&processManager]() {
            std::cout << "Terminate Hotkey (Alt+Ctrl+F4) callback triggered." << std::endl;
            ProcessManager::ProcessInfo focusedProcess = processManager.getFocusedProcessInfo();
            if (focusedProcess.pid != 0) {
                std::wcout << L"Focused process for Terminate: " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")" << std::endl;
                if (!processManager.isCriticalProcess(focusedProcess.name)) {
                    std::wcout << L"Attempting to TERMINATE process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")" << std::endl;
                    if (processManager.terminateProcess(focusedProcess.pid)) {
                        std::wcout << L"Callback: Successfully TERMINATED process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                    } else {
                        std::wcout << L"Callback: Failed to TERMINATE process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                    }
                } else {
                    std::wcout << L"Callback: Cannot terminate critical process " << focusedProcess.name << L" (PID: " << focusedProcess.pid << L")\n";
                }
            } else {
                std::cout << "Terminate Hotkey: No process in foreground." << std::endl;
            }
        });
    } else {
        std::cerr << "Main: Failed to register terminate hotkey (Alt+Ctrl+F4)." << std::endl;
    }

    // Message loop - keeping it simple to avoid memory corruption
    {
        MSG msg;
        // Main message loop
        while (running && GetMessageA(&msg, nullptr, 0, 0)) {
            // Process hotkeys first
            bool handled = hotkeyManager.processMessage(msg);
            
            // If not a hotkey, process normally
            if (!handled) {
                if (msg.message == WM_COMMAND) {
                    if (LOWORD(msg.wParam) == ID_TRAY_MENU_QUIT) {
                        running = false;
                        PostQuitMessage(0);
                        break;
                    }
                }
                
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            
            // Don't zero out the message - this was likely causing corruption
        }
    } // Explicit scope to ensure msg is destroyed here
    
    // Empty the message queue before doing cleanup
    {
        MSG msg;
        // Remove any remaining messages without processing them
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            // Just remove messages, don't process them
        }
    }
    
    // Handle cleanup within a try-catch block for all exceptions
    try {
        // Log application exit
        std::cout << "=======================================================" << std::endl;
        std::cout << "ProcessFlipper shutting down" << std::endl;
        std::cout << "=======================================================" << std::endl;
    
        // Properly unregister hotkeys first (explicit cleanup, even though the destructor will handle this)
        if (suspendHotkeyId != -1) {
            UnregisterHotKey(NULL, suspendHotkeyId);
            std::cout << "Unregistered suspend hotkey ID: " << suspendHotkeyId << std::endl;
        }
        if (terminateHotkeyId != -1) {
            UnregisterHotKey(NULL, terminateHotkeyId);
            std::cout << "Unregistered terminate hotkey ID: " << terminateHotkeyId << std::endl;
        }
    
        // Set running to false to ensure any remaining threads know to exit
        running = false;
    
        // Remove tray icon - needs to happen before the window is destroyed
        trayIcon.Remove();
        std::cout << "Tray icon removed" << std::endl;
    
        // Destroy the window if it exists
        if (hWnd && IsWindow(hWnd)) {
            DestroyWindow(hWnd);
            std::cout << "Window destroyed" << std::endl;
        }
    
        // Perform another message queue emptying to handle any destruction messages
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                // Just removing them
            }
        }

        // Unregister window class if needed
        UnregisterClassA("ProcessFlipperTrayClass", hInstance);
        std::cout << "Window class unregistered" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception during cleanup." << std::endl;
    }
    
    // Finally close the log file - do this outside all exception handlers to ensure it happens
    std::cout << "Closing log file" << std::endl;
    if (logFile.is_open()) {
        logFile.flush();
        logFile.close();
    }

    return 0;
}