#include "ShellCode.h"

void __stdcall InstruShellCode(Manual_Mapping_data* pData)
{
	if (!pData->bFirst) return;
	pData->bFirst = false;
	pData->bStart = true;

	char* pBase = pData->pBase;
	auto pOptionHeader = &reinterpret_cast<IMAGE_NT_HEADERS*>((ULONG_PTR)pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pBase)->e_lfanew)->OptionalHeader;

}
