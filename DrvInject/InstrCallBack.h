#pragma once

#include <ntddk.h>
#include <ntifs.h>

UINT64 g_fnLoadLibrary = NULL;
UINT64 g_fnGetProcAddress = NULL;
UINT64 g_fnRtlAddFunction = NULL;
DWORD32 g_dwPid = NULL ;
wchar_t* g_zDllName = NULL;

NTSTATUS inst_callback_inject(HANDLE  process_id, UNICODE_STRING* us_dll_path);
PUCHAR install_callback_get_dll_memory(UNICODE_STRING* us_dll_path); 