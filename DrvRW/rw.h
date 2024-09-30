#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <intrin.h>


namespace rw {
	auto FakeOpenProcess(HANDLE pid) -> HANDLE;
}
