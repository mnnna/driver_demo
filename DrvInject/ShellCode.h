#pragma once
#include<ntifs.h>
#include<ntddk.h>
#include<ntimage.h>

#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)
#define RELOC_FLAG RELOC_FLAG64



typedef PVOID HINSTANCE, HMODULE;

using f_LoadLibraryA = HINSTANCE(__stdcall*)(const char* lpLibFilename);
using f_GetProcAddress = PVOID(__stdcall*)(HMODULE hModule, LPCSTR lpProcName);
using f_DLL_ENTRY_POINT = BOOLEAN(__stdcall*)(void* hDll, DWORD32 dwReason, void* pReserved);
using f_RtlAddFunctionTable = BOOLEAN(__stdcall*)(_IMAGE_RUNTIME_FUNCTION_ENTRY* FunctionTable, DWORD32 EntryCount, DWORD64 BaseAddress);

extern UINT64 g_fnLoadLibrary;
extern UINT64 g_fnGetProcAddress;
extern UINT64 g_fnRtlAddFunction;
//DWORD32 g_dwPid = NULL;
//wchar_t* g_zDllName = NULL;

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

EXTERN_C
NTSTATUS
MmCopyVirtualMemory(PEPROCESS FromProcess, PVOID FromAddress,
	PEPROCESS ToProcess, PVOID ToAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode,
	PSIZE_T NumberOfBytesCopied);





struct Manual_Mapping_data//�ڴ�ӳ��dll����
{
	//�����ض�λIAT
	f_LoadLibraryA pLoadLibraryA;
	f_GetProcAddress pGetProcAddress;
	//x64ר��
	f_RtlAddFunctionTable pRtlAddFunctionTable;

	char* pBase;
	DWORD32 dwReadson;
	PVOID reservedParam;
	BOOLEAN bFirst;//ֻ�����һ��ϵͳ���ý���
	BOOLEAN bStart;//��ʼִ��shellcode�� ����ͬ���ص�InstCallBack
	BOOLEAN bContinue;//���ڼ���ִ�� ȥ��PEͷ��ִ��dllmain
	//BOOLEAN clean;//���Ըı������� ȥ��PEͷ��
	size_t DllSize;
};

void __stdcall InstruShellCode(Manual_Mapping_data* pData);