#include "InstrCallBack.h"
#include "Logger.h"
#include"ShellCode.h"

UINT64 g_fnLoadLibrary = 0;
UINT64 g_fnGetProcAddress = 0;
UINT64 g_fnRtlAddFunction = 0;
//DWORD32 g_dwPid = NULL;
//wchar_t* g_zDllName = NULL;


#pragma warning(disable:6001);
//Ҫע��� DLL�����ڴ���
#pragma pack(push)    //���浱ǰ�ڴ����״̬
#pragma pack(1) //�����ڴ����ֵΪ1 �൱��û���ڴ����ĸ���
struct shellcode_t {
private:
	char padding[43];//43
public:
	uintptr_t manual_data;//8 �ض�λ�ṹ��
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
	0x53, //push  rbx												  
	0x55, 			 						    // push rbp  
	0x56, 		                				// push rsi
	0x57, 										// push rdi 
	0x41, 0x50, 								// push r8 
	0x41, 0x51, 								// push r9
	0x41, 0x52, 		    				    // push r10
	0x41, 0x53, 							   //  push r11
	0x41, 0x54, 			  				  //   push r12
	0x41, 0x55,							      //   push r13 
	0x41, 0x56, 						      //   push r14
	0x41, 0x57, 							  //   push r15 
	//���涼�Ǳ���Ĵ���
	// sub rsp,0x20
	//��rsp�����ȥ
	0x48,0x89,0x25,0x4c,0x00,0x00,0x00,//��rsp����   // mov qword ptr ds:[233A145006A], rsp
	0x48,0x83,0xec,0x38,               // sub rsp,38
	0x48,0x81,0xe4,0xf0,0xff,0xff,0xff, //ǿ�ж���   and rsp, FFFFFFFFFFFFFFF0
	
	//00000217F568001 | 48:83EC 20 | sub rsp,0x20 |
	//00000217F568001 | 48 : 83C4 20 | add rsp,0x20 |
	//Call ShellCode �����ض�λ

	0x48, 0xB9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  //mov rcx,�ض�λ���� // mov rcx,<>

	0xFF, 0x15, 0x29, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,//call ��ַ // call qword ptr ds:[addr]

	//�ָ��Ĵ���
	0x48,0x8b,0x25,0x22,0x00,0x00,0x00,//��ԭ����rsp�ָ�
	//add rsp,0x20
	//pop �Ĵ���
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
	0x41, 0xFF, 0xE2,  //jmp r10 ����  ����InstCallע�� RIPҪ���ط�
	//0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,//call ��ַ
	0,0,0,0,0,0,0,0,  // rip
	0,0,0,0,0,0,0,0 //ԭ����rsp������
};
#pragma pack(pop) //�ָ���ǰ�ڴ����״̬

NTSTATUS inst_callback_set_callback(PVOID insta_callback) {
	NTSTATUS status = STATUS_SUCCESS;
	PACCESS_TOKEN Token{ 0 };
	PULONG TokenMask{ 0 };
	PVOID InstCallBack = insta_callback;//instcallback��ַ



	Token = PsReferencePrimaryToken(IoGetCurrentProcess());

	//���õ���λ
	TokenMask = (PULONG)((ULONG_PTR)Token + 0x40);
	//21λ��DEBUGȨ��(λ20)
	TokenMask[0] |= 0x100000;
	TokenMask[1] |= 0x100000;
	TokenMask[2] |= 0x100000;

	//����InstCallBack
	status = ZwSetInformationProcess(NtCurrentProcess(), ProcessInstrumentationCallback, &InstCallBack, sizeof(PVOID));

	if (!NT_SUCCESS(status)) Log("failed to set instcall back", true, status);
	else Log("set instcall back success", 0, 0);



	return status;

}

