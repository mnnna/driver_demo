#include "ShellCode.h"

void __stdcall InstruShellCode(Manual_Mapping_data* pData)
{
	if (!pData->bFirst) return;
	pData->bFirst = false;
	pData->bStart = true;

	char* pBase = pData->pBase;
	auto pOptionHeader = &reinterpret_cast<IMAGE_NT_HEADERS*>((ULONG_PTR)pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pBase)->e_lfanew)->OptionalHeader;

	char* LocationDelta = pBase - pOptionHeader->ImageBase; //计算差值

	if (LocationDelta) {
		if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) { // 重定位
			auto*  pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (pBase + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress); // 重定位表RVA

			auto* pReloadEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pRelocaData + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			
			while (pRelocaData < pReloadEnd && pRelocaData->SizeOfBlock) {
				UINT64 AmountOfEntries = ( pRelocaData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/ sizeof(short);  // ?????

				unsigned short*  pRelativeInfo = reinterpret_cast<unsigned short*> (pRelocaData + 1);
				for (UINT64 i = 0; i != AmountOfEntries; i++) {
					//计算需要重定位的地址 
					auto pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocaData->VirtualAddress + (*(pRelativeInfo) & 0xFFF));
					//手动重定位
					*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);

				}
				pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (reinterpret_cast<char*> (pRelocaData) + pRelocaData->SizeOfBlock); //遍历条目
			} 
		}

	}
	//实现手动加载所用到的其他DLL
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {   //判断是否有 IAT 表
		IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);// 指向导入表描述符
		
		while (pImportDescr->Name) { // 遍历导入表描述符 所引用的dll ， 重定位函数的地址
			HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name); // pImportDescr->Name 是指向字符串的RVA地址， 需要加 pbase， 返回值是一个 模块; 这一步实现加载导入表 中的 dll

			// 修复 dLL 里面到处的函数地址
			// INT 导入名称表
			// IAT 导入地址表  

			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk); //INT
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk); //IAT

			if (!pInt) pInt = pIat; //双桥结构

			for (; *pIat; ++pIat, ++pInt) {
				if (IMAGE_SNAP_BY_ORDINAL(*pInt)) {  //判断是否是序号导出
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, (char*)(*pInt & 0xffff)); //取低两个字节？？？？
				}
				else {
					IMAGE_IMPORT_BY_NAME*  pImport = (IMAGE_IMPORT_BY_NAME*)(pBase + *pInt);
					(ULONG_PTR)pData->pGetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++;

		}
	} //

	// 手动 调佣 tls 
#define DLL_PROCESS_ATTACH 1
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) { // 定位 tls 回调表: image_Data_directory_tlsDriectrory
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallBack = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

		for (; pCallBack && *pCallBack; ++pCallBack) {
			(*pCallBack)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}

	}

	//修复异常表
	auto excep = pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	if (excep.Size) {
		pData->pRtlAddFunctionTable((_IMAGE_RUNTIME_FUNCTION_ENTRY*)(pBase + excep.VirtualAddress), excep.Size / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY),(DWORD64) pBase);
	}

	while (!pData->bContinue);
	//手动调佣  dll main
	((f_DLL_ENTRY_POINT)(pOptionHeader->AddressOfEntryPoint))(pBase, DLL_PROCESS_ATTACH, 0);
}
