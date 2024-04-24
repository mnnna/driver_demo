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
				pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (reinterpret_cast<char*> (pRelocaData) + pRelocaData->SizeOfBlock);
			} 
		}

	}
	//实现手动加载 DLL
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {   //判断是否有 IAT 表
		
	} //
}
