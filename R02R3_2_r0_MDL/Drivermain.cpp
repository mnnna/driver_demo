#include<ntifs.h>
#include<ntddk.h>

void DriverUnload(PDRIVER_OBJECT DriverObject) {

	if (DriverObject->DeviceObject) {
		IoDeleteDevice(DriverObject->DeviceObject);
		UNICODE_STRING symbolLinkName = RTL_CONSTANT_STRING(L"\\??\\DemoInject");
		NTSTATUS status = IoDeleteSymbolicLink(&symbolLinkName);
		if (!NT_SUCCESS(status)) {
			DbgPrint("����ɾ���ɹ���\n");
		}
	}
	DbgPrint("����ж�سɹ���\n");
}

NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	//���ظ�3ring
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	DbgPrint("DispatchCreate��\n");
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}

NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject , PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	//���ظ�3ring
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	DbgPrint("DispatchCreate��\n");
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	char buff[255] = "hello world form r0 \n";

	PVOID sysBuff = pIrp->AssociatedIrp.SystemBuffer;
	if (!MmIsAddressValid(sysBuff)) {
		DbgPrint("sysBuff is null��\n");
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	RtlCopyMemory(sysBuff, buff, sizeof(buff));
	pIrp->IoStatus.Information = sizeof(buff);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	return STATUS_SUCCESS;

 }

NTSTATUS DispatchWrite(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	 
	auto stack = IoGetCurrentIrpStackLocation(pIrp);
	stack->Parameters.Read.Length;

	PVOID sysBuff = pIrp->AssociatedIrp.SystemBuffer;
	if (!MmIsAddressValid(sysBuff) && stack > 0) {
		DbgPrint("sysBuff is null��\n");
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	DbgPrint("sysBuff is %p��\n", sysBuff);
	pIrp->IoStatus.Information = sizeof(stack);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	return STATUS_SUCCESS;

}




EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegisterPath) {
	RegisterPath;
	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS status;
	PDEVICE_OBJECT PDeviceObj;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\DemoInject");
	UNICODE_STRING symbolLINKName = RTL_CONSTANT_STRING(L"\\??\\DemoInject");

	//�����豸����
	status = IoCreateDevice(DriverObject, NULL, &DeviceName, FILE_DEVICE_UNKNOWN, NULL, FALSE, &PDeviceObj);
	if (!NT_SUCCESS(status)){
		return status ;
	}
	
	PDeviceObj->Flags |= DO_BUFFERED_IO ; // �� PDeviceObj->Flags ����Ϊ 0����ζ��������еı�־λ�����豸����ָ���Ĭ��״̬��

	//���� R0 �� R3 �ķ������� 
	status = IoCreateSymbolicLink(&symbolLINKName, &DeviceName);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	//����
	DbgPrint("�������سɹ���\n");
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DbgPrint("�������سɹ���\n");
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	DbgPrint("�������سɹ���\n");
	return STATUS_SUCCESS;
}