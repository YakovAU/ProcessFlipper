#include "process_manager.h"
#include <iostream> // For debugging, remove later if not needed
#include <windows.h>
#include <processthreadsapi.h> // For OpenProcessToken, GetCurrentProcess
#include <securitybaseapi.h> // For LookupPrivilegeValue, AdjustTokenPrivileges
#include <handleapi.h> // For CloseHandle
#include <tlhelp32.h> // For CreateToolhelp32Snapshot, Process32First, etc.
#include <winuser.h> // For EnumWindows

// Helper function to enable SeDebugPrivilege
// Moved before its first use by suspendProcess, resumeProcess, terminateProcess
bool EnableDebugPrivilege() {
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "Failed to open process token: " << GetLastError() << std::endl;
        return false;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        std::cerr << "Failed to lookup privilege value: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        std::cerr << "Failed to adjust token privileges: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        std::cerr << "The token does not have the specified privilege." << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

// Constructor to initialize critical processes
ProcessManager::ProcessManager() {
    criticalProcessNames = {
        L"explorer.exe",
        L"wininit.exe",
        L"winlogon.exe",
        L"csrss.exe",
        L"smss.exe",
        L"lsass.exe",
        L"services.exe",
        L"svchost.exe",
        L"dwm.exe",
        L"taskmgr.exe",
        // Add other critical process names as needed
    };
}

std::wstring ProcessManager::getProcessNameByPid(DWORD pid) {
    if (pid == 0) {
        // std::wcerr << L"getProcessNameByPid called with PID 0." << std::endl; // Optional: log if PID is 0
        return L"";
    }
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == nullptr) {
        std::wcerr << L"getProcessNameByPid: OpenProcess failed for PID " << pid << L". Error: " << GetLastError() << std::endl;
        return L"";
    }

    WCHAR processName[MAX_PATH];
    if (GetModuleFileNameExW(hProcess, nullptr, processName, MAX_PATH) == 0) {
        std::wcerr << L"getProcessNameByPid: GetModuleFileNameExW failed for PID " << pid << L". Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return L"";
    }

    CloseHandle(hProcess);
    std::wstring fullPath(processName);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return fullPath.substr(lastSlash + 1);
    }
    return fullPath; // Should not happen if path is valid
}

// Structure to pass data to EnumWindows callback
struct EnumWindowsCallback_Data {
    HWND targetHwnd;
    DWORD processId;
};

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc_FindProcess(HWND hwnd, LPARAM lParam) {
    EnumWindowsCallback_Data* data = reinterpret_cast<EnumWindowsCallback_Data*>(lParam);
    if (hwnd == data->targetHwnd) {
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId); // Try again, or just use the PID from the process enumeration
        // For this alternative, we'd ideally get the PID from the process enumeration that led to this window.
        // However, EnumWindows itself doesn't directly give the PID of the window's owner.
        // A more robust way is to iterate processes, then iterate their windows.
        // Let's refine this. We'll iterate processes first.
        return FALSE; // Stop enumeration
    }
    return TRUE; // Continue enumeration
}


// Helper structure for EnumThreadWndProc
struct EnumThreadWndData {
    HWND targetHwnd;
    bool found;
};

// Callback for EnumThreadWindows
BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam) {
    EnumThreadWndData* pData = (EnumThreadWndData*)lParam;
    // std::cout << "EnumThreadWndProc: Checking HWND " << hwnd << " against Target HWND " << pData->targetHwnd << std::endl; // Can be very verbose
    if (hwnd == pData->targetHwnd) {
        std::cout << "EnumThreadWndProc: Target HWND " << pData->targetHwnd << " found!" << std::endl;
        pData->found = true;
        return FALSE; // Stop enumerating
    }
    return TRUE; // Continue
}


