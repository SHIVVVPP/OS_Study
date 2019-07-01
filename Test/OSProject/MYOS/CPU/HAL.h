#pragma once
#include "windef.h"


/*
HAL (Hardware Abstraction Layer)
하드웨어 추상화 레이어
컴퓨터의 물리적인 하드웨어와 컴퓨터에서 실행되는 소프트웨어 사이의 추상화 계층이다.
컴퓨터에서 프로그램이 수만가지의 하드웨어를 별 차이 없이 다룰 수 있도록 OS에서 만들어주는 가교 역할을 한다.
HAL을 통해 필요한 기능을 OS에 요청하고 나머지는 OS가 알아서 처리해주도록 한다.
*/

/*
I/O 포트로 데이터 전송 및 받는 함수들을 따로 정리

키보드의 경우
	PC 내부 버스(BUS)와 포트 I/O 방식으로 연결되어 있으며, 포트 어드레스는 0x60와 0x64를 사용한다.
	실제로 할당된 포트는 두 개지만 포트에서 데이터를 읽을 때와 쓸 때 접근하는 레지스터가 다르므로,
	실제로는 네 개의 레지스터와 연결된 것과 같다.

PIT 컨트롤러의 경우
	PIT 컨트롤러는 1개의 컨트롤 레지스터와 3개의 카운터로 구성되며, I/O 포트 0x43, 0x40, 0x41, 0x42에 연결되어 있다.
	이중에서 컨트롤 레지스터는 쓰기만 가능하고, 컨트롤 레지스터를 제외한 카운터는 읽기와 쓰기가 모두 가능하다.

	... 
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

//Exception 만들면서 추가

#ifdef _MSC_VER
#define interrupt __declspec(naked)
#else
#define interrupt
#endif

#pragma pack (push, 1)
typedef struct registers
{
	u32int ds, es, fs, gs;                  // Data segment selector
	u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad.
	//u32int int_no, err_code;    // Interrupt number and error code (if applicable)
	u32int eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;
#pragma pack (pop)


// PhysicalMemoryManager 생성시 추가

#define PAGE_SIZE 4096  // 페이지의 크기는 4096

// PAGE_SIZE 는 4096 이므로 PAGE_SIZE -1 = 1111 1111 1111
// 만약 32bit 라면, ~(PAGE_SIZE - 1) = 11111111 11111111 11110000 00000000
// PAGE_TABLE_ENTRY 구조가
//	페이지 기준 주소[31:12](20bit) Available[11:9](3bit) opation[8:0](9bit) 이므로
// 주소 값만 얻어올 수 있다.
#define PAGE_ALIGN_DOWN(value)	((value) & ~(PAGE_SIZE - 1))

// PAGE - 1 = 1111 1111 1111
// 끝자리가 0000 0000 0000 이라면
// value 그대로
// 아니라면 PAGE_ALIGN_DOWN + PAGE_SIZE 한 주소값
#define PAGE_ALIGN_UP(value)	((value) & (PAGE_SIZE -1)) ? \
									(PAGE_ALIGN_DOWN((value)) + PAGE_SIZE) : ((value))
