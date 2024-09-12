#include "CommIO.h"


void DriverUnload(PDRIVER_OBJECT pDriver){
	if (pDriver->DeviceObject) {
		IoDeleteDevice(pDriver->DeviceObject);
		UNICODE_STRING SymbolName = RTL_CONSTANT_STRING(SYBOLNAME);
		NTSTATUS status = IoDeleteSymbolicLink(&SymbolName);
		if (!NT_SUCCESS(status)) {
			DbgPrint("����ɾ���ɹ���\n");
		}
	}
	DbgPrint("����ж�سɹ���\n"); 
}

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegisterPath) {
	RegisterPath;
	NTSTATUS status = STATUS_SUCCESS;
	pDriver->DriverUnload = DriverUnload;

	PDEVICE_OBJECT pDevice = NULL;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICENAME);
	UNICODE_STRING SymbolName = RTL_CONSTANT_STRING(SYBOLNAME);

	//�����豸����
	status = IoCreateDevice(pDriver, NULL, &DeviceName, FILE_DEVICE_UNKNOWN, NULL, FALSE, &pDevice);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	//pDeviceObj->Flags = 0; 


	status = IoCreateSymbolicLink(&SymbolName, &DeviceName);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(pDevice);
		return status;
	}

	//���� 
	pDriver->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;


	DbgPrint("���سɹ�  !\n");
	return STATUS_SUCCESS;
}



