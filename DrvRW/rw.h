#pragma once
#include <ntifs.h>
#include <ntddk.h>

namespace rw {
	auto FakeOpenProcess(HANDLE pid) -> HANDLE;
}
