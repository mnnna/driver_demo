#pragma once
#include<ntifs.h>
#include<ntddk.h>
#include<ntimage.h>


EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
	__in HANDLE ProcessHandle,
	__in PROCESSINFOCLASS ProcessInformationClass,
	__in_bcount(ProcessInformationLength) PVOID ProcessInformation,
	__in ULONG ProcessInformationLength
);

EXTERN_C NTSTATUS MmCopyVirtualMemory(PEPROCESS FromProcess, PVOID FromAddress,
	PEPROCESS ToProcess, PVOID ToAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode,
	PSIZE_T NumberOfBytesCopied);
