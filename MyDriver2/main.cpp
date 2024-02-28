#include<ntifs.h>
#include<ntddk.h>
#include"HookManager.h"

void DriverUnload(PDRIVER_OBJECT DriverObject) {


}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	RegisterPath;

	HookManager::GetInstance()->InstallInlinehook( );
	return STATUS_SUCCESS; 
}