DWORD GetProcessIdFromWindowHandle_Alternative(HWND targetHwnd) {
    std::cout << "GetProcessIdFromWindowHandle_Alternative: Called for HWND " << targetHwnd << std::endl;
    if (!targetHwnd) {
        std::cout << "GetProcessIdFromWindowHandle_Alternative: targetHwnd is NULL, returning 0." << std::endl;
        return 0;
    }

    DWORD foundPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Only snap processes here
    if (hSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "GetProcessIdFromWindowHandle_Alternative: CreateToolhelp32Snapshot (Processes) failed. Error: " << GetLastError() << std::endl;
        return 0;
    }
    std::cout << "GetProcessIdFromWindowHandle_Alternative: Process Snapshot Handle: " << hSnap << std::endl;

    PROCESSENTRY32W pe; // Use W version for wide char process names
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnap, &pe)) {
        std::cout << "GetProcessIdFromWindowHandle_Alternative: Iterating processes..." << std::endl;
        do {
            std::wcout << L"  Process: " << pe.szExeFile << L" (PID: " << pe.th32ProcessID << L")" << std::endl;
            // For each process, enumerate its threads
            HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); // Snapshot all threads in the system
            if (hThreadSnap != INVALID_HANDLE_VALUE) {
                // std::cout << "    Thread Snapshot Handle: " << hThreadSnap << " for PID: " << pe.th32ProcessID << std::endl;
                THREADENTRY32 te;
                te.dwSize = sizeof(THREADENTRY32);
                if (Thread32First(hThreadSnap, &te)) {
                    // std::cout << "    Iterating threads for PID: " << pe.th32ProcessID << std::endl;
                    do {
                        if (te.th32OwnerProcessID == pe.th32ProcessID) { // Check if thread belongs to current process
                            // std::cout << "      Thread ID: " << te.th32ThreadID << " (Owner PID: " << te.th32OwnerProcessID << ")" << std::endl;
                            EnumThreadWndData data = { targetHwnd, false };
                            // std::cout << "      Calling EnumThreadWindows for TID: " << te.th32ThreadID << " with Target HWND: " << targetHwnd << std::endl;
                            EnumThreadWindows(te.th32ThreadID, EnumThreadWndProc, (LPARAM)&data);
                            if (data.found) {
                                std::cout << "      FOUND! PID: " << pe.th32ProcessID << " for HWND " << targetHwnd << " via TID " << te.th32ThreadID << std::endl;
                                foundPid = pe.th32ProcessID;
                                CloseHandle(hThreadSnap);
                                CloseHandle(hSnap);
                                return foundPid;
                            }
                        }
                    } while (Thread32Next(hThreadSnap, &te));
                } else {
                    // DWORD err = GetLastError();
                    // if (err != ERROR_NO_MORE_FILES) { // ERROR_NO_MORE_FILES is expected if a process has no threads or iteration ends
                    //    std::cerr << "    Thread32First failed for PID " << pe.th32ProcessID << ". Error: " << err << std::endl;
                    // }
                }
                CloseHandle(hThreadSnap);
            } else {
                 std::cerr << "  GetProcessIdFromWindowHandle_Alternative: CreateToolhelp32Snapshot for threads failed for PID " << pe.th32ProcessID << ". Error: " << GetLastError() << std::endl;
            }
        } while (Process32NextW(hSnap, &pe) && foundPid == 0);
    } else {
        std::cerr << "GetProcessIdFromWindowHandle_Alternative: Process32FirstW failed. Error: " << GetLastError() << std::endl;
    }

    CloseHandle(hSnap);
    if (foundPid != 0) {
        std::cout << "GetProcessIdFromWindowHandle_Alternative: Successfully found PID " << foundPid << " for HWND " << targetHwnd << std::endl;
    } else {
        std::cout << "GetProcessIdFromWindowHandle_Alternative: Could not find PID for HWND " << targetHwnd << " after checking all processes/threads." << std::endl;
    }
    return foundPid;
}


