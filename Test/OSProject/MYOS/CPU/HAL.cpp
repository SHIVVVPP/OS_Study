#include "HAL.h"
#include "IDT.h"

void OutPortByte(ushort port, uchar value)
{
	_outp(port, value);
}

void OutPortWord(ushort port, ushort value)
{
	_outpw(port, value);
}

void OutPortDWord(ushort port, unsigned int value)
{
	_outpd(port, value);
}

long InPortDWord(unsigned int port)
{
	return _inpd(port);
}

uchar InPortByte(ushort port)
{
#ifdef _MSC_VER
	_asm {
		mov		dx, word ptr[port]
		in		al, dx
		mov		byte ptr[port], al
	}
#endif
	return (unsigned char)port;
	//return (uchar)_inp(port);
}

ushort InPortWord(ushort port)
{
	return _inpw(port);
}


// 인터럽트 서비스 프로시저
void setvect(int intno, void(&vect)(), int flags)
{
	InstallInterrputHandler(intno, (uint16_t)(I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32 | flags), 0x8, vect);
}
