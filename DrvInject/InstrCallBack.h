#pragma once

#include "ShellCode.h"
#include "Logger.h"


NTSTATUS inst_callback_inject(HANDLE  process_id, UNICODE_STRING* us_dll_path);
PUCHAR install_callback_get_dll_memory(UNICODE_STRING* us_dll_path); 

NTSTATUS inst_callback_alloc_memory(HANDLE PID, PUCHAR p_dll_memory, _Out_ PVOID* inst_callbak_addr, _Out_ PVOID* p_manual_data);
PUCHAR inst_callback_get_dll_memory(UNICODE_STRING* us_dll_path);