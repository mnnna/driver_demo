#include "rw.h"

namespace rw {
	static auto CopyEprocess(PEPROCESS process) -> PEPROCESS {
		auto fProcess = ExAllocatePool(NonPagedPool, PAGE_SIZE);
		auto fakeCr3 = ExAllocatePool(NonPagedPool, PAGE_SIZE);

		if (fProcess == nullptr || fakeCr3 == nullptr || (UINT_PTR)fProcess & 0xfff || (UINT_PTR)fakeCr3 & 0xfff) {
			DbgPrint("failed to allocate mem\n");
			return nullptr;
		}

		auto cprocess = IoGetCurrentProcess(); 
	}


	auto FakeOpenProcess(HANDLE pid) -> HANDLE {
		PEPROCESS Process;
		PEPROCESS FakeProcess;
		HANDLE hProcess; 
		NTSTATUS state;

		FakeProcess = CopyEprocess(Process);
		state = ObOpenObjectByPointer(FakeProcess, 0, 0, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProcess); // 这个函数能根据传进来的句柄创建一个对象

		if (!NT_SUCCESS(state)) {
			DbgPrint("failed to open process\n");
			return nullptr;
		}

		return hProcess;
	}
}