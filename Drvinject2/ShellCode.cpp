#include"ShellCode.h"
//ʹ���Զ�λ,�ҵ�pData ����pData��ͨ��RCX��������
void __stdcall InstruShellCode(Manual_Mapping_data* pData) {



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

	char* LocationDelta = pBase - pOptionHeadr->ImageBase; //�������ֵ ��ʱ����Ҫ����Ҫ�ض�λ�ĵ�ַ���������ֵ�����ֶ��ض�λ

	//��ʼģ���ض�λ������ֶ��ض�λ
	if (LocationDelta) {
		//����������ض�λ�� ��0��ʼ��
		if (pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
			//VirtualAddress �ýڿ������ڴ��е�RVA
			//��λ���ض�λ������
			auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			//�����ض�λ��Ľ�����ַ
			auto* pRelocEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);

			//��ʼ�����ض�λ��
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock) {
				//������ض�λ���е�TypeOffset����ĸ��� Ҳ���ǵ�ǰ�ض�λ�����еı����RVA�ĸ���
				//�ض�λ�ĸ���������IMAGE_BASE_RELOCATION����ط�
				//�ض�λ��ƫ�ƵĴ�С��WORD
				UINT64 AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(short);//typeoffset�� RVA ռ��12λ
				//ָ���ض�λ��ƫ��
				//typedef struct _IMAGE_BASE_RELOCATION {
				//	DWORD   VirtualAddress; //�ض�λ����ʼ��ַ��RVA
				//	DWORD   SizeOfBlock;
				//	//  WORD    TypeOffset[1];
				//Windows�ض�λ���ǰ�ҳ�漰��
				//����ĵ�ַ,����������һ��RVA����.
				//TypeOffset�и�4λ������ض����������
				//��12λ ��ʾ�����һҳ(4KB)��ƫ��

				//�õ���һ��typeoffset�е�RVA
				unsigned short* pRelativeInfo = reinterpret_cast<unsigned short*>(pRelocData + 1);
				//++pRelativeInfo �õ����¸�typeoffset�е�RVA
				for (UINT64 i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo) {
					if (RELOC_FLAG(*pRelativeInfo)) { //typeoffset�ĸ�4λ�� �ض�λ���� �����x64λֱ��Ѱַ���ǽ����ض�λ
						//ֻ��ֱ��Ѱַ����Ҫ�ض�λ
						//pBase+RVA==��Ҫ�ض�λҳ��
						//ҳ��+0xfff & TypeOffset ����Ҫ�ض�λ�ĵ�ַ(һ��ֱ�ӵ�ַ)
						//�������Ҫ�ض�λ�ĵ�ַ
						UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						//�����ֶ��ض�λ
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}
				}
				//��һ���ض�λ��(�Ͼ���ֹһ��ҳ����Ҫ�ض�λ)
				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<char*>(pRelocData) + pRelocData->SizeOfBlock);
			}

		}
	}
	/*
	IMAGE_DATA_DIRECTORY STRUCT
	DWORD   VirtualAddress; //�����RVA ���ﲻдFA��ԭ������Ϊ����ϵͳ�����ڴ�����д��ֵ��Ҫ��dll���ص��ڴ��в����
	DWORD   Size;           //����Ĵ�С �����ο���ϵͳû�������С��ϵͳ�ǿ��ñ�ṹ���жϴ�С��

	_IMAGE_IMPORT_DESCRIPTOR
		IMAGE_THUNK_DATA �������Ʊ�Import Name Table��  INT ������
			IMAGE_IMPOART_BY_NAME
		IMAGE_THUNK_DATA �����ַ��Import Address Table��
			IATIMAGE_IMPOART_BY_NAME
	IMAGE_THUNK_DATA64 {
	union {
		ULONGLONG ForwarderString;  // PBYTE
		ULONGLONG Function;         // ��ǰdllģ�鵼��ĺ�����ַ��RVA
		ULONGLONG Ordinal;          //Ordinal �������API�����ֵ�������λΪ1��ʱ�򣬸�λ���ã���Чֵֻ�������ֽڣ�Ҳ���ǵ���λ��MFCdll������ŵ���
		ULONGLONG AddressOfData;    // PIMAGE_IMPORT_BY_NAME
	} u1;

	*/
	//�޸�IAT���еĺ�����ַ �����ֶ����ض�Ӧdll
	if (pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
		//�õ������������ һ��dll��Ӧһ������������� һ�����������������������dll�������ĺ���
		IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		//ָ��ģ�������ַ�����RVA��ַ
		while (pImportDescr->Name) {
			//�ֶ�����dll
			HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name);//Ӧ�ò��inline hookȷʵ���Ա������� ������������صĶ������Լ�������dll
			//��������˼ά�����������������Լ���Ҫ�õ�dll Ҫ��Ȼ���Լ�Ҳ�ܲ�����
//�õ�INT���IAT��
//INT
			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk);
			//IAT MFC��dllһ���ǿ����Ƶ�����
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk);
			//���������int���ǰ�iat���int ��Ϊ����������޸�int�� ֱ�Ӹ�int���б���ĵ�ַ ULONGLONG Function;
			if (!pInt) pInt = pIat;
			//Ordinal �������API�����ֵ�������λΪ1��ʱ�򣬸�λ���ã���Чֵֻ�������ֽڣ�Ҳ���ǵ���λ��MFCdll������ŵ���
			for (; *pIat; ++pIat, ++pInt) {
				//�������ŵ������λΪ1
				if (IMAGE_SNAP_BY_ORDINAL(*pInt)) {
					//�������ŵ��� �������ֽڱ�����Ǻ������
					//�޸� ULONGLONG Function;
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, (char*)(*pInt & 0xffff));

				}
				else
				{
					//��������Ƶ��� 
					//���������õ�IMAGE_IMPORT_BY_NAME����ĺ�������
					//��Ϊ���������ŵ��� IMAGE_IMPORT_BY_NAME ��ָ��ĺ��������ǿյ�
					IMAGE_IMPORT_BY_NAME* pImport = (IMAGE_IMPORT_BY_NAME*)(pBase + *pInt);
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++; //�õ���һ��dll�ĵ����������
		}
	}

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


	//�޸�x64���쳣��
	auto excep = pOptionHeadr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	if (excep.Size) {
		pData->pRtlAddFunctionTable((_IMAGE_RUNTIME_FUNCTION_ENTRY*)(pBase + excep.VirtualAddress), excep.Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY), (DWORD64)pBase);

	}

	//�ȴ�Ĩ��PEͷ��ж��instrcallback
	while (!pData->bContinue);
	//ִ��DllMain����
	((f_DLL_ENTRY_POINT)(pBase + pOptionHeadr->AddressOfEntryPoint))(pBase, DLL_PROCESS_ATTACH, 0);


}