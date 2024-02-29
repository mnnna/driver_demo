#include "HookManager.h"

HookManager* HookManager::mInstance;

bool HookManager::InstallInlinehook(void** originAddr, void* hookAddr)
{
    static bool bFrist = true;
    if (bFrist) {
        mTrampLinePool = (char*)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE * 4, 'Jmp');

        if (!mTrampLinePool) {
            DbgPrint("Error InstallInlinehook");
            return false;
        };

        RtlSecureZeroMemory(mTrampLinePool, PAGE_SIZE * 4);   
        mPoolUSED = 0;
        bFrist = false;
        return true;
    }

    if (mHookCount == MAX_HOOK_COUNT) {
        DbgPrint("FULL");
        return false;
    };

    const UINT32 trampLineByteCount = 30;
    // 0x6A 0x00 0x3E 0xC7 0x04 0x24 0x00 0x00 0x00 0x00 0x3E 0xC7 0x44 0x24 0x04 0x00 0x00 0x00 0x00 0xC3
    char TrampLineCode[trampLineByteCount] = { 0x6A,0x00 ,0x3E ,0xC7 ,0x04 ,0x24 ,0x00 ,0x00 ,0x00 ,0x00 ,0x3E ,0xC7 ,0x44 ,0x24 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0xC3 };

    char* curTrampLinePool = mTrampLinePool + mPoolUSED;
    char* startJmoAddr = (char*)*originAddr; 
    UINT32 uBreakBytes = 0; 

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
    return mInstance;
}
