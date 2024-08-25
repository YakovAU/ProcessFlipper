# process_killer.pyx

from libc.stdint cimport uintptr_t
from cpython.mem cimport PyMem_Malloc, PyMem_Free
cimport cython
from libc.string cimport memcpy

cdef extern from "Windows.h":
    ctypedef void * HANDLE
    ctypedef unsigned long DWORD
    ctypedef long LONG
    ctypedef int BOOL
    ctypedef char * LPCSTR
    ctypedef void * HMODULE
    ctypedef void * FARPROC

    HANDLE OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
    HANDLE OpenThread(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId)
    BOOL CloseHandle(HANDLE hObject)
    DWORD GetLastError()
    HMODULE LoadLibraryA(LPCSTR lpLibFileName)
    FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
    BOOL FreeLibrary(HMODULE hLibModule)
    DWORD FormatMessageA(DWORD dwFlags, void * lpSource, DWORD dwMessageId, DWORD dwLanguageId, char * lpBuffer,
                         DWORD nSize, void * Arguments)
    DWORD SuspendThread(HANDLE hThread)
    DWORD ResumeThread(HANDLE hThread)
    HANDLE CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
    BOOL Thread32First(HANDLE hSnapshot, void * lpte)
    BOOL Thread32Next(HANDLE hSnapshot, void * lpte)

ctypedef LONG NTSTATUS
ctypedef NTSTATUS (*NtTerminateProcessFunc)(HANDLE ProcessHandle, NTSTATUS ExitStatus)

PROCESS_TERMINATE = 0x0001
PROCESS_SUSPEND_RESUME = 0x0800
THREAD_SUSPEND_RESUME = 0x0002
STATUS_SUCCESS = 0
FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000
FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100
LANG_NEUTRAL = 0x00
TH32CS_SNAPTHREAD = 0x00000004

cdef struct THREADENTRY32:
    DWORD dwSize
    DWORD cntUsage
    DWORD th32ThreadID
    DWORD th32OwnerProcessID
    LONG tpBasePri
    LONG tpDeltaPri
    DWORD dwFlags

cdef bytes int_to_bytes(int value):
    cdef bytes result = str(value).encode('ascii')
    return result

cdef bytes get_error_message(DWORD error_code):
    cdef DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
    cdef char * message_buffer = NULL
    cdef DWORD size = FormatMessageA(flags, NULL, error_code, LANG_NEUTRAL, <char *> &message_buffer, 0, NULL)

    if size == 0:
        return b"Unknown error (code: " + int_to_bytes(error_code) + b")"

    cdef bytes message = message_buffer[:size]
    cdef bytes error_code_bytes = int_to_bytes(error_code)
    return b"Error " + error_code_bytes + b": " + message.strip()

cdef force_kill_process_internal(int pid):
    cdef HANDLE process_handle
    cdef NTSTATUS status
    cdef HMODULE ntdll
    cdef NtTerminateProcessFunc NtTerminateProcess
    cdef DWORD error

    ntdll = LoadLibraryA("ntdll.dll")
    if ntdll == NULL:
        error = GetLastError()
        raise OSError(get_error_message(error).decode('utf-8'))

    NtTerminateProcess = <NtTerminateProcessFunc> GetProcAddress(ntdll, "NtTerminateProcess")
    if NtTerminateProcess == NULL:
        error = GetLastError()
        FreeLibrary(ntdll)
        raise OSError(get_error_message(error).decode('utf-8'))

    process_handle = OpenProcess(PROCESS_TERMINATE, False, pid)
    if process_handle == NULL:
        error = GetLastError()
        FreeLibrary(ntdll)
        raise OSError(get_error_message(error).decode('utf-8'))

    try:
        status = NtTerminateProcess(process_handle, STATUS_SUCCESS)
        if status != STATUS_SUCCESS:
            raise OSError(f"Failed to terminate process. NTSTATUS: 0x{status:08X}")
    finally:
        CloseHandle(process_handle)
        FreeLibrary(ntdll)

cdef suspend_resume_process_internal(int pid, bint suspend):
    cdef HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    if h_snapshot == NULL:
        error = GetLastError()
        raise OSError(get_error_message(error).decode('utf-8'))

    cdef THREADENTRY32 te
    cdef BOOL success
    cdef HANDLE h_thread
    cdef DWORD result

    te.dwSize = sizeof(THREADENTRY32)
    success = Thread32First(h_snapshot, &te)

    while success:
        if te.th32OwnerProcessID == pid:
            h_thread = OpenThread(THREAD_SUSPEND_RESUME, False, te.th32ThreadID)
            if h_thread != NULL:
                if suspend:
                    result = SuspendThread(h_thread)
                else:
                    result = ResumeThread(h_thread)
                if result == -1:
                    error = GetLastError()
                    CloseHandle(h_thread)
                    CloseHandle(h_snapshot)
                    raise OSError(get_error_message(error).decode('utf-8'))
                CloseHandle(h_thread)
        success = Thread32Next(h_snapshot, &te)

    CloseHandle(h_snapshot)

def force_kill_process(int pid):
    force_kill_process_internal(pid)
    print(f"Process with PID {pid} has been forcefully terminated.")

def suspend_process(int pid):
    suspend_resume_process_internal(pid, True)
    print(f"Process with PID {pid} has been suspended.")

def resume_process(int pid):
    suspend_resume_process_internal(pid, False)
    print(f"Process with PID {pid} has been resumed.")
