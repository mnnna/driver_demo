#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <intrin.h>
#include <ntimage.h>

#pragma warning (disable : 4838)
#pragma warning (disable : 4309)

#define MAX_PATH 260

//ȷ���Ƿ���Ҫ�ض�λ
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#define RELOC_FLAG RELOC_FLAG64

#define MYLOG(text,is_err,...) DbgPrintEx(77,0,"[drv_inject]:");\
	if(is_err)DbgPrintEx(77,0,"err,func_name:%s,line:%d   ",__FUNCTION__,__LINE__);\
	DbgPrintEx(77,0,text,__VA_ARGS__);\
	DbgPrintEx(77,0,"\r\n");


#define X64 0x8664
#define X86 0x14c

//������д�ͻ��ض���
extern UINT64 g_fnLoadLibrary;
extern UINT64 g_fnGetProcAddress;
extern UINT64 g_fnAddFuntionTable;

typedef PVOID HINSTANCE, HMODULE;

using f_LoadLibraryA = HINSTANCE(__stdcall*)(const char* lpLibFilename);
using f_GetProcAddress = PVOID(__stdcall*)(HMODULE hModule, LPCSTR lpProcName);
using f_DLL_ENTRY_POINT = BOOLEAN(__stdcall*)(void* hDll, DWORD32 dwReason, void* pReserved);
using f_RtlAddFunctionTable = BOOLEAN(__stdcall*)(_IMAGE_RUNTIME_FUNCTION_ENTRY* FunctionTable, DWORD32 EntryCount, DWORD64 BaseAddress);

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

