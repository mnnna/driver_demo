#include "InstrCallBack.h"
#include "Logger.h"
#include"ShellCode.h"

UINT64 g_fnLoadLibrary = NULL;
UINT64 g_fnGetProcAddress = NULL;
UINT64 g_fnRtlAddFunction = NULL;
//DWORD32 g_dwPid = NULL;
//wchar_t* g_zDllName = NULL;


#pragma warning(disable:6001);
//要注入的 DLL读到内存中
#pragma pack(push)    //保存当前内存对齐状态
#pragma pack(1) //设置内存对齐值为1 相当于没有内存对齐的概念
struct shellcode_t {
private:
	char padding[43];//43
public:
	uintptr_t manual_data;//8 重定位结构体
private:
	char pdding[47];
public:
	uintptr_t rip;
	uintptr_t shellcode;
};

//shell_code
char g_instcall_shellcode[] =
{
	0x50,//push rax
	0x51, //push  rcx   
	0x52, //push  rdx
	0x53, //push  rbx												//
	0x55, 															//
	0x56, 															//
	0x57, 															//
	0x41, 0x50, 													//
	0x41, 0x51, 													//
	0x41, 0x52, 													//
	0x41, 0x53, 													//
	0x41, 0x54, 													//
	0x41, 0x55, 													//
	0x41, 0x56, 													//
	0x41, 0x57, 													//
	//上面都是保存寄存器
	// sub rsp,0x20
	//把rsp保存过去
	0x48,0x89,0x25,0x4c,0x00,0x00,0x00,//将rsp保存
	0x48,0x83,0xec,0x38,
	0x48,0x81,0xe4,0xf0,0xff,0xff,0xff, //强行对齐

	//00000217F568001 | 48:83EC 20 | sub rsp,0x20 |
	//00000217F568001 | 48 : 83C4 20 | add rsp,0x20 |
	//Call ShellCode 进行重定位

	0x48, 0xB9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  //mov rcx,重定位数据

	0xFF, 0x15, 0x29, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,//call 地址

	//恢复寄存器
	0x48,0x8b,0x25,0x22,0x00,0x00,0x00,//将原来的rsp恢复
	//add rsp,0x20
	//pop 寄存器
	0x41, 0x5F,
	0x41, 0x5E,
	0x41, 0x5D,
	0x41, 0x5C,
	0x41, 0x5B,
	0x41, 0x5A,
	0x41, 0x59,
	0x41, 0x58,
	0x5F,
	0x5E,
	0x5D,
	0x5B,
	0x5A,
	0x59,
	0x58,//pop rax
	0x41, 0xFF, 0xE2,  //jmp r10 返回  不是InstCall注入 RIP要换地方
	//0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,//call 地址
	0,0,0,0,0,0,0,0,  // rip
	0,0,0,0,0,0,0,0 //原来的rsp放在这
};
#pragma pack(pop) //恢复当前内存对齐状态

NTSTATUS inst_callback_set_callback(PVOID insta_callback) {
	NTSTATUS status = STATUS_SUCCESS;
	PVOID InstCallBack = insta_callback;
	PACCESS_TOKEN Token = { 0 };
	PEPROCESS PE = { 0 };
	PULONG TokenMask = { 0 };

	PE = (PEPROCESS) IoGetCurrentProcess();
	Token = PsReferencePrimaryToken(PE);
	TokenMask = (PULONG)((ULONG_PTR)Token + 0x40);

	TokenMask[0] |= 0x100000;
	TokenMask[1] |= 0x100000;
	TokenMask[2] |= 0x100000;

	status = ZwSetInformationProcess(NtCurrentProcess(), ProcessInstrumentationCallback, &insta_callback, sizeof(PVOID)); 

	if (!NT_SUCCESS(status)) {  //x64
		Log("FAILED to set instrcallback", true, status);
	}
	else {
		Log("set instrcallback successfully", false, 0);
	}

	return status;

}

