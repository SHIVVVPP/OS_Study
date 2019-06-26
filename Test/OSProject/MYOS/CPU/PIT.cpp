#include "PIT.h"
#include "HAL.h"
#include "SkyConsole.h"

/*
PIT - Programmable Interval Timer
일상적인 시간 제어 문제를 해결하기 위해 설계된 카운터/타이머 디바이스이다.
3개의 카운터와 1개의 레지스터를 가지고 있다.
040h Counter 0 읽기/쓰기
041h Counter 1 읽기/쓰기
042h Counter 2 읽기/쓰기
043h Counter 3 쓰기
*/
#define I86_PIT_REG_COUNTER0 0x40
#define I86_PIT_REG_COUNTER1 0x41
#define I86_PIT_REG_COUNTER2 0x42
#define I86_PIT_REG_COMMAND 0x43

extern void SendEOI();

volatile uint32_t _pitTicks = 0;
DWORD _lastTickCount = 0;

__declspec(naked) void InterruptPITHandler()
{
	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}
	
	if (_pitTicks - _lastTickCount >= 100)
	{
		_lastTickCount = _pitTicks;
		SkyConsole::Print("Timer Count : %d\n", _pitTicks);
	}

	_pitTicks++;

	SendEOI();

	// 레지스터를 복원하고 원래 수행하던 곳으로 돌아간다.
	_asm
	{
		POPFD
		POPAD
		IRETD
	}
}

//  PIT 초기화
void InitializePIT()
{
	setvect(32, InterruptPITHandler);
}

//  타이머를 시작
/*
PIT는 1초마다 1193181번의 숫자를 카운팅하고 타이머 인터럽트를 발생시킨다.
이 인터럽트를 통해서 우리는 프로세스나 스레드의 스케줄링을 할 수 있으므로 PIT의 역할은 매우 중요하다.

첫번재 인자는 진동수, 두번째 인자는 사용할 카운터 레지스터, 
세번째 인자는 제어 레지스터의 1-3 비트에 설정하는 부분으로 카운터 방식을 설정하는데 쓰인다.
*/
void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode)
{
	if (freq == 0)
		return;

	uint16_t divisor = uint16_t(1193181 / (uint16_t)freq);

	// 커맨드 전송
	uint8_t ocw = 0;
	ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
	ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
	SendPITCommand(ocw);

	//프리퀀시 비율 설정
	SendPITData(divisor & 0xff, 0);
	SendPITData((divisor >> 8) & 0xff, 0);

	//타이머 틱 카운트 리셋
	_pitTicks = 0;
}

void _cdecl msleep(int ms)
{

	unsigned int ticks = ms + GetTickCount();
	while (ticks > GetTickCount())
		;
}


void SendPITCommand(uint8_t cmd) {

	OutPortByte(I86_PIT_REG_COMMAND, cmd);
}

void SendPITData(uint16_t data, uint8_t counter) {

	uint8_t	port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
		((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);

	OutPortByte(port, (uint8_t)data);
}

uint32_t SetPITTickCount(uint32_t i)
{
	uint32_t ret = _pitTicks;
	_pitTicks = i;
	return ret;
}

//! returns current tick count
uint32_t GetPITTickCount() {

	return _pitTicks;
}

unsigned int GetTickCount()
{
	return _pitTicks;
}
