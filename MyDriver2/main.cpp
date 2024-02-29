#include<ntifs.h>
#include<ntddk.h>
#include"HookManager.h"

typedef NTSTATUS(NTAPI *pfnNtOpenProcess)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);
typedef NTSTATUS(NTAPI *pfnNtCreateFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

pfnNtOpenProcess g_oriNtOpenProcess;
pfnNtCreateFile g_oriNtCreateFile;

NTSTATUS NTAPI FakeNtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId) {
    DbgPrintEx(102,0, "Fake NtOpenProcess");
    
    return g_oriNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
};

NTSTATUS NTAPI FakeNtCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength) {
    
    DbgPrint("Fake Ntfakeopenfile"); 

    return g_oriNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

};



void DriverUnload(PDRIVER_OBJECT DriverObject) {
    HookManager::GetInstance()->RemoveInlinehook((void*)FakeNtOpenProcess);
    HookManager::GetInstance()->RemoveInlinehook((void*)FakeNtCreateFile);

}
 
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	RegisterPath;
    g_oriNtOpenProcess = NtOpenProcess;
    g_oriNtCreateFile = NtCreateFile;
	HookManager::GetInstance()->InstallInlinehook((void**)g_oriNtOpenProcess,(void *)FakeNtOpenProcess);
    HookManager::GetInstance()->InstallInlinehook((void**)g_oriNtCreateFile, (void*)FakeNtCreateFile);
	return STATUS_SUCCESS; 
}