#pragma once
#include"structer.h"
#include"MDL.h"
#include"ia32/ia32.hpp"

class HookManager
{
	// µ¥ÀýÄ£Ê½
public: 
	bool InstallInlinehook(HANDLE pid, __inout void** originAddr, void* hookAddr );
	bool RemoveInlinehook(HANDLE pid, void* hookAddr);
	static HookManager* GetInstance();

private: 
	bool IsolationPageTable(PEPROCESS process, void* isolateioAddr);

	UINT32 mHookCount = 0; 

	HOOK_INFO mHookInfo[MAX_HOOK_COUNT] = { 0 };

	char* mTrampLinePool = 0;
	UINT32 mPoolUSED = 0;

	static HookManager* mInstance;
};


struct PAGE_TABLE
{
	struct
	{
		pte_64* Pte;
		pde_64* Pde;
		pdpte_64* Pdpte;
		pml4e_64* Pml4e;
	}Entry;
	ULONG64 VirtualAddress;
};


