#include <ntifs.h>
#include <ntddk.h>

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING RegisterPath) {
	return STATUS_SUCCESS;
}