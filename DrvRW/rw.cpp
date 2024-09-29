#include "rw.h"

namespace rw {
	static auto VaToPa(void* va) -> uintptr_t{
		return MmGetPhysicalAddress(va).QuadPart;
	}

	static auto PaToVa(uintptr_t pa) -> void*{
		return MmGetVirtualForPhysical({ .QuadPart=(signed long long )pa});

	}

	static auto CopyEprocess(PEPROCESS process) -> PEPROCESS {

		KAPC_STATE apc = { 0 };
		auto fProcess = ExAllocatePool(NonPagedPool, PAGE_SIZE);
		auto fakeCr3 = ExAllocatePool(NonPagedPool, PAGE_SIZE);

		if (fProcess == nullptr || fakeCr3 == nullptr || (UINT_PTR)fProcess & 0xfff || (UINT_PTR)fakeCr3 & 0xfff) {
			DbgPrint("failed to allocate mem\n");
			return nullptr;
		} 

		auto cprocess = IoGetCurrentProcess(); 

		memcpy(fProcess, PAGE_ALIGN(cprocess),PAGE_SIZE); //��ҳ���룬������ͷһ�𿽱�
		KeStackAttachProcess(process, &apc);

		auto gameCr3 = __readcr3();
		auto gameCr3Va = PaToVa(gameCr3 & ~0xfff); // ȥ�� pcid
		memcpy(fakeCr3, gameCr3Va, PAGE_SIZE);
		KeUnstackDetachProcess(&apc);

		auto offset = (UINT_PTR)cprocess & 0xfff;
		fProcess + offset +  0x28// �滻 Cr3 �� CR3 ��ֵ�洢�� KPROCESS �� DirectoryTableBase ��Ա�С�

	}


	auto FakeOpenProcess(HANDLE pid) -> HANDLE {
		PEPROCESS Process;
		PEPROCESS FakeProcess;
		HANDLE hProcess; 
		NTSTATUS state;

		state = PsLookupProcessByProcessId(pid, &Process);

		if (!NT_SUCCESS(state)) {
			DbgPrint("failed to look up process\n");
			return nullptr;
		}

		FakeProcess = CopyEprocess(Process);

		state = ObOpenObjectByPointer(FakeProcess, 0, 0, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProcess); // ��������ܸ��ݴ������ľ������һ������

		if (!NT_SUCCESS(state)) {
			DbgPrint("failed to open process\n");
			return nullptr;
		}

		return hProcess;
	}
}