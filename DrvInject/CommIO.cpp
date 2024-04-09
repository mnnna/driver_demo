#include"CommIO.h"

NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	//返回给3ring
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	DbgPrint("DispatchCreate！\n");

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}

NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	//返回给3ring
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	DbgPrint("DispatchClose！\n");

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}

NTSTATUS DispatchControl(PDEVICE_OBJECT DeviceObject, PIRP pIrp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	char buff[255] = "hello world r0";

	PVOID sysBuff = pIrp->AssociatedIrp.SystemBuffer;

	auto stack = IoGetCurrentIrpStackLocation(pIrp);
	int length = stack->Parameters.Read.Length;
	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	DbgPrint("DispatchClose！\n");

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}