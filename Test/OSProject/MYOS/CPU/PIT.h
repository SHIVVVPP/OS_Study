#ifndef PIT_H
#define PIT_H
#include "stdint.h"

/*
타이머 디바이스는 PIC 컨트롤러의 IRQ 0에 연결되어 있으며,
한 번이나 일정한 주기로 인터럽트를 발생시킨다.
타이머를 PIT(Programmable Interval Timer) 라고도 부르는데,
컨트롤러의 레지스터에 주기를 설정함으로써 발생 간격을 조절 할 수 있다.
타이머 디바이스는 주로 시간 측정이나 시분할 멀티태스킹을 처리할 때 사용한다.
*/


//-----------------------------------------------
//	Operational Command control bits 컨트롤 레지스터 필드 구성과 의미
//-----------------------------------------------

// PIT OCW(Operation Command Word) 제어 커맨드 나타내는 각 비트
#define		I86_PIT_OCW_MASK_BINCOUNT		1		//00000001 BCD비트(0번째 비트)
#define		I86_PIT_OCW_MASK_MODE			0xE		//00001110 Mode비트 (1~3번째 비트)
#define		I86_PIT_OCW_MASK_RL				0x30	//00110000 RW비트 (4~5번째 비트)
#define		I86_PIT_OCW_MASK_COUNTER		0xC0	//11000000 SC(SelectCount)비트 (6~7번째 비트)

/*
0번째 비트 (0000 000^)
BCD
카운터의 값을 바이너리 또는 4비트 BCD 포맷으로 설정한다.
0으로 설정할 경우 바이너리 포맷을 사용함을 의미하며, 1로 설정할 경우 BCD포맷을 사용함을 의미한다.
PIT 컨트롤러 내부의 카운터는 2바이트 이므로,BCD 포맷은 0~9999까지 가능하며,
바이너리 포맷은 0x00 ~ 0xFFFF까지 가능하다.
*/
#define		I86_PIT_OCW_BINCOUNT_BINARY		0		//0
#define		I86_PIT_OCW_BINCOUNT_BCD		1		//1

/*
1~3번째 비트 (0000 ^^^0)
Mode
PIT 컨트롤러의 카운트 모드를 설정한다.
*/
#define		I86_PIT_OCW_MODE_TERMINALCOUNT	0		//0000 Interrupt during counting
#define		I86_PIT_OCW_MODE_ONESHOT		0x2		//0010 Programmable monoflop
#define		I86_PIT_OCW_MODE_RATEGEN		0x4		//0100 Clock rate generator
#define		I86_PIT_OCW_MODE_SQUAREWAVEGEN	0x6		//0110 Square wave generator
#define		I86_PIT_OCW_MODE_SOFTWARETRIG	0x8		//1000 Software-triggered impulse
#define		I86_PIT_OCW_MODE_HARDWARETRIG	0xA		//1010 Hardware-triggered impulse

/*
4~5번째 비트 (00^^ 0000)
RW
-Read/Write의 약자이며, 카운터에 읽기 또는 쓰기를 수행할 때 어떠한 용도로 사용할지를 설정한다.
*/
#define		I86_PIT_OCW_RL_LATCH			0			//000000 카운터의 현재 값을 읽음(Latch Instruction), 2바이트 전송
#define		I86_PIT_OCW_RL_LSBONLY			0x10		//010000 카운터의 하위 바이트를 읽거나 씀, 1바이트 전송
#define		I86_PIT_OCW_RL_MSBONLY			0x20		//100000 카운터의 상위 바이트를 읽거나 씀, 1바이트 전송
#define		I86_PIT_OCW_RL_DATA				0x30		//110000 카운터의 하위 바이트에서 상위 바이트 순으로 연속해서 I/O 포트를 읽거나 쓴다. 2바이트 전송
// 비트 00 으로 설정할 경우 , 비트 3~0은 모두 0으로 설정하고 하위 바이트에서 상위 바이트 순으로 연속해서 I/O 포트를 읽어야 한다.

/*
6~7번째 비트 (^^00 0000)
SC
-Select Counter의 약자이며, 커맨드의 대상이 되는 카운터를 저장한다.
*/
#define		I86_PIT_OCW_COUNTER_0			0		//00000000 카운터 0
#define		I86_PIT_OCW_COUNTER_1			0x40	//01000000 카운터 1
#define		I86_PIT_OCW_COUNTER_2			0x80	//10000000 카운터 2

/*
PIT 컨트롤러의 내부 클록은 1.193181Mhz으로 동작하며, 매회마다 각 카운터의 값을 1씩 감소시켜 0이 되었을 때 신호를 발생시킨다.
신호를 발생시킨 이후에 PIT 컨트롤러는 설정된 모드에 따라 다르게 동작한다.
*/
#define TimerFrequency 1193180


extern void SendPITCommand(uint8_t cmd);
extern void SendPITData(uint16_t data, uint8_t counter);

extern uint32_t SetPITTickCount(uint32_t i);
extern uint32_t GetPITTickCount();

void InitializePIT();


unsigned int GetTickCount();
void _cdecl msleep(int ms);

extern void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode);
#endif