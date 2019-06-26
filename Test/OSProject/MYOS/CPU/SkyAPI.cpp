#include "SkyAPI.h"
#include "SkyConsole.h"
#include "Hal.h"
#include "string.h"
#include "va_list.h"
#include "stdarg.h"
#include "sprintf.h"

CRITICAL_SECTION g_criticalSection;

void SKYAPI kInitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	lpCriticalSection->LockRecursionCount = 0;
	lpCriticalSection->OwningThread = 0;
}

void SKYAPI kEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	if (lpCriticalSection->LockRecursionCount == 0)
	{
		_asm cli
	}

	lpCriticalSection->LockRecursionCount++;

}

void SKYAPI kLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	lpCriticalSection->LockRecursionCount--;

	{
		_asm sti
	}
}