#include "InstrCallBack.h"
#include "Logger.h"
#include"ShellCode.h"
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
	//���涼�Ǳ���Ĵ���
	// sub rsp,0x20
	//��rsp�����ȥ
	0x48,0x89,0x25,0x4c,0x00,0x00,0x00,//��rsp����
	0x48,0x83,0xec,0x38,
	0x48,0x81,0xe4,0xf0,0xff,0xff,0xff, //ǿ�ж���

	//00000217F568001 | 48:83EC 20 | sub rsp,0x20 |
	//00000217F568001 | 48 : 83C4 20 | add rsp,0x20 |
	//Call ShellCode �����ض�λ

	0x48, 0xB9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,  //mov rcx,�ض�λ����

	0xFF, 0x15, 0x29, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,//call ��ַ

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
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0 //ԭ����rsp������
};
#pragma pack(pop) //�ָ���ǰ�ڴ����״̬


NTSTATUS inst_callback_inject(HANDLE process_id, UNICODE_STRING* us_dll_path)
{	
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS Process = { 0 };
	KAPC_STATE Apc = { 0 };
	PUCHAR pDllMem = { 0 };

	status = PsLookupProcessByProcessId(process_id, &Process);

	if (!NT_SUCCESS(status)) {
		Log("failed to get process", true, 0);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}


	KeStackAttachProcess(Process, &Apc); //����
	// �����ڴ�-----
	PUCHAR pDllMem =  install_callback_get_dll_memory(us_dll_path);
	// �����ڴ�-----

	return NTSTATUS();
}

NTSTATUS inst_callback_alloc_memory(PUCHAR p_dll_memory, _Out_  PVOID* _inst_callbak_addr, _Out_ PVOID p_manual_data) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process = { 0 };
	char* pStartMapAdd = 0;
	size_t AllocSize = 0;
	size_t RetSize = 0;
	Manual_Mapping_data ManualMapData = { 0 };
	PVOID pManuaMapData = 0 ;
	IMAGE_NT_HEADERS* pNTHeader = nullptr;
	IMAGE_FILE_HEADER* pFileHeader = nullptr;
	IMAGE_OPTIONAL_HEADER* pOptHeader = NULL;

	if (!reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_magic !=  0x5A4D){  // 5A4D
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
		Log("FAILED to allocmem", true, status);
		return status;
	}
	RtlSecureZeroMemory(pStartMapAdd, sizeof(AllocSize));

	status = MmCopyVirtualMemory(Process, &ManualMapData, Process, pManuaMapData, sizeof(pManuaMapData), KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {  
		Log("FAILED to wirte  mem for maunaldata", true, status);
		return status;
	}
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
	ULONG fileSize = 0;
	PUCHAR pDllMem = { 0 };

	InitializeObjectAttributes(&objattr, us_dll_path, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status =  ZwCreateFile(&hFile, GENERIC_READ, &objattr, &IoStatusBlock, &lainter, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ| FILE_SHARE_WRITE| FILE_SHARE_DELETE , FILE_OPEN, 0,0,0);
	if (!NT_SUCCESS(status)) {
		Log("failed to ctreate dll", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}

	status = NtQueryInformationFile(hFile, &IoStatusBlock, &fileinfo, sizeof(fileinfo), FileStandardInformation);// ��ȡ dll �Ĵ�С
	fileSize = fileinfo.AllocationSize.QuadPart;
	if (!NT_SUCCESS(status)) {
		Log("failed to get size info ", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}
	fileSize += 0x1000; // �ڴ����
	fileSize = (ULONG)PAGE_ALIGN(fileSize);

	pDllMem = (PUCHAR)ExAllocatePoolWithTag(PagedPool, fileSize, 'Dllp');
	RtlSecureZeroMemory(pDllMem,fileSize);

	status =  ZwReadFile(hFile, NULL, NULL, NULL, &IoStatusBlock, pDllMem, fileSize, &byteOffset, NULL);
	ZwFlushBuffersFile(hFile, &IoStatusBlock);
	if (!NT_SUCCESS(status)) {
		ExFreePool(pDllMem);
		ZwClose(hFile);
		Log("failed to read file ", true, status);
		return 0;
	

	ZwClose(hFile);



	return pDllMem;
}
