#include "HookManager.h"

HookManager* HookManager::mInstance;

bool HookManager::InstallInlinehook(void** originAddr, void* hookAddr)
{
    return false;
}

bool HookManager::RemoveInlinehook(void* hookAddr)
{
    return false;
}

HookManager* HookManager::GetInstance()
{
    if (mInstance == nullptr) {
        mInstance = (HookManager*) ExAllocatePoolWithTag(NonPagedPool, sizeof(HookManager), 'test');
    }
    return nullptr;
}
