#include "InstrCallBack.h"
#include "Logger.h"
#include"ShellCode.h"
//要注入的 DLL 读到内存中


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


	KeStackAttachProcess(Process, &Apc); //附加
	// 读到内存-----
	PUCHAR pDllMem =  install_callback_get_dll_memory(us_dll_path);
	// 读到内存-----

	return NTSTATUS();
}

NTSTATUS inst_callback_alloc_memory(PUCHAR p_dll_memory, _Out_  PVOID* _inst_callbak_addr, _Out_ PVOID p_manual_data) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PEPROCESS Process = { 0 };
	char* pStartMapAdd = 0;
	size_t AllocSize = 0;
	size_t RetSize = 0;

	IMAGE_NT_HEADERS* pNTHeader = nullptr;
	IMAGE_FILE_HEADER* pFileHeader = nullptr;

	if (!reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_magic !=  0x5A4D){  // 5A4D
		status = STATUS_INVALID_PARAMETER;
		Log("Is not a valid PE", true, status);
		return status;
	} 

	pNTHeader = (IMAGE_NT_HEADERS*)((ULONG_PTR)p_dll_memory + reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_lfanew);
	pFileHeader = &pNTHeader->FileHeader; // &?

	if (pFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64) {  //x64
		status = STATUS_NOT_SUPPORTED;
		Log("Is not a x64 PE", true, status);
		return status;
	}

	status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&pStartMapAdd, NULL, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//申请3环的游戏的内存 （r3 的内存）


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
		if (!NT_SUCCESS(status)) {  //x64
			Log("FAILED to load section", true, status);
			return status;
		}
		}
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

	status = NtQueryInformationFile(hFile, &IoStatusBlock, &fileinfo, sizeof(fileinfo), FileStandardInformation);// 获取 dll 的大小
	fileSize = fileinfo.AllocationSize.QuadPart;
	if (!NT_SUCCESS(status)) {
		Log("failed to get size info ", true, status);
		status = STATUS_UNSUCCESSFUL;
		return 0;
	}
	fileSize += 0x1000; // 内存对齐
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
