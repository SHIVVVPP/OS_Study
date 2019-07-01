#pragma once
#include "windef.h"
#include "SkyStruct.h"
#include "Hal.h"
#include "PIT.h"
#include "time.h"

/////////////////////////////////////////////////////
//						µø±‚»≠
/////////////////////////////////////////////////////
typedef struct _CRITICAL_SECTION
{
	LONG LockRecursionCount;
	HANDLE OwningThread;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

extern CRITICAL_SECTION g_criticalSection;

void SKYAPI kEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void SKYAPI kInitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void SKYAPI kLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

// VirtualMemoryManager
#define kEnterCriticalSection() __asm PUSHFD __asm CLI
#define kLeaveCriticalSection() __asm POPFD