#pragma once
#include <ntifs.h>
#include <fltKernel.h>
#include  <dontuse.h>

// include fltmgr.lib
namespace common {
	typedef NTSTATUS(*fnCommoFunctiuon)(void* inbuf/*���뻺����*/, ULONG inlen/*���������*/, void* outbuf, ULONG outlen, PULONG writenlen /*ʵ��ͨѶ�ֽ�*/);

	inline PFLT_FILTER fltFilter(nullptr);
	inline PFLT_PORT drvPort(nullptr);
	inline PFLT_PORT ClientPort(nullptr);

	auto createCommonPort(PDRIVER_OBJECT driverObject, PUNICODE_STRING RegisterPath) -> NTSTATUS;
}