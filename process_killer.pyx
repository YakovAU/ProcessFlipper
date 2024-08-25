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
    BOOL CloseHandle(HANDLE hObject)
    DWORD GetLastError()
    HMODULE LoadLibraryA(LPCSTR lpLibFileName)
    FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
    BOOL FreeLibrary(HMODULE hLibModule)
    DWORD FormatMessageA(DWORD dwFlags, void * lpSource, DWORD dwMessageId, DWORD dwLanguageId, char * lpBuffer,
                         DWORD nSize, void * Arguments)

ctypedef LONG NTSTATUS
ctypedef NTSTATUS (*NtTerminateProcessFunc)(HANDLE ProcessHandle, NTSTATUS ExitStatus)

PROCESS_TERMINATE = 0x0001
STATUS_SUCCESS = 0
FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000
FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100
LANG_NEUTRAL = 0x00

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

def force_kill_process(int pid):
    force_kill_process_internal(pid)
    print(f"Process with PID {pid} has been forcefully terminated.")

def main():
    pid = int(input("Enter the PID of the process to kill: "))
    try:
        force_kill_process(pid)
    except OSError as e:
        print(f"Failed to kill process: {e}")
        print("Note: This tool may require administrator privileges to work effectively.")

if __name__ == "__main__":
    main()