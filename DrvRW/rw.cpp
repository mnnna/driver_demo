#include "rw.h"

namespace rw {
	static auto CopyEprocess(PEPROCESS process) -> PEPROCESS {
		auto fProcess = ExAllocatePool(NonPagedPool, PAGE_SIZE);
		auto cprocess = IoGetCurrentProcess(); 
	}
	auto FakeOpenProcess(HANDLE pid) -> HANDLE {
		
	}
}