#include<ntifs.h>
#include<ntddk.h>
#include<intrin.h>
#include"ia32/ia32.hpp" 
#include"struct.h"

#pragma warning(disable:4389)
void* GetPteBase() {
	cr3 CR3;
	PHYSICAL_ADDRESS cr3_pa = { 0 }; 
	CR3.flags =  __readcr3();
	cr3_pa.QuadPart = CR3.address_of_page_directory * PAGE_SIZE; 
	PULONG64 cr3_va  = (PULONG64) MmGetVirtualForPhysical(cr3_pa);

	UINT64 nCount = 0;
	while ((*cr3_va & 0x000FFFFFFFFFF000) != cr3_pa.QuadPart) {
		if (++nCount >= 512) {
			return nullptr;
		}
		cr3_va++;
	} 
	return (void*)(0xffff000000000000 | (nCount << 39));
}

//set https_proxy=http://192.168.0.117:8889
//git config --global http.proxy http ://192.168.0.112:8889

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	DriverObject;
}

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PLSA_UNICODE_STRING RegistryPath) {
	//DbgPrint("DriverObject:%p\n", DriverObject);
	//DbgPrint("RegistryPath :%wz\n", RegistryPath);
	//DbgBreakPoint();
	PVOID64 PteBase = GetPteBase();
	DbgPrint("PteBase :%p\n", PteBase);
	DriverObject->DriverUnload = DriverUnload;
	return  STATUS_SUCCESS;
} 