#include<ntifs.h>
#include<ntddk.h>


void DriverUnload(PDRIVER_OBJECT Driver_Object){
	Driver_Object;
	DbgPrint("ж�سɹ�  !\n");
}

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT Driver_Object, PUNICODE_STRING RegisterPath) {
	RegisterPath;
	Driver_Object->DriverUnload = DriverUnload;
	DbgPrint("���سɹ�  !\n");
	return STATUS_SUCCESS;
}



