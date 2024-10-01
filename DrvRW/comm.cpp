#include "comm.h"

namespace common {

	auto  connectCallBack(
		PFLT_PORT ClientPort,
		PVOID ServerPortCookie,
		PVOID ConnectionContext,
		ULONG SizeOfContext,
		PVOID* ConnectionPortCookie
	)->NTSTATUS{

		UNREFERENCED_PARAMETER(ServerPortCookie);
		UNREFERENCED_PARAMETER(ConnectionContext);
		UNREFERENCED_PARAMETER(ConnectionContext);
		UNREFERENCED_PARAMETER(SizeOfContext);
		UNREFERENCED_PARAMETER(ConnectionPortCookie);

		ClientPort = ClientPort;
		DbgPrint("connect clinet ");
		return STATUS_SUCCESS;
	}

	 auto msgCallBack(
		PVOID PortCookie,
		PVOID InputBuffer,
		ULONG InputBufferLength,
		PVOID OutputBuffer /*���ݸ��� r3 ������*/ , 
		ULONG OutputBufferLength,
		PULONG ReturnOutputBufferLength )->NTSTATUS{
		
		 UNREFERENCED_PARAMETER(PortCookie);
		 UNREFERENCED_PARAMETER(InputBuffer);
		 UNREFERENCED_PARAMETER(InputBufferLength);
		 UNREFERENCED_PARAMETER(OutputBuffer);
		 UNREFERENCED_PARAMETER(OutputBufferLength);
		 UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

		 // �õ�r3 ��pid�� ͨ�� pid α��������


		 DbgPrint("msgCallBack");    // ����α��ľ��������
		 return CommonFunc(InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength); // ��DrvMain ����� createCommonPort �ͻ�ע����Щ����
	}

	auto disconnectCallBack(PVOID ConnectionCookie){
		UNREFERENCED_PARAMETER(ConnectionCookie);
		FltCloseClientPort(fltFilter,&ClientPort);
		DbgPrint("close  client port ");
	}

	auto miniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)->NTSTATUS {
		DbgPrint("hi\n");
		return STATUS_SUCCESS;
	}


	auto createCommonPort(PDRIVER_OBJECT driverObject, PUNICODE_STRING RegisterPath, fnCommoFunc func) -> NTSTATUS {

		UNREFERENCED_PARAMETER(RegisterPath);

		if (func == nullptr) {
			return STATUS_UNSUCCESSFUL;
		}
		CommonFunc = func;

		NTSTATUS status;
		auto freg = FLT_REGISTRATION{
			sizeof(FLT_REGISTRATION),
			FLT_REGISTRATION_VERSION,
			NULL,
			NULL,
			NULL,
			miniUnload,
			nullptr,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL

		};

		status = FltRegisterFilter(driverObject,&freg,&fltFilter);

		if (NT_SUCCESS(status)) {
			OBJECT_ATTRIBUTES oa{ 0 };
			UNICODE_STRING portName = RTL_CONSTANT_STRING(L"\\mf");
			PSECURITY_DESCRIPTOR sd = { nullptr };
			status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
				if (!NT_SUCCESS(status)) {
					DbgPrint("fail to create sd \n");
					FltUnregisterFilter(fltFilter);
					FltFreeSecurityDescriptor(sd);
					return status;
				}

				InitializeObjectAttributes(&oa, &portName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, sd);		//�������Գ�ʼ��

				status = FltCreateCommunicationPort(fltFilter, &drvPort, &oa, nullptr, connectCallBack, disconnectCallBack, msgCallBack, 1);
				if (!NT_SUCCESS(status)) {
					DbgPrint("fail to Create Communication Port \n");
					FltUnregisterFilter(fltFilter);
					FltFreeSecurityDescriptor(sd);
					return status;
				}
				FltFreeSecurityDescriptor(sd);
		}
		return status;
	}
}