NTSTATUS inst_callback_inject(HANDLE process_id, UNICODE_STRING* us_dll_path)
{	
	PEPROCESS Process = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	KAPC_STATE Apc{ 0 };
	PUCHAR pDllMem = 0;
	PVOID InstCallBack = 0;//shellcode ���ڵ��ڴ��ַ����Ϊinstcallback�ĵ�ַ
	PVOID pManualMapData = 0, pShellCode = 0;//������ڴ�,һ����ӳ��ṹ���Ե�ַ,һ����ShellCode��ַ
	status = PsLookupProcessByProcessId(process_id, &Process);

	if (!NT_SUCCESS(status)) {
		Log("failed to get process", true, 0);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}


	KeStackAttachProcess(Process, &Apc); //����
	while (TRUE) {
		pDllMem = install_callback_get_dll_memory(us_dll_path); 	// �����ڴ�-----
		if (!pDllMem) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		status = inst_callback_alloc_memory(pDllMem, &InstCallBack, &pManualMapData); //  �ֶ����� dll �� ģ�� dll �ļ�������

		if (!NT_SUCCESS(status))  break; 

		// ���� instrucallback

		status = inst_callback_set_callback(InstCallBack); // ���� instrcmatiion callback ָ�� shellcode
		break;
	}
	DbgPrint("pDllMem: %p\n", pDllMem);
	if (pManualMapData && MmIsAddressValid(pManualMapData)) {
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
	inst_callback_set_callback(0); //ж��  

	if (pManualMapData && MmIsAddressValid(pManualMapData) && PsLookupProcessByProcessId(process_id, &Process) != STATUS_PENDING) {
		__try {
				DbgPrint("pbase: %s\n", (((Manual_Mapping_data*)pManualMapData))->pBase);
				*(PUCHAR)((((Manual_Mapping_data*)pManualMapData))->pBase) = 0;
				((Manual_Mapping_data*)pManualMapData)->bContinue = true; 

		}
		__except (1) {
			Log("process exit!2", true, 0);
		}
	}


	ObDereferenceObject(Process); // ȡ������ Eprocess
	KeUnstackDetachProcess(&Apc); // ȡ������
	if (pDllMem && MmIsAddressValid(pDllMem)) ExFreePool(pDllMem);
	return status ;
}

NTSTATUS inst_callback_alloc_memory(PUCHAR p_dll_memory, _Out_  PVOID* _inst_callbak_addr, _Out_ PVOID* p_manual_data) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process{ 0 };
	char* pStartMapAdd = 0; //R3���ַ ͨ��ZwAllocatevirtual Dll��PEͷ��ʼ�ĵ�ַ
	size_t AllocSize = 0;
	size_t RetSize = 0;
	size_t DllSize;
	Manual_Mapping_data ManualMapData = { 0 };
	PVOID pManuaMapData = 0 , pShellCode=0;
	IMAGE_NT_HEADERS* pNTHeader = nullptr;
	IMAGE_FILE_HEADER* pFileHeader = nullptr;
	IMAGE_OPTIONAL_HEADER* pOptHeader = nullptr;

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
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&pStartMapAdd, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//����3������Ϸ���ڴ� ��r3 ���ڴ棩
	if (!NT_SUCCESS(status)) {
		Log("failed to alloc memory", true, status);
		return status;
	}
	DllSize = AllocSize;
	RtlSecureZeroMemory(pStartMapAdd, AllocSize);


	//��ʼ��ManualMapData ����ṹ���ͨ��shellcode��RCX�Ĵ��������ض�λshellcode
	//����ͨ���ж���Щ�ṹ���еı�־λ����ʱȡ��instcall�ص�������ж��PE�ṹ
	ManualMapData.dwReadson = 0;
	ManualMapData.pGetProcAddress = (f_GetProcAddress)g_fnGetProcAddress;
	ManualMapData.pLoadLibraryA = (f_LoadLibraryA)g_fnLoadLibrary;
	ManualMapData.pRtlAddFunctionTable = (f_RtlAddFunctionTable)g_fnRtlAddFunction;

	ManualMapData.pBase = pStartMapAdd;
	ManualMapData.bContinue = false;   
	ManualMapData.bFirst = true;
	ManualMapData.bStart = false;
	ManualMapData.DllSize = DllSize;

	if (!NT_SUCCESS(status)) {  
		Log("FAILED to get aollcate mem", true, status);
		return status;
	}

	/*RtlSecureZeroMemory(pStartMapAdd, sizeof(AllocSize)); */

	

	Process = IoGetCurrentProcess();

	status = MmCopyVirtualMemory(Process, p_dll_memory, Process, pStartMapAdd, PAGE_SIZE, KernelMode, &RetSize);

	if (!NT_SUCCESS(status)) { 
		Log("FAILED to load pe header", true, status);
		return status;
	}


	IMAGE_SECTION_HEADER* pSectionHeader =  IMAGE_FIRST_SECTION(pNTHeader); // �ý���ͷ
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
		Log("FAILED to allocmem for manualMapData", true, status);
		return status;
	}
	RtlSecureZeroMemory(pManuaMapData, AllocSize);

	status = MmCopyVirtualMemory(Process, &ManualMapData, Process, pManuaMapData, sizeof(ManualMapData), KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {  
		Log("FAILED to wirte  mem for maunaldata", true, status);
		return status;
	}

	// ӳ�� shellcode, �� shellcode ���ص��ڴ���
	status =  ZwAllocateVirtualMemory(NtCurrentProcess(), &pShellCode, 0, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to alloc  mem for shellcode", true, status);
		return status;
	}
	RtlSecureZeroMemory(pShellCode, AllocSize);

	status = MmCopyVirtualMemory(Process, InstruShellCode, Process, pShellCode, AllocSize, KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to write mem  for shellcode", true, status);
		return status;
	}

	//  g_instcall_shellcode

	shellcode_t shell_code; 
	memset(&shell_code, 0, sizeof(shellcode_t));
	memcpy(&shell_code, &g_instcall_shellcode, sizeof(shellcode_t)); // ͨ���ṹ�������� shellcode

	shell_code.manual_data = (UINT64)pManuaMapData;  //���� rcx ��������
	shell_code.rip = (UINT64)pShellCode; //PE������

	status = ZwAllocateVirtualMemory(NtCurrentProcess(), _inst_callbak_addr, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("FAILED to alloc mem for instrcall shellcode", true, status);
		return status;
	}
	RtlSecureZeroMemory(*_inst_callbak_addr, AllocSize);

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
	HANDLE hFile = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES objattr = { 0 };
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	LARGE_INTEGER lainter = { 0 };
	LARGE_INTEGER byteOffset = { 0 };
	FILE_STANDARD_INFORMATION fileinfo = {0};
	ULONG64 FileSize = 0;
	PUCHAR pDllMemory = { 0 };

	InitializeObjectAttributes(&objattr, us_dll_path, OBJ_CASE_INSENSITIVE, 0, 0);
	status =  ZwCreateFile(&hFile, GENERIC_READ, &objattr, &IoStatusBlock, &lainter, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, FILE_OPEN, 0,0,0);
	if (!NT_SUCCESS(status)) {
		Log("failed to ctreate dll", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}

	status = ZwQueryInformationFile(hFile, &IoStatusBlock, &fileinfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);// ��ȡ dll �Ĵ�С
	FileSize = (ULONG64)fileinfo.AllocationSize.QuadPart;
	if (!NT_SUCCESS(status)) {
		Log("failed to get size info ", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}
	FileSize += 0x1000; // �ڴ����
	FileSize = (UINT64)PAGE_ALIGN(FileSize);

	pDllMemory = (PUCHAR)ExAllocatePoolWithTag(PagedPool, FileSize, 'Dllp');
	RtlSecureZeroMemory(pDllMemory, FileSize);

	status =  ZwReadFile(hFile, 0, 0, 0, &IoStatusBlock, pDllMemory, FileSize, &byteOffset, 0);
	ZwFlushBuffersFile(hFile, &IoStatusBlock);
	if (!NT_SUCCESS(status)) {
		ExFreePool(pDllMemory);
		ZwClose(hFile);

		Log("failed to read file content", true, status);
		return 0;
	}

	ZwClose(hFile);
	return pDllMemory;
}
