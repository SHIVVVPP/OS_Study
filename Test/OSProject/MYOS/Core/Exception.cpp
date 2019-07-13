#include "Exception.h"
#include "HAL.h" // interrupt(declspec(naked)) 와 register_t 정의
#include <stdint.h>
#include "CYNConsole.h"
#include "sprintf.h"
#include "string.h"

//#include "kmain.h"

#pragma warning (disable:4100)

static char* sickpc = " (>_<) MY OS Error!\n\n";

void _cdecl kExceptionMessageHeader()
{
	char* disclamer =
		"We apologize, OS has encountered a problem and has been shut down\n\
to prevent damage to your computer. Any unsaved work might be lost.\n\
We are sorry for the inconvenience this might have caused.\n\n\
Please report the following information and restart your computer\n\
The system has been halted.\n\n";

	CYNConsole::MoveCursor(0, 0);
	CYNConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	CYNConsole::Clear();
	CYNConsole::Print(sickpc);
	CYNConsole::Print(disclamer);
}


void HandleDivideByZero(registers_t regs)
{
	kExceptionMessageHeader();
	CYNConsole::Print("Divide by 0 at Address[0x%x:0x%x]\n", regs.cs, regs.eip);
	CYNConsole::Print("EFLAGS[0x%x]\n", regs.eflags);
	CYNConsole::Print("ss : 0x%x\n", regs.ss);
	for (;;);
}

//extern int _divider;
//void HandleDivideByZero(registers_t regs)
//{
//	_divider = 10;
//}

interrupt void kHandleDivideByZero()
{
	_asm {
		cli
		pushad

		push ds
		push es
		push fs
		push gs
	}

	_asm
	{
		call HandleDivideByZero// 위의 HandleDivideByZero함수 호출
	}

	_asm {

		pop gs
		pop fs
		pop es
		pop ds

		popad

		mov al, 0x20
		out 0x20, al
		sti
		iretd
	}
}

interrupt void kHandleSingleStepTrap()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Single step\n");
	for (;;);
}

interrupt void kHandleNMITrap()
{
	kExceptionMessageHeader();
	CYNConsole::Print("NMI trap\n");
	for (;;);
}

interrupt void kHandleBreakPointTrap()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Breakpoint trap\n");
	for (;;);
}

interrupt void kHandleOverflowTrap()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Overflow trap");
	for (;;);
}

interrupt void kHandleBoundsCheckFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Bounds check fault\n");
	for (;;);
}

interrupt void kHandleInvalidOpcodeFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Invalid opcode");
	for (;;);
}

interrupt void kHandleNoDeviceFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Device not found\n");
	for (;;);
}

interrupt void kHandleDoubleFaultAbort()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Double fault\n");
	for (;;);
}

interrupt void kHandleInvalidTSSFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Invalid TSS\n");
	for (;;);
}

interrupt void kHandleSegmentFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Invalid segment\n");
	for (;;);
}

interrupt void kHandleStackFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Stack fault\n");
	for (;;);
}

interrupt void kHandleGeneralProtectionFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("General Protection Fault\n");
	for (;;);
}

interrupt void kHandlePageFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Page Fault\n");
	for (;;);
}

interrupt void kHandleFPUFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Alignment Check\n");
	for (;;);
}

interrupt void kHandleAlignedCheckFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Alignment Check\n");
	for (;;);
}

interrupt void kHandleMachineCheckAbort()
{
	kExceptionMessageHeader();
	CYNConsole::Print("Machine Check\n");
	for (;;);
}

interrupt void kHandleSIMDFPUFault()
{
	kExceptionMessageHeader();
	CYNConsole::Print("FPU SIMD fault\n");
	for (;;);
}

void HaltSystem(const char* errMsg)
{
	CYNConsole::MoveCursor(0, 0);
	CYNConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	CYNConsole::Clear();
	CYNConsole::Print(sickpc);

	CYNConsole::Print("*** STOP: %s", errMsg);

	__asm
	{
	halt:
		jmp halt;
	}
}
