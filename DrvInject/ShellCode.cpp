#include "ShellCode.h"
//ʹ���Զ�λ,�ҵ�pData ����pData��ͨ��RCX��������

void __stdcall InstruShellCode(Manual_Mapping_data* pData)
{
	if (!pData->bFirst) return;

	//�ɹ� �������� ��ֹ����
	pData->bFirst = false;
	//�Ѿ�����shellcode�˿���ж��instrcall��
	pData->bStart = true;
	//�õ��Ѿ����ص��ڴ��е�PE�ṹ��ʼ��ַ
	char* pBase = pData->pBase;
	//�õ�ѡ��ͷ
	
	auto* pOptionHeadr = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pBase)->e_lfanew)->OptionalHeader;
	//ImageBase �������ַ������˵��ģ�����ַ���ڿ��������ַ������£���ֵ��Ȼ���е�����Ч�ˡ�*
		//ImageBase �������ַ������˵��ģ�����ַ���ڿ��������ַ������£���ֵ��Ȼ���е�����Ч�ˡ�*

	char* LocationDelta = pBase - pOptionHeadr->ImageBase; //�������ֵ ��ʱ����Ҫ����Ҫ�ض�λ�ĵ�ַ���������ֵ�����ֶ��ض�λ

	if (LocationDelta) {
		//�����IMAGE_DIRECTORY_ENTRY_BASERELOC�������ض�λ�� ��0��ʼ��
		if (pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
			auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			//�����ض�λ��Ľ�����ַ
			auto* pRelocEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			//��ʼ�����ض�λ��
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock) {
				//������ض�λ���е�TypeOffset����ĸ��� Ҳ���ǵ�ǰ�ض�λ�����еı����RVA�ĸ���
				//�ض�λ�ĸ���������IMAGE_BASE_RELOCATION����ط�
				//�ض�λ��ƫ�ƵĴ�С��WORD
				UINT64 AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(short);//typeoffset�� RVA ռ��12λ

				
				unsigned short* pRelativeInfo = reinterpret_cast<unsigned short*>(pRelocData + 1);
				for (UINT64 i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo) {
					//������Ҫ�ض�λ�ĵ�ַ 
					if (RELOC_FLAG(*pRelativeInfo)) {//typeoffset�ĸ�4λ�� �ض�λ���� �����x64λֱ��Ѱַ���ǽ����ض�λ
						//ֻ��ֱ��Ѱַ����Ҫ�ض�λ
						//pBase+RVA==��Ҫ�ض�λҳ��
						//ҳ��+0xfff & TypeOffset ����Ҫ�ض�λ�ĵ�ַ(һ��ֱ�ӵ�ַ)
						//�������Ҫ�ض�λ�ĵ�ַ
						UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						//�ֶ��ض�λ
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}
					

				}
				//��һ���ض�λ��(�Ͼ���ֹһ��ҳ����Ҫ�ض�λ)
				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<char*>(pRelocData) + pRelocData->SizeOfBlock); //������Ŀ
			} 
		}

	}
	//ʵ���ֶ��������õ�������DLL
	 if (pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {  //�ж��Ƿ��� IAT ��
		//�õ������������ һ��dll��Ӧһ������������� һ�����������������������dll�������ĺ���
		 IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);// ָ�����������
		
		 while (pImportDescr->Name) { // ��������������� �����õ�dll �� �ض�λ�����ĵ�ַ
			 HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name); // pImportDescr->Name ��ָ���ַ�����RVA��ַ�� ��Ҫ�� pbase�� ����ֵ��һ�� ģ��; ��һ��ʵ�ּ��ص���� �е� dll

			// �޸� dLL ���浽���ĺ�����ַ
			// INT �������Ʊ�
			// IAT �����ַ��  

			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk); //INT
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk); //IAT
			//���������int���ǰ�iat���int ��Ϊ����������޸�int�� ֱ�Ӹ�int���б���ĵ�ַ ULONGLONG Function;
			if (!pInt) pInt = pIat; //˫�Žṹ������û�е��� INT ������£�ʹ�� IAT ����Ϊ������Դ���������ͨ��������һЩ�������Ż�����������£�û�ж����� INT�����ǽ� INT �� IAT �ϲ�Ϊһ����

			for (; *pIat; ++pIat, ++pInt) {
				if (IMAGE_SNAP_BY_ORDINAL(*pInt)) {  //�ж��Ƿ�����ŵ���
					//�������ŵ��� �������ֽڱ�����Ǻ������
					//�޸� ULONGLONG Function;
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, (char*)(*pInt & 0xffff)); //ȡ�������ֽ�
				}
				else {
					IMAGE_IMPORT_BY_NAME* pImport = (IMAGE_IMPORT_BY_NAME*)(pBase + *pInt);
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++;

		}
	} //


//�ֶ�����TLS�ص�����
//������dll��û����TLS���� ������֤��Ҫע���dll�����������dll��������û����TLS
//���˾�Ҫ�ֶ�����
//ע��һ��Ҫ���ֶ��ض�λ �ٵ���TLS�ص����� ��ΪTLS�ص����� �б������VA��ַ
//��Ҫ��ͨ���ض�λ������ض�λ
#define DLL_PROCESS_ATTACH 1
	 if (pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) {
		 auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		 //ע�� ����Ҫ�����ض�λ
		 //TLS���CallBackҪ��LocationDelta
		 auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks); //�õ�TLS�ص�������
		 //����TLS�ص�������
		 for (; pCallback && *pCallback; ++pCallback) {
			 //�ֶ�����
			 (*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		 }
	 }

	//�޸��쳣��
	 auto excep = pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	 if (excep.Size) {
		 pData->pRtlAddFunctionTable((_IMAGE_RUNTIME_FUNCTION_ENTRY*)(pBase + excep.VirtualAddress), excep.Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY), (DWORD64)pBase);

	 }

	 //�ȴ�Ĩ��PEͷ��ж��instrcallback
	 while (!pData->bContinue);
	 //ִ��DllMain����
	 ((f_DLL_ENTRY_POINT)(pBase + pOptionHeadr->AddressOfEntryPoint))(pBase, DLL_PROCESS_ATTACH, 0);
}
