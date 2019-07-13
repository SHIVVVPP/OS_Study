#pragma once
#include "windef.h"
#include "SkyStruct.h"
#include "Hal.h"
#include "PIT.h"
#include "time.h"

/////////////////////////////////////////////////////
//						동기화
/////////////////////////////////////////////////////
typedef struct _CRITICAL_SECTION
{
	LONG LockRecursionCount;
	HANDLE OwningThread;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

extern CRITICAL_SECTION g_criticalSection;


// VirtualMemoryManager
#define kEnterCriticalSection() __asm PUSHFD __asm CLI
#define kLeaveCriticalSection() __asm POPFD

////////////////////////////////////////////////////
//					ASSERT - kheap에서 추가 // CYNOS.h virtualMemoryManager 위에 Exception도 include
///////////////////////////////////////////////////
#define ASSERT(a,b) if(a==false) CYNConsole::Print("Kernel Panic : %s\n", b) _asm hlt

#define CYN_ASSERT(Expr, Msg)\
	__CYN_ASSERT(#Expr, Expr, __FILE__,__LINE__,Msg)
void __CYN_ASSERT(const char* expr_str, bool expr, const char* file, int line, const char* msg);