NTSTATUS inst_callback_inject(HANDLE process_id, UNICODE_STRING* us_dll_path)
{	
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS Process = { 0 };
	KAPC_STATE Apc = { 0 };
	PUCHAR pDllMem = { 0 };
	PVOID InstCallBack =  0 , pManualMapData =0;

	status = PsLookupProcessByProcessId(process_id, &Process);

	if (!NT_SUCCESS(status)) {
		Log("failed to get process", true, 0);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}


	KeStackAttachProcess(Process, &Apc); //附加
	while (TRUE) {
		pDllMem = install_callback_get_dll_memory(us_dll_path); 	// 读到内存-----
		if (!pDllMem) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		status = inst_callback_alloc_memory(pDllMem, &InstCallBack, &pManualMapData); //  手动加载 dll ， 模拟 dll 的加载流程

		if (!NT_SUCCESS(status))  break; 

		// 设置 instrucallback

		status = inst_callback_set_callback(InstCallBack); // 设置 instrcmatiion callback 指向 shellcode
		break;
	}

	if (pDllMem && MmIsAddressValid(pDllMem)) {
		__try {
			while (1) {
				if (((Manual_Mapping_data*)pManualMapData)->bStart) break;
			}
		}
		__except (1) {
			Log("process exit!", true,0);
			ObDereferenceObject(Process);
			KeUnstackDetachProcess(&Apc);
			return status;
		}
		
	}
	inst_callback_set_callback(0); //卸载  

	if (pDllMem && MmIsAddressValid(pDllMem) && PsLookupProcessByProcessId(process_id, &Process) != STATUS_PENDING) {
		__try {
			while (1) {
				*(PUCHAR)((((Manual_Mapping_data*)pManualMapData))->pBase) = 0;
				((Manual_Mapping_data*)pManualMapData)->bContinue = true; 
			}
		}
		__except (1) {
			Log("process exit!", true, 0);
		}
	}


	ObDereferenceObject(Process); // 取消引用 Eprocess
	KeUnstackDetachProcess(&Apc); // 取消附加
	if (pDllMem && MmIsAddressValid(pDllMem)) ExFreePool(pDllMem);

	return status ;
}

NTSTATUS inst_callback_alloc_memory(PUCHAR p_dll_memory, _Out_  PVOID* _inst_callbak_addr, _Out_ PVOID* p_manual_data) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process = { 0 };
	char* pStartMapAdd = 0;
	size_t AllocSize = 0;
	size_t RetSize = 0;
	Manual_Mapping_data ManualMapData = { 0 };
	PVOID pManuaMapData = 0 , pShellCode=0;
	IMAGE_NT_HEADERS* pNTHeader = nullptr;
	IMAGE_FILE_HEADER* pFileHeader = nullptr;
	IMAGE_OPTIONAL_HEADER* pOptHeader = NULL;

	if (reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_magic !=  0x5A4D){  // 5A4D
		status = STATUS_INVALID_PARAMETER;
		Log("Is not a valid PE", true, status);
		return status;
	} 

	pNTHeader = (IMAGE_NT_HEADERS*)((ULONG_PTR)p_dll_memory + reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_lfanew);
	pFileHeader = &pNTHeader->FileHeader; // &?
	pOptHeader = &pNTHeader->OptionalHeader;

	if (pFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64) {  //x64
		status = STATUS_NOT_SUPPORTED;
		Log("Is not a x64 PE", true, status);
		return status;
	}
	AllocSize = pOptHeader->SizeOfImage;
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&pStartMapAdd, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//申请3环的游戏的内存 （r3 的内存）
	RtlSecureZeroMemory(pStartMapAdd, AllocSize);

	ManualMapData.dwReadson = 0;
	ManualMapData.pGetProcAddress = (f_GetProcAddress)g_fnGetProcAddress;
	ManualMapData.pLoadLibraryA = (f_LoadLibraryA)g_fnLoadLibrary;
	ManualMapData.pRtlAddFunctionTable = (f_RtlAddFunctionTable)g_fnRtlAddFunction;

	ManualMapData.pBase = pStartMapAdd;
	ManualMapData.bContinue = false;   
	ManualMapData.bFirst = true;
	ManualMapData.bStart = false;
	ManualMapData.DllSize = AllocSize;

	if (!NT_SUCCESS(status)) {  //x64
		Log("FAILED to get aollcate mem", true, status);
		return status;
	}

	RtlSecureZeroMemory(pStartMapAdd, sizeof(AllocSize)); 

	

	Process = (PEPROCESS)IoGetCurrentProcess();

	status = MmCopyVirtualMemory(Process, p_dll_memory, Process, pStartMapAdd, PAGE_SIZE, KernelMode, &RetSize);

	if (!NT_SUCCESS(status)) {  //x64
		Log("FAILED to load pe header", true, status);
		return status;
	}


	IMAGE_SECTION_HEADER* pSectionHeader =  IMAGE_FIRST_SECTION(pNTHeader); // 拿节区头
	for (int i = 0; i < pFileHeader->NumberOfSections; i++, pSectionHeader++) {
		if (pSectionHeader->SizeOfRawData) {
			status = MmCopyVirtualMemory(Process, p_dll_memory+ pSectionHeader->PointerToRawData , Process, pStartMapAdd + pSectionHeader->VirtualAddress , pSectionHeader->SizeOfRawData, KernelMode, &RetSize); 
			if (!NT_SUCCESS(status)) {  
					Log("FAILED to load section", true, status);
					return status;
			}
		}
	}

	
	AllocSize = PAGE_SIZE;
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), &pManuaMapData, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {  
		Log("FAILED to allocmem", true, status);
		return status;
	}
	RtlSecureZeroMemory(pManuaMapData, sizeof(AllocSize));

	status = MmCopyVirtualMemory(Process, &ManualMapData, Process, pManuaMapData, sizeof(ManualMapData), KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {  
		Log("FAILED to wirte  mem for maunaldata", true, status);
		return status;
	}

	// 映射 shellcode, 把 shellcode 加载到内存中
	ZwAllocateVirtualMemory(NtCurrentProcess(), &pShellCode, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to alloc  mem for shellcode", true, status);
		return status;
	}
	RtlSecureZeroMemory(pShellCode, sizeof(AllocSize));

	status = MmCopyVirtualMemory(Process, InstruShellCode, Process, pShellCode, AllocSize, KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to write mem  for shellcode", true, status);
		return status;
	}

	//  g_instcall_shellcode

	shellcode_t shell_code; 
	memset(&shell_code, 0, sizeof(shell_code));
	memcpy(&shell_code, &g_instcall_shellcode, sizeof(shellcode_t));

	shell_code.manual_data = (UINT64)pManuaMapData;  //传进 rcx 当做参数
	shell_code.rip = (UINT64)pShellCode;

	status = ZwAllocateVirtualMemory(NtCurrentProcess(), _inst_callbak_addr, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to alloc mem for instrcall shellcode", true, status);
		return status;
	}
	RtlSecureZeroMemory(*_inst_callbak_addr, sizeof(AllocSize));

	status = MmCopyVirtualMemory(Process, &shell_code, Process, *_inst_callbak_addr, sizeof(shellcode_t), KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to write mem for instrcall shellcode", true, status);
		return status;
	}

	// 
	*p_manual_data = pManuaMapData;
	return status;
}

