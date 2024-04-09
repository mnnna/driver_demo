#pragma once
#include <ntddk.h>
#include <ntifs.h>

#define  DEVICENAME L"\\Device\\DemoInject"
#define  SYBOLNAME L"\\??\\DemoInject"

EXTERN_C NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP pIrp);
EXTERN_C NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP pIrp);
EXTERN_C NTSTATUS DispatchControl(PDEVICE_OBJECT DeviceObject, PIRP pIrp);