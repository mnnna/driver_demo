#include <ntifs.h>
#include<ntddk.h>

PVOID g_callBack_Handle = 0; 
HANDLE g_pid = 0;

OB_PREOP_CALLBACK_STATUS PobPreOperationCallback( PVOID RegistrationContext,POB_PRE_OPERATION_INFORMATION OperationInformation) {
	UNREFERENCED_PARAMETER(RegistrationContext);
	PEPROCESS process = { 0 };
	PsLookupProcessByProcessId(g_pid,&process); 

	if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
		if (OperationInformation->Object == process) {
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
		}
	}

	return OB_PREOP_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	if (g_callBack_Handle != 0) {
		ObUnRegisterCallbacks(g_callBack_Handle);
	}
}


EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, UNICODE_STRING RegisterPath) {
	UNREFERENCED_PARAMETER(RegisterPath);
	

	OB_CALLBACK_REGISTRATION callback = { 0 };
	OB_OPERATION_REGISTRATION callbackOper = { 0 };


	callback.Version = OB_FLT_REGISTRATION_VERSION;
	callback.OperationRegistrationCount = 1;
	callback.Altitude = RTL_CONSTANT_STRING(L"123.321123");
	callback.RegistrationContext = nullptr;
	callback.OperationRegistration = &callbackOper;

	callbackOper.ObjectType = PsProcessType;
	callbackOper.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	callbackOper.PreOperation = &PobPreOperationCallback;
	callbackOper.PostOperation = nullptr;

	DriverObject->DriverUnload = DriverUnload;

	return ObRegisterCallbacks(&callback, &g_callBack_Handle);

}

