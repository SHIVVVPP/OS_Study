#include "CYNTest.h"

void TestFPU()
{
	float sampleFloat = 0.3f;
	sampleFloat *= 5.482f;
	CYNConsole::Print("Sample Float Value %f\n", sampleFloat);
}

int _divider = 0;
int _dividend = 100;
void TestInterrupt()
{
	int result = _dividend / _divider;

	if (_divider != 0)
		result = _dividend / _divider;

	CYNConsole::Print("Result is %d, divider : %d\n", result, _divider);
}

