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
		PEPROCESS 
		
		PEPROCESS FakeProcess;



		CopyProcess()
	}
}