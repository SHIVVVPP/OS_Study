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
