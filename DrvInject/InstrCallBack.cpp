#include "InstrCallBack.h"
#include "HideMemory.h"
#pragma warning (disable : 4996)

UINT64 g_fnLoadLibrary = 0;
UINT64 g_fnGetProcAddress = 0;
UINT64 g_fnAddFuntionTable = 0;

//DWORD32 g_dwPid = NULL;
//wchar_t* g_zDllName = NULL;

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

NTSTATUS inst_callback_set_callback(PVOID inst_callback) {
	NTSTATUS status = STATUS_SUCCESS;
	PACCESS_TOKEN Token{ 0 };
	PULONG TokenMask{ 0 };
	PVOID InstCallBack = inst_callback;//instcallback��ַ



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

NTSTATUS inst_callback_inject(HANDLE process_id, UNICODE_STRING* us_dll_path) {
	PEPROCESS Process{ 0 };
	NTSTATUS status = STATUS_SUCCESS;
	KAPC_STATE Apc{ 0 };
	PUCHAR pDllMem = 0;
	PVOID InstCallBack = 0;//shellcode ���ڵ��ڴ��ַ����Ϊinstcallback�ĵ�ַ
	PVOID pManualMapData = 0, pShellCode = 0;//������ڴ�,һ����ӳ��ṹ���Ե�ַ,һ����ShellCode��ַ
	status = PsLookupProcessByProcessId(process_id, &Process);
	if (!NT_SUCCESS(status)) {
		MYLOG("failed to get process", true);
		return status;
	}
	//���̹߳ҿ���Ŀ�����
	KeStackAttachProcess(Process, &Apc);

	while (TRUE) {
		//��dll�����ڴ���
		pDllMem = inst_callback_get_dll_memory(us_dll_path);
		if (!pDllMem) {
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		//��dll���ص��ڴ���
		status = inst_callback_alloc_memory(process_id, pDllMem, &InstCallBack, &pManualMapData);
		if (!NT_SUCCESS(status)) break;

		//����instrcallbackʹ��ָ��shellcode
		status = inst_callback_set_callback(InstCallBack);
		break;
	}

	//��bstartΪ��ʼ��1Ҳ���ǿ�ʼ�ֶ�����dll��ʱ��callback�Ϳ���ȥ����
	if (pManualMapData && MmIsAddressValid(pManualMapData)) {
		//�п������ʱ������˳��� ������Ҫ�쳣����һ��
		__try {
			while (1) {
				if (((Manual_Mapping_data*)pManualMapData)->bStart) break;
			}
		}
		__except (1) {

			MYLOG("process exit!", true);
			ObDereferenceObject(Process);
			KeUnstackDetachProcess(&Apc);
			return status;
		}
	}

	//ж��
	inst_callback_set_callback(0);                                                                     //�����˳�
	if (pManualMapData && MmIsAddressValid(pManualMapData) && PsLookupProcessByProcessId(process_id, &Process) != STATUS_PENDING) {
		__try {
			//Ĩ��PEͷ
			*(PUCHAR)((((Manual_Mapping_data*)pManualMapData))->pBase) = 0;
			//����ִ��ִ��dllmain������
			((Manual_Mapping_data*)pManualMapData)->bContinue = true;
		}
		__except (1) {

			MYLOG("process exit?", true);
		}
	}


	ObDereferenceObject(Process);
	KeUnstackDetachProcess(&Apc);
	//�ͷŴ��ļ������ڴ��е�dll
	if (pDllMem && MmIsAddressValid(pDllMem)) ExFreePool(pDllMem);
	return status;
}

NTSTATUS inst_callback_alloc_memory(HANDLE PID, PUCHAR p_dll_memory, _Out_ PVOID* inst_callbak_addr, _Out_ PVOID* p_manual_data) {
	PEPROCESS Process{ 0 };
	//�Ѿ��ҿ��˱����ظ��ҿ�
	IMAGE_NT_HEADERS* pNtHeader = nullptr;
	IMAGE_OPTIONAL_HEADER* pOptHeader = nullptr;
	IMAGE_FILE_HEADER* pFileHeader = nullptr;

	char* pStartMapAddr = nullptr;//R3���ַ ͨ��ZwAllocatevirtual Dll��PEͷ��ʼ�ĵ�ַ
	size_t AllocSize = 0, RetSize;
	size_t DllSize;
	Manual_Mapping_data ManualMapData{ 0 };
	PVOID pManualMapData = 0, pShellCode = 0;//������ڴ�,һ����ӳ��ṹ��ַ,һ����ShellCode��ַ 
	NTSTATUS status = STATUS_SUCCESS;

	if (reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_magic != 0x5A4D) {
		status = STATUS_INVALID_PARAMETER;
		Log("the dll is not an valid file structure", true, status);
		return status;
	}
	//��ȡNTͷ ͨ��NTͷ��ȡ�ļ�ͷ �� ѡ��ͷ
	pNtHeader = (IMAGE_NT_HEADERS*)((ULONG_PTR)p_dll_memory + reinterpret_cast<IMAGE_DOS_HEADER*>(p_dll_memory)->e_lfanew);
	pFileHeader = &pNtHeader->FileHeader;
	pOptHeader = &pNtHeader->OptionalHeader;

	//Machine ����ƽ̨����׼ȷ����˵Ӧ����CPU��ָ�������������ִ���ļ��������������͵�CPU�ϡ� 
	//IMAGE_FILE_MACHINE_I386��0x14C��Ҳ��ͨ�����ֶ����ж�IMAGE_NT_HEADER��ʹ��32λ�Ľṹ����64λ�Ľṹ
	if (pFileHeader->Machine != X64) {
		status = STATUS_NOT_SUPPORTED;
		Log("the dll is x86 structure,not support", true, status);
		return status;
	}
	//SizeOfImage ���������ڴ��е��ܴ�С����ֵ���ڴ����ı������ܴ�С�����нڣ�����ͷ��ӳ����ڴ���ܴ�С��
	AllocSize = pOptHeader->SizeOfImage;
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&pStartMapAddr, 0, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {

		Log("failed to alloc memory", true, status);
		return status;
	}
	DllSize = AllocSize;
	//���
	RtlSecureZeroMemory(pStartMapAddr, AllocSize);

	//��ʼ��ManualMapData ����ṹ���ͨ��shellcode��RCX�Ĵ��������ض�λshellcode
	//����ͨ���ж���Щ�ṹ���еı�־λ����ʱȡ��instcall�ص�������ж��PE�ṹ
	ManualMapData.dwReadson = 0;
	ManualMapData.pGetProcAddress = (f_GetProcAddress)g_fnGetProcAddress;
	ManualMapData.pLoadLibraryA = (f_LoadLibraryA)g_fnLoadLibrary;
	ManualMapData.pRtlAddFunctionTable = (f_RtlAddFunctionTable)g_fnAddFuntionTable;

	ManualMapData.pBase = pStartMapAddr;
	ManualMapData.bContinue = false; //�����־�����ж��Ƿ�Ĩ����ע��DLL��PE��־ Ĩ�����ٵ���dllmain
	ManualMapData.bFirst = true; //�Ƿ��ǵ�һ�μ��� ����ǵ�һ��������shellcode  ��ΪֻҪ��ϵͳ������������instrcall ����ֻ������һ��instrcall������
	ManualMapData.bStart = false;//�����־�����ж��Ƿ�ʼִ��shellcode�� ��ʼ������instrucall��û���� ��ʱȡ��instrcall �����еĺ�����instrcall��־���û����
	ManualMapData.DllSize = DllSize;

	//��ʼ�����ڴ�����������飨section��
	//����ֱ�ӿ���һ��ҳ�Ĵ�С PE�ṹӳ�䵽�ڴ���
	//���������� DOSͷ NTͷ ����� ���������ṹ����һ������
	Process = IoGetCurrentProcess();
	status = MmCopyVirtualMemory(Process, p_dll_memory, Process, pStartMapAddr, PAGE_SIZE, KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {
		Log("failed to write pe header!", true, status);
		return status;
	}

	//��ʼ���������
	IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader); //ȡ����һ�������ָ�� ����ʱ�����ļ��У�
	//����ÿ������ͷ �����������
	for (int i = 0; i < pFileHeader->NumberOfSections; i++, pSectionHeader++) {
		if (pSectionHeader->SizeOfRawData) {
			//��һ����������������д�뵽�����ַ,���Ѿ����� FA->RVAת��
			/*
			VirtualSize ���ڴ��еĴ�С���ڴ����ǰ����ֵ**
			VirtualAddress �ýڿ������ڴ��е�RVA
			SizeOfRawData �ý����ļ��еĴ�С���ļ�������ֵ��
			PointerToRawData �ý����ļ��е�ƫ��FA��file address��
			*/
			status = MmCopyVirtualMemory(Process, p_dll_memory + pSectionHeader->PointerToRawData, Process, pStartMapAddr + pSectionHeader->VirtualAddress, pSectionHeader->SizeOfRawData, KernelMode, &RetSize);
			if (!NT_SUCCESS(status)) {
				Log("failed to write sections", true, status);
				return status;
			}
		}
	}
	//��ʼӳ��ManualMapData
	AllocSize = PAGE_SIZE;
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), &pManualMapData, 0, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("failed to alloc mem for manualdata", true, status);
		return status;
	}
	RtlSecureZeroMemory(pManualMapData, AllocSize);

	status = MmCopyVirtualMemory(Process, &ManualMapData, Process, pManualMapData, sizeof(ManualMapData), KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {

		Log("failed to write mem for manualdata", true, status);
		return status;
	}
	//��ʼӳ�� shellcode
	status = ZwAllocateVirtualMemory(NtCurrentProcess(), &pShellCode, 0, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {

		Log("failed to alloc mem for shellcode", true, status);
		return status;
	}

	RtlSecureZeroMemory(pShellCode, AllocSize);

	//д��shellcode
	status = MmCopyVirtualMemory(Process, InstruShellCode, Process, pShellCode, AllocSize, KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {

		Log("failed to write mem for shellcode", true, status);
		return status;
	}

	shellcode_t shell_code;
	memset(&shell_code, 0, sizeof shell_code);
	//��shellcode�������ṹ����
	memcpy(&shell_code, &g_instcall_shellcode, sizeof shellcode_t);
	shell_code.manual_data = (UINT64)pManualMapData;//������� Ҫͨ��RCX����
	shell_code.rip = (UINT64)pShellCode; //PE������

	//����shellcode inst_callbak_addr�������� instrcallback Ҫָ��shellcode
	ZwAllocateVirtualMemory(NtCurrentProcess(), inst_callbak_addr, 0, &AllocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(status)) {
		Log("failed to alloc mem for instcall shellcode", true, status);
		return status;
	}
	RtlSecureZeroMemory(*inst_callbak_addr, AllocSize);

	//д��InstCallBack ShellCode
	MmCopyVirtualMemory(Process, &shell_code, Process, *inst_callbak_addr, sizeof shell_code, KernelMode, &RetSize);
	if (!NT_SUCCESS(status)) {
		Log("failed to write mem for instcall shellcode", true, status);
		return status;
	}


	*p_manual_data = pManualMapData; //��������Ϣ�ṹ�崫��ȥ

	for (size_t index = 0; index < DllSize; index += PAGE_SIZE)

	hide_mem(PID, (void*)((UINT64)pStartMapAddr + index), MM_NOACCESS);  //�����ڴ����� ע��PUBG

	hide_mem(PID, pManualMapData, MM_NOACCESS);

	hide_mem(PID, pShellCode, MM_NOACCESS);

	hide_mem(PID, (void*)inst_callbak_addr, MM_NOACCESS);
	return status;
}

PUCHAR inst_callback_get_dll_memory(UNICODE_STRING* us_dll_path) {
	HANDLE hFile = 0;
	OBJECT_ATTRIBUTES objattr;
	IO_STATUS_BLOCK IoStatusBlock = { 0 };
	LARGE_INTEGER		lainter = { 0 };
	NTSTATUS status;
	FILE_STANDARD_INFORMATION	fileinfo = { 0 };
	ULONG FileSize = 0; //dll�ļ���С
	PUCHAR pDllMemory = 0; //dll�ļ����ص��ڴ��еĵ�ַ
	LARGE_INTEGER byteoffset = { 0 };//��ȡ�ļ���ƫ�ƿ�ʼ
	//���ö�������
	InitializeObjectAttributes(&objattr, us_dll_path, OBJ_CASE_INSENSITIVE, 0, 0);
	/*
	FileHandle�����ڽ��մ�����򿪵��ļ�����ľ����ָ�롣
DesiredAccess����ʾ���ļ�ʱ����ķ���Ȩ�ޣ��� ACCESS_MASK ���ͱ�ʾ�����磬����ָ�� FILE_READ_DATA��FILE_WRITE_DATA ��Ȩ�ޡ�
ObjectAttributes��һ��ָ�� OBJECT_ATTRIBUTES �ṹ��ָ�룬�ýṹ�����й��ļ��������Ϣ�����ļ������������ƿռ�ȡ�
IoStatusBlock��һ��ָ�� IO_STATUS_BLOCK �ṹ��ָ�룬���ڽ����й� I/O �����������Ϣ���緵��״̬�ʹ�����ֽ�����
AllocationSize��һ����ѡ��ָ�� LARGE_INTEGER �ṹ��ָ�룬��ʾ�ļ��ĳ�ʼ�����С��
FileAttributes����ʾ�ļ����Եı�־������ ARCHIVE��HIDDEN��NORMAL �ȡ�
ShareAccess����ʾ�ļ�����ļ����� FILE_SHARE_READ��FILE_SHARE_WRITE��
CreateDisposition����ʾ�ļ��Ĵ�����򿪷�ʽ���� FILE_CREATE��FILE_OPEN_IF �ȡ�
CreateOptions����ʾ�ļ�������򿪵�����ѡ��� FILE_NON_DIRECTORY_FILE��FILE_SYNCHRONOUS_IO_NONALERT �ȡ�
EaBuffer��һ����ѡ��ָ����չ���ԣ�Extended Attributes����������ָ�롣
EaLength��ָ����չ���Ի������ĳ��ȡ�
	*/
	status = ZwCreateFile(&hFile, GENERIC_READ, &objattr, &IoStatusBlock, &lainter, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, FILE_OPEN, 0, 0, 0);
	if (!NT_SUCCESS(status)) {

		Log("failed to create file", true, status);
		return 0;
	}
	//��ȡ�ļ���С
	status = ZwQueryInformationFile(hFile, &IoStatusBlock, &fileinfo, sizeof(fileinfo), FileStandardInformation);
	FileSize = (ULONG)fileinfo.AllocationSize.QuadPart;
	if (!NT_SUCCESS(status)) {
		Log("failed to get file size", true, status);
		return 0;
	}
	//�ڴ����
	FileSize += 0x1000; //��0x1000�ڶ��� ��ö���󻹱�С��
	FileSize = (UINT64)PAGE_ALIGN(FileSize);
	//�����ڴ�ռ�
	pDllMemory = (PUCHAR)ExAllocatePoolWithTag(PagedPool, FileSize, 'Dllp'); //�ļ�����ȥ
	RtlSecureZeroMemory(pDllMemory, FileSize);


	status = ZwReadFile(hFile, 0, 0, 0, &IoStatusBlock, pDllMemory, FileSize, &byteoffset, 0);
	//ˢ��һ�� Ҫ��Ȼ�Ῠ��
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
