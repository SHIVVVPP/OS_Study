#include "CYNOS.h"

void __CYN_ASSERT(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
	if (!expr)
	{
		char buf[256];
		sprintf(buf, "Assert failed: %s Expected: %s %s %d\n", msg, expr_str, file, line);
	
		HaltSystem(buf);
	}
}