ProcessManager::ProcessInfo ProcessManager::getFocusedProcessInfo() {
    ProcessInfo focusedProcess = {0, L""};
    HWND foregroundWindow = GetForegroundWindow();

    if (foregroundWindow) {
        // Log the raw foreground window handle
        std::cout << "getFocusedProcessInfo: GetForegroundWindow() returned HWND: " << foregroundWindow << std::endl;

        // Check if the window is visible and a top-level window
        // IsWindowVisible is a good first check.
        // GetParent == NULL is a common check for top-level windows.
        // GetWindow(hwnd, GW_OWNER) == NULL is also a good check, as some top-level windows might have an owner (e.g. dialogs)
        // but we are interested in main application windows.
        bool isVisible = IsWindowVisible(foregroundWindow);
        HWND owner = GetWindow(foregroundWindow, GW_OWNER);
        WCHAR className[256] = {0}; // Initialize to prevent garbage if GetClassNameW fails or doesn't fill
        GetClassNameW(foregroundWindow, className, sizeof(className)/sizeof(WCHAR)); // -1 for null terminator
        
        WCHAR windowText[256] = {0}; // Initialize
        GetWindowTextW(foregroundWindow, windowText, sizeof(windowText)/sizeof(WCHAR)); // -1 for null terminator

        HWND parentHwnd = GetParent(foregroundWindow);
        HWND rootOwnerHwnd = GetAncestor(foregroundWindow, GA_ROOTOWNER);
        LONG_PTR style = GetWindowLongPtr(foregroundWindow, GWL_STYLE); // Use LONG_PTR for compatibility
        LONG_PTR exStyle = GetWindowLongPtr(foregroundWindow, GWL_EXSTYLE); // Use LONG_PTR

        std::wcout << L"getFocusedProcessInfo Details for HWND " << foregroundWindow << L":" << std::endl;
        std::wcout << L"  Text: \"" << windowText << L"\"" << std::endl;
        std::wcout << L"  ClassName: " << className << std::endl;
        std::wcout << L"  IsVisible: " << (isVisible ? L"true" : L"false") << std::endl;
        std::wcout << L"  Owner: " << owner << std::endl;
        std::wcout << L"  Parent: " << parentHwnd << std::endl;
        std::wcout << L"  RootOwner: " << rootOwnerHwnd << std::endl;
        std::wcout << L"  Style: 0x" << std::hex << style << std::dec << std::endl;
        std::wcout << L"  ExStyle: 0x" << std::hex << exStyle << std::dec << std::endl;

        if (isVisible && owner == (HWND)0) { // Keep initial check, but PID finding is the issue
            DWORD processId = 0; // Initialize
            GetWindowThreadProcessId(foregroundWindow, &processId); // Attempt 1
            DWORD lastErrorAttempt1 = GetLastError(); // Capture error for Attempt 1 immediately
            std::cout << "getFocusedProcessInfo: HWND " << foregroundWindow << " - PID from GetWindowThreadProcessId (attempt 1): " << processId << ". GetLastError(): " << lastErrorAttempt1 << std::endl;

            if (processId == 0) {
                // If Attempt 1 yielded PID 0 and GetLastError was also 0 (the specific problematic case from user logs)
                if (lastErrorAttempt1 == 0) {
                    std::cout << "getFocusedProcessInfo: Entered 'lastErrorAttempt1 == 0' block." << std::endl;
                    HWND rootWindow = GetAncestor(foregroundWindow, GA_ROOT);
                    std::cout << "getFocusedProcessInfo: GetAncestor(GA_ROOT) returned HWND: " << rootWindow << std::endl;

                    if (rootWindow && rootWindow != foregroundWindow) {
                        std::cout << "getFocusedProcessInfo: Attempt 1 had PID 0 and GLE 0. Retrying GetWindowThreadProcessId with root window HWND: " << rootWindow << std::endl;
                        GetWindowThreadProcessId(rootWindow, &processId); // Attempt 2 on root window
                        DWORD lastErrorAttempt2 = GetLastError(); // Capture error for Attempt 2
                        std::cout << "getFocusedProcessInfo: HWND " << rootWindow << " (Root) - PID from GetWindowThreadProcessId (attempt 2): " << processId
                                  << ". GetLastError(): " << lastErrorAttempt2 << std::endl;
                        // If attempt 2 also failed with an error, log its specific error
                        if (processId == 0 && lastErrorAttempt2 != 0) {
                             std::cerr << "getFocusedProcessInfo: GetWindowThreadProcessId (attempt 2) for root HWND: " << rootWindow
                                        << " returned PID 0. GetLastError(): " << lastErrorAttempt2 << std::endl;
                        }
                    } else if (rootWindow == foregroundWindow) {
                        std::cout << "getFocusedProcessInfo: Foreground window is already the root window. GA_ROOT attempt skipped." << std::endl;
                    } else { // This covers rootWindow being NULL
                         std::cout << "getFocusedProcessInfo: No distinct root window found (GetAncestor returned NULL or same as foreground). GA_ROOT attempt skipped." << std::endl;
                    }
                }

                // If processId is still 0 (either after Attempt 1 with a non-zero GLE, or after Attempt 2 also failed to yield a PID)
                if (processId == 0) {
                    std::cout << "getFocusedProcessInfo: GetWindowThreadProcessId attempts failed or yielded PID 0. Attempting full alternative PID lookup for original HWND: " << foregroundWindow << std::endl;
                    processId = GetProcessIdFromWindowHandle_Alternative(foregroundWindow);
                    std::cout << "getFocusedProcessInfo: HWND " << foregroundWindow << " - PID from Full Alternative method: " << processId << std::endl;
                }
            }


            if (processId != 0) {
                // Additional check: Ensure it's not our own process to avoid self-suspension/termination
                if (processId == GetCurrentProcessId()) {
                    std::wcout << L"getFocusedProcessInfo: Foreground window (PID: " << processId
                               << L", Class: " << className << L") belongs to this application. Ignoring." << std::endl;
                    return focusedProcess; // Return empty
                }

                focusedProcess.pid = processId;
                focusedProcess.name = getProcessNameByPid(processId);
                std::wcout << L"getFocusedProcessInfo: Identified focused process: "
                          << (focusedProcess.name.empty() ? L"[Unnamed]" : focusedProcess.name.c_str())
                          << L" (PID: " << focusedProcess.pid << L", HWND: " << foregroundWindow << L")" << std::endl;
            } else {
                 std::wcerr << L"getFocusedProcessInfo: Still could not get PID for HWND: " << foregroundWindow << std::endl;
            }
        } else {
            std::wcout << L"getFocusedProcessInfo: Foreground window (HWND: " << foregroundWindow
                      << L") is not considered a target. Visible: " << (isVisible ? L"true" : L"false")
                      << L", Owner: " << owner << std::endl;
        }
    } else {
        std::wcerr << L"getFocusedProcessInfo: GetForegroundWindow() returned NULL." << std::endl;
    }
    return focusedProcess;
}

