#pragma once
#include <ntifs.h>
#include <fltKernel.h>
#include  <dontuse.h>
#include <ntstrsafe.h>

// include fltmgr.lib
namespace common {
	typedef NTSTATUS(*fnCommoFunc)(void* inbuf/*���뻺����*/, ULONG inlen/*���������*/, void* outbuf, ULONG outlen, PULONG writenlen /*ʵ��ͨѶ�ֽ�*/);

	inline fnCommoFunc CommonFunc(nullptr);

	inline PFLT_FILTER fltFilter(nullptr);
	inline PFLT_PORT drvPort(nullptr);
	inline PFLT_PORT ClientPort(nullptr);

	auto createCommonPort(PDRIVER_OBJECT driverObject, PUNICODE_STRING RegisterPath, fnCommoFunc dunc) -> NTSTATUS;
}