PUCHAR install_callback_get_dll_memory(UNICODE_STRING* us_dll_path)
{
	HANDLE hFile;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES objattr = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	LARGE_INTEGER lainter = { 0 };
	LARGE_INTEGER byteOffset = { 0 };
	FILE_STANDARD_INFORMATION fileinfo = {0};
	ULONG64 fileSize = 0;
	PUCHAR pDllMem = { 0 };

	InitializeObjectAttributes(&objattr, us_dll_path, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status =  ZwCreateFile(&hFile, GENERIC_READ, &objattr, &IoStatusBlock, &lainter, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ| FILE_SHARE_WRITE| FILE_SHARE_DELETE , FILE_OPEN, 0,0,0);
	if (!NT_SUCCESS(status)) {
		Log("failed to ctreate dll", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}

	status = NtQueryInformationFile(hFile, &IoStatusBlock, &fileinfo, sizeof(fileinfo), FileStandardInformation);// 获取 dll 的大小
	fileSize = fileinfo.AllocationSize.QuadPart;
	if (!NT_SUCCESS(status)) {
		Log("failed to get size info ", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}
	fileSize += 0x1000; // 内存对齐
	fileSize = (ULONG64)PAGE_ALIGN(fileSize);

	pDllMem = (PUCHAR)ExAllocatePoolWithTag(PagedPool, fileSize, 'Dllp');
	RtlSecureZeroMemory(pDllMem,fileSize);

	status =  ZwReadFile(hFile, NULL, NULL, NULL, &IoStatusBlock, pDllMem, fileSize, &byteOffset, NULL);
	ZwFlushBuffersFile(hFile, &IoStatusBlock);
	if (!NT_SUCCESS(status)) {
		ExFreePool(pDllMem);
		ZwClose(hFile);
		Log("failed to read file ", true, status);
		return 0;

	}
	ZwClose(hFile);
	return pDllMem;
}
