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
			HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name); // pImportDescr->Name ��ָ���ַ�������Ե�ַ�� ��Ҫ�� pbase�� ����ֵ��һ�� ģ��; ��һ��ʵ�ּ��ص���� �е� dll

			// �޸� dLL ���浽���ĺ�����ַ
			// INT �������Ʊ�
			// IAT �����ַ��  

			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk); //INT
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk); //IAT

			if (!pInt) pInt = pIat; //˫�Žṹ

			for (; *pIat; ++pIat, ++pInt) {}

		}
	} //
}