bool ProcessManager::isCriticalProcess(const std::wstring& processName) {
    if (processName.empty()) {
        return false; // Or true, depending on desired behavior for unnamed processes
    }
    std::wstring lowerProcessName = processName;
    std::transform(lowerProcessName.begin(), lowerProcessName.end(), lowerProcessName.begin(), ::towlower);

    for (const auto& criticalName : criticalProcessNames) {
        if (lowerProcessName == criticalName) {
            return true;
        }
    }
    return false;
}

std::vector<ProcessManager::ProcessInfo> ProcessManager::getRunningProcesses() {
    std::vector<ProcessInfo> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                ProcessInfo info;
                info.pid = pe32.th32ProcessID;
                info.name = pe32.szExeFile;
                processes.push_back(info);
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return processes;
}

bool ProcessManager::suspendProcess(DWORD pid) {
    if (pid == 0) {
        std::wcerr << L"suspendProcess: PID is 0." << std::endl;
        return false;
    }
    ProcessInfo pInfo = {pid, getProcessNameByPid(pid)};
    if (pInfo.name.empty() && pid !=0) { // Check if name is empty but PID is not 0 (might indicate issue with getProcessNameByPid)
        std::wcout << L"Warning: Could not get process name for PID " << pid << L" during suspend. Proceeding with suspension." << std::endl;
    }

    if (isCriticalProcess(pInfo.name)) {
        std::wcout << L"Suspend: Cannot suspend critical process: " << (pInfo.name.empty() ? L"[Unnamed]" : pInfo.name.c_str()) << L" (PID: " << pid << L")" << std::endl;
        return false;
    }
    std::wcout << L"Suspend: Attempting to suspend process " << (pInfo.name.empty() ? L"[Unnamed]" : pInfo.name.c_str()) << L" (PID: " << pid << L")" << std::endl;

    if (!EnableDebugPrivilege()) {
        std::wcout << L"Warning (Suspend): Failed to enable SeDebugPrivilege. Suspension might fail for protected processes." << std::endl;
    }

    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (hProcess == NULL) {
        std::wcerr << L"Suspend: OpenProcess failed for PID " << pid << L". Error: " << GetLastError() << std::endl;
        return false;
    }

    NTSUSP NtSuspendProcess = (NTSUSP)GetProcAddress(GetModuleHandleA("ntdll"), "NtSuspendProcess");
    if (NtSuspendProcess == NULL) {
        std::wcerr << L"Suspend: GetProcAddress for NtSuspendProcess failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    NTSTATUS status = NtSuspendProcess(hProcess);
    CloseHandle(hProcess);

    if (NT_SUCCESS(status)) {
        std::wcout << L"Suspend: Successfully suspended process " << (pInfo.name.empty() ? L"[Unnamed]" : pInfo.name.c_str()) << L" (PID: " << pid << L"). Status: 0x" << std::hex << status << std::dec << std::endl;
        return true;
    } else {
        std::wcerr << L"Suspend: NtSuspendProcess failed for PID " << pid << L". Status: 0x" << std::hex << status << std::dec << std::endl;
        return false;
    }
}

bool ProcessManager::resumeProcess(DWORD pid) {
    if (pid == 0) {
        std::wcerr << L"resumeProcess: PID is 0." << std::endl;
        return false;
    }
    // It's useful to know the name for logging, even if we don't block critical processes from resuming.
    std::wstring processName = getProcessNameByPid(pid);
    std::wcout << L"Resume: Attempting to resume process " << (processName.empty() ? L"[Unnamed]" : processName.c_str()) << L" (PID: " << pid << L")" << std::endl;

    if (!EnableDebugPrivilege()) {
        std::wcout << L"Warning (Resume): Failed to enable SeDebugPrivilege. Resumption might fail for protected processes." << std::endl;
    }

    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (hProcess == NULL) {
        std::wcerr << L"Resume: OpenProcess failed for PID " << pid << L". Error: " << GetLastError() << std::endl;
        return false;
    }

    NTRESU NtResumeProcess = (NTRESU)GetProcAddress(GetModuleHandleA("ntdll"), "NtResumeProcess");
    if (NtResumeProcess == NULL) {
        std::wcerr << L"Resume: GetProcAddress for NtResumeProcess failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    NTSTATUS status = NtResumeProcess(hProcess);
    CloseHandle(hProcess);

    if (NT_SUCCESS(status)) {
        std::wcout << L"Resume: Successfully resumed process " << (processName.empty() ? L"[Unnamed]" : processName.c_str()) << L" (PID: " << pid << L"). Status: 0x" << std::hex << status << std::dec << std::endl;
        return true;
    } else {
        std::wcerr << L"Resume: NtResumeProcess failed for PID " << pid << L". Status: 0x" << std::hex << status << std::dec << std::endl;
        return false;
    }
}


bool ProcessManager::terminateProcess(DWORD pid) {
    if (pid == 0) return false;

    ProcessInfo pInfo = {pid, getProcessNameByPid(pid)};
    if (isCriticalProcess(pInfo.name)) {
        std::wcout << L"Cannot terminate critical process: " << pInfo.name << L" (PID: " << pid << L")" << std::endl;
        return false;
    }

    // Attempt to enable SeDebugPrivilege for more forceful termination
    if (!EnableDebugPrivilege()) {
        std::wcout << L"Warning: Failed to enable SeDebugPrivilege. Termination might not override all protections." << std::endl;
        // Continue without it, but log the warning.
    }

    // PROCESS_ALL_ACCESS provides more permissions, including terminate.
    // Also, try to open with more access rights.
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        // Fallback to just PROCESS_TERMINATE if PROCESS_ALL_ACCESS fails (e.g., due to insufficient privileges even with SeDebugPrivilege)
        hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL) {
            std::wcerr << L"Failed to open process " << pid << L" for termination. Error: " << GetLastError() << std::endl;
            return false;
        }
    }

    // Terminate the process
    // The second parameter to TerminateProcess is the exit code for the terminated process.
    // Using 1 (or any non-zero value) often indicates an abnormal termination.
    if (!TerminateProcess(hProcess, 1)) {
        std::wcerr << L"Failed to terminate process " << pid << L". Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Optionally, wait for the process to actually terminate
    // WaitForSingleObject(hProcess, 5000); // Wait up to 5 seconds

    CloseHandle(hProcess);
    std::wcout << L"Successfully initiated termination for process: " << pInfo.name << L" (PID: " << pid << L")" << std::endl;
    return true;
}