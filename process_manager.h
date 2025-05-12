#pragma once
#include <iomanip>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

#include <string>
#include <vector>
#include <algorithm> // Required for std::transform
#include <Psapi.h> // Required for GetModuleFileNameEx

class ProcessManager {
public:
    struct ProcessInfo {
        DWORD pid;
        std::wstring name;
    };

    ProcessManager(); // Constructor to initialize critical processes

    std::vector<ProcessInfo> getRunningProcesses();
    ProcessInfo getFocusedProcessInfo();
    bool isCriticalProcess(const std::wstring& processName);

    bool suspendProcess(DWORD pid);
    bool resumeProcess(DWORD pid);
    bool terminateProcess(DWORD pid);

private:
    typedef LONG (NTAPI *NTSUSP)(HANDLE ProcessHandle);
    typedef LONG (NTAPI *NTRESU)(HANDLE ProcessHandle);

    std::vector<std::wstring> criticalProcessNames;

    std::wstring getProcessNameByPid(DWORD pid);
};

