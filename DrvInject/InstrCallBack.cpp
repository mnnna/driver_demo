#include "InstrCallBack.h"
#include "Logger.h"

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
	PUCHAR pDllMem =  install_callback_get_dll_memory(us_dll_path);

	return NTSTATUS();
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
