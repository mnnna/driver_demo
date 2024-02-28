#include<ntifs.h>
#include<ntddk.h>
#include"HookManager.h"

typedef NTSTATUS(NTAPI *pfnNtOpenProcess)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);

pfnNtOpenProcess g_oriNtOpenProcess;

NTSTATUS NTAPI FakeNtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId) {
    DbgPrintEx(102,0, "Fake NtOpenProcess");
   
    return g_oriNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
};

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    HookManager::GetInstance()->RemoveInlinehook((void*)FakeNtOpenProcess);

}
 
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	RegisterPath;
    g_oriNtOpenProcess = NtOpenProcess;
	HookManager::GetInstance()->InstallInlinehook((void**)g_oriNtOpenProcess,(void *)FakeNtOpenProcess);
	return STATUS_SUCCESS; 
}