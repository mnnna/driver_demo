#include "ShellCode.h"

void __stdcall InstruShellCode(Manual_Mapping_data* pData)
{
	if (!pData->bFirst) return;
	pData->bFirst = false;
	pData->bStart = true;

	char* pBase = pData->pBase;
	auto pOptionHeader = &reinterpret_cast<IMAGE_NT_HEADERS*>((ULONG_PTR)pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pBase)->e_lfanew)->OptionalHeader;

	char* LocationDelta = pBase - pOptionHeader->ImageBase; //�����ֵ

	if (LocationDelta) {
		if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) { // �ض�λ
			auto*  pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (pBase + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress); // �ض�λ��RVA

			auto* pReloadEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pRelocaData + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			
			while (pRelocaData < pReloadEnd && pRelocaData->SizeOfBlock) {
				UINT64 AmountOfEntries = ( pRelocaData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/ sizeof(short);  // ?????

				unsigned short*  pRelativeInfo = reinterpret_cast<unsigned short*> (pRelocaData + 1);
				for (UINT64 i = 0; i != AmountOfEntries; i++) {
					//������Ҫ�ض�λ�ĵ�ַ 
					auto pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocaData->VirtualAddress + (*(pRelativeInfo) & 0xFFF));
					//�ֶ��ض�λ
					*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);

				}
				pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (reinterpret_cast<char*> (pRelocaData) + pRelocaData->SizeOfBlock); //������Ŀ
			} 
		}

	}
	//ʵ���ֶ��������õ�������DLL
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {   //�ж��Ƿ��� IAT ��
		IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);// ָ�����������
		
		while (pImportDescr->Name) { // ��������������� �����õ�dll �� �ض�λ�����ĵ�ַ
			HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name); // pImportDescr->Name ��ָ���ַ�����RVA��ַ�� ��Ҫ�� pbase�� ����ֵ��һ�� ģ��; ��һ��ʵ�ּ��ص���� �е� dll

			// �޸� dLL ���浽���ĺ�����ַ
			// INT �������Ʊ�
			// IAT �����ַ��  

			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk); //INT
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk); //IAT

			if (!pInt) pInt = pIat; //˫�Žṹ

			for (; *pIat; ++pIat, ++pInt) {
				if (IMAGE_SNAP_BY_ORDINAL(*pInt)) {  //�ж��Ƿ�����ŵ���
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, (char*)(*pInt & 0xffff)); //ȡ�������ֽڣ�������
				}
				else {
					IMAGE_IMPORT_BY_NAME*  pImport = (IMAGE_IMPORT_BY_NAME*)(pBase + *pInt);
					(ULONG_PTR)pData->pGetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++;

		}
	} //

	// �ֶ� ��Ӷ tls 
#define DLL_PROCESS_ATTACH 1
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) { // ��λ tls �ص���: image_Data_directory_tlsDriectrory
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallBack = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

		for (; pCallBack && *pCallBack; ++pCallBack) {
			(*pCallBack)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}

	}

	//�޸��쳣��
	auto excep = pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	if (excep.Size) {
		pData->pRtlAddFunctionTable((_IMAGE_RUNTIME_FUNCTION_ENTRY*)(pBase + excep.VirtualAddress), excep.Size / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY),(DWORD64) pBase);
	}

	while (!pData->bContinue);
	//�ֶ���Ӷ  dll main
	((f_DLL_ENTRY_POINT)(pOptionHeader->AddressOfEntryPoint))(pBase, DLL_PROCESS_ATTACH, 0);
}
