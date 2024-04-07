#pragma once
#include<Windows.h>

BOOL LoadDriver(const char* lpszDrivername, const char* sysFileName);

BOOL UnloadDriver(const char* lpszDriverName);