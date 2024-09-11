#include "ShellCode.h"
//使用自定位,找到pData 这里pData是通过RCX传进来的

void __stdcall InstruShellCode(Manual_Mapping_data* pData)
{
	if (!pData->bFirst) return;
	pData->bFirst = false;
	pData->bStart = true;

	char* pBase = pData->pBase;
	//拿到选项头
	auto* pOptionHeader = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pBase)->e_lfanew)->OptionalHeader;// 
	//ImageBase 镜像基地址，或者说主模块基地址，在开启随机基址的情况下，该值仍然会有但是无效了。*
	char* LocationDelta = pBase - pOptionHeader->ImageBase; //计算差值 到时候需要将需要重定位的地址加上这个差值进行手动重定位

	if (LocationDelta) {
		//第五项（IMAGE_DIRECTORY_ENTRY_BASERELOC）就是重定位表 从0开始数
		if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) { // 重定位
			auto*  pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (pBase + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress); // 重定位表RVA
			//计算重定位表的截至地址
			auto* pReloadEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocaData) + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
			//开始遍历重定位表
			while (pRelocaData < pReloadEnd && pRelocaData->SizeOfBlock) {
				//计算出重定位表中的TypeOffset数组的个数 也就是当前重定位表项中的保存的RVA的个数
				//重定位的个数不包括IMAGE_BASE_RELOCATION这个地方
				//重定位的偏移的大小是WORD
				UINT64 AmountOfEntries = ( pRelocaData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/ sizeof(short);  // typeoffset中 RVA 占低12位

				
				unsigned short*  pRelativeInfo = reinterpret_cast<unsigned short*> (pRelocaData + 1); // IMAGE_BASE_RELOCATION 指针+1
				for (UINT64 i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo ) {
					//计算需要重定位的地址 
					if (RELOC_FLAG(*pRelativeInfo)) { //typeoffset的高4位是 重定位类型 如果是x64位直接寻址我们进行重定位
						//只有直接寻址才需要重定位
						//pBase+RVA==需要重定位页面
						//页面+0xfff & TypeOffset 就是要重定位的地址(一个直接地址)
						//计算出需要重定位的地址
						UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocaData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						//手动重定位
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}
					

				}
				//下一个重定位表(毕竟不止一个页面需要重定位)
				pRelocaData = reinterpret_cast<IMAGE_BASE_RELOCATION*> (reinterpret_cast<char*> (pRelocaData) + pRelocaData->SizeOfBlock); //遍历条目
			} 
		}

	}
	//实现手动加载所用到的其他DLL
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {   //判断是否有 IAT 表
		//拿到导入表描述符 一个dll对应一个导入表描述符 一个调入表描述符里面包含这个dll所导出的函数
		IMAGE_IMPORT_DESCRIPTOR* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);// 指向导入表描述符
		
		while (pImportDescr->Name) { // 遍历导入表描述符 所引用的dll ， 重定位函数的地址
			HMODULE hDll = pData->pLoadLibraryA(pBase + pImportDescr->Name); // pImportDescr->Name 是指向字符串的RVA地址， 需要加 pbase， 返回值是一个 模块; 这一步实现加载导入表 中的 dll

			// 修复 dLL 里面到处的函数地址
			// INT 导入名称表
			// IAT 导入地址表  

			ULONG_PTR* pInt = (ULONG_PTR*)(pBase + pImportDescr->OriginalFirstThunk); //INT
			ULONG_PTR* pIat = (ULONG_PTR*)(pBase + pImportDescr->FirstThunk); //IAT
			//如果不存在int我们吧iat表给int 因为我们最后都是修改int表 直接改int表中保存的地址 ULONGLONG Function;
			if (!pInt) pInt = pIat; //双桥结构允许在没有单独 INT 的情况下，使用 IAT 来作为导入表的源，这种情况通常发生在一些编译器优化或特殊情况下，没有独立的 INT，而是将 INT 和 IAT 合并为一个表。

			for (; *pIat; ++pIat, ++pInt) {
				if (IMAGE_SNAP_BY_ORDINAL(*pInt)) {  //判断是否是序号导出
					//如果是序号导出 低两个字节保存的是函数序号
					//修改 ULONGLONG Function;
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, (char*)(*pInt & 0xffff)); //取低两个字节
				}
				else {
					IMAGE_IMPORT_BY_NAME*  pImport = (IMAGE_IMPORT_BY_NAME*)(pBase + *pInt);
					*pIat = (ULONG_PTR)pData->pGetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++;

		}
	} //


//手动调用TLS回调函数
//就算你dll中没有用TLS机制 但不保证你要注入的dll包含其他库的dll里面里有没有用TLS
//用了就要手动调用
//注意一定要先手动重定位 再调用TLS回调函数 因为TLS回调函数 中保存的是VA地址
//需要先通过重定位表进行重定位
#define DLL_PROCESS_ATTACH 1
	if (pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) { // 定位 tls 回调表: image_Data_directory_tlsDriectrory
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase +  pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);

		auto* pCallBack = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

		for (; pCallBack && *pCallBack; ++pCallBack) {
			(*pCallBack)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}

	}

	//修复异常表
	auto excep = pOptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	if (excep.Size) {  
		pData->pRtlAddFunctionTable((_IMAGE_RUNTIME_FUNCTION_ENTRY*)(pBase + excep.VirtualAddress), excep.Size / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY),(DWORD64)pBase);
	}

	while (!pData->bContinue);
	//手动调佣  dll main
	((f_DLL_ENTRY_POINT)(pBase + pOptionHeader->AddressOfEntryPoint))(pBase, DLL_PROCESS_ATTACH, 0);
}
