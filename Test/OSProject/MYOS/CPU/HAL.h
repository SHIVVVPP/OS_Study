#pragma once
#include "windef.h"

/*
HAL (Hardware Abstraction Layer)
하드웨어 추상화 레이어
컴퓨터의 물리적인 하드웨어와 컴퓨터에서 실행되는 소프트웨어 사이의 추상화 계층이다.
컴퓨터에서 프로그램이 수만가지의 하드웨어를 별 차이 없이 다룰 수 있도록 OS에서 만들어주는 가교 역할을 한다.
HAL을 통해 필요한 기능을 OS에 요청하고 나머지는 OS가 알아서 처리해주도록 한다.
*/

extern "C" int _outp(unsigned short, int);
extern "C" unsigned long _outpd(unsigned int, int);
extern "C" unsigned short _outpw(unsigned short, unsigned short);
extern "C" int _inp(unsigned short);
extern "C" unsigned short _inpw(unsigned short);
extern "C" unsigned long _inpd(unsigned int shor);

void OutPortByte(ushort port, uchar value);
void OutPortWord(ushort port, ushort value);
void OutPortDWord(ushort port, unsigned int value);
uchar InPortByte(ushort port);
ushort InPortWord(ushort port);
long InPortDWord(unsigned int port);


//PIT 만들면서 추가
void setvect(int intno, void(&vect)(), int flags = 0);