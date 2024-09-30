#pragma once
#include <ntifs.h>
#include <fltKernel.h>
#include  <dontuse.h>

// include fltmgr.lib
namespace common {
	inline PFLT_FILTER fltFilter(nullptr);
	inline PFLT_PORT drvPort(nullptr);
	inline PFLT_PORT ClientPort(nullptr);

	auto createCommonPort(PDRIVER_OBJECT driverObject, PUNICODE_STRING RegisterPath) -> NTSTATUS;
}