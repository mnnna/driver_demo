#include "HideMemory.h"
 
struct _MMPTE
{
	ULONGLONG Valid : 1;                                                      //0x0
	ULONGLONG PageFileReserved : 1;                                           //0x0
	ULONGLONG PageFileAllocated : 1;                                          //0x0
	ULONGLONG ColdPage : 1;                                                   //0x0
	ULONGLONG SwizzleBit : 1;                                                 //0x0
	ULONGLONG Protection : 5;                                                 //0x0
	ULONGLONG Prototype : 1;                                                  //0x0
	ULONGLONG Transition : 1;                                                 //0x0
	ULONGLONG PageFileLow : 4;                                                //0x0
	ULONGLONG UsedPageTableEntries : 10;                                      //0x0
	ULONGLONG ShadowStack : 1;                                                //0x0
	ULONGLONG Unused : 5;                                                     //0x0
	ULONGLONG PageFileHigh : 32;                                              //0x0
};
//0x30 bytes (sizeof)
struct _MMPFN
{
	void* padding1;
	void* pte_address;
	struct _MMPTE OriginalPte;                                      //0x10
	char padding2[0x18];															//0x28
};

_MMPFN get_MemPfnDataBase() {
	
}

bool hide_mem(HANDLE pid, void* va, ULONG attribute)
{


	return false;
}
