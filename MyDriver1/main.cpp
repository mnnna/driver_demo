#include<ntifs.h>
#include<ntddk.h>

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	DriverObject;
}

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PLSA_UNICODE_STRING RegistryPath) {
	DbgPrint("DriverObject:%p\n", DriverObject);
	DbgPrint("RegistryPath :%wz\n", RegistryPath);
	DbgBreakPoint();
	DriverObject->DriverUnload = DriverUnload;
	return  STATUS_SUCCESS;
} 