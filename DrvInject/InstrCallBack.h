#pragma once

#include <ntddk.h>
#include <ntifs.h>

UINT64 g_fnLoadLibrary = NULL;
UINT64 g_fnGetProcAddress = NULL;
UINT64 g_fnRtlAddFunction = NULL;