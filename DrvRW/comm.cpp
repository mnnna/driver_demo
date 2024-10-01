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
		PVOID OutputBuffer /*传递给给 r3 的数据*/ , 
		ULONG OutputBufferLength,
		PULONG ReturnOutputBufferLength )->NTSTATUS{
		
		 UNREFERENCED_PARAMETER(PortCookie);
		 UNREFERENCED_PARAMETER(InputBuffer);
		 UNREFERENCED_PARAMETER(InputBufferLength);
		 UNREFERENCED_PARAMETER(OutputBuffer);
		 UNREFERENCED_PARAMETER(OutputBufferLength);
		 UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

		 // 拿到r3 的pid， 通过 pid 伪造句柄返回


		 DbgPrint("msgCallBack");    // 传递伪造的句柄给三环
		 return CommonFunc(InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnOutputBufferLength); // 在DrvMain 里调用 createCommonPort 就会注册这些函数
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

				InitializeObjectAttributes(&oa, &portName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, sd);		//对象属性初始化

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