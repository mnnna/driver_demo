#pragma once
#include <ntddk.h>
#include "hde/hde64.h"

extern bool hide_mem(HANDLE pid, void* va, ULONG attribute);