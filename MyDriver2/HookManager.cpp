#include "HookManager.h"

HookManager* HookManager::mInstance;

bool HookManager::InstallInlinehook(void** originAddr, void* hookAddr)
{
    return false;
}

bool HookManager::RemoveInlinehook(void* hookAddr)
{
    static bool bFrist = true;
    if (bFrist) {
        mTrampLinePool = (char*)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 4, 'Jmp');
    }

    return false;
}

HookManager* HookManager::GetInstance()
{
    if (mInstance == nullptr) {
        mInstance = (HookManager*) ExAllocatePoolWithTag(NonPagedPool, sizeof(HookManager), 'test');
    }
    return nullptr;
}
