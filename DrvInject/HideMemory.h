#pragma once
#include <ntddk.h>


extern bool hide_mem(HANDLE pid, void* va, ULONG attribute);