#include "PIT.h"
#include "HAL.h"
#include "SkyConsole.h"

/*
PIT 컨트롤러는 1개의 컨트롤 레지스터와 3개의 카운터로 구성되며,
I/O 포트 0x43(컨트롤 레지스터), 0x40, 0x41, 0x42에 연결되어 있다.
이중 컨트롤 레지스터는 쓰기만 가능하고, 컨트롤 레지스터를 제외한 카운터는 읽기 쓰기가 모두 가능하다.

-------------------------------------------------------------------------------------------
포트번호	|		읽기/쓰기		|	레지스터 이름		| 설명
-------------------------------------------------------------------------------------------
0x43	|		쓰기			|	컨트롤 레지스터	| PIT 컨트롤러를 제어하는 레지스터
-------------------------------------------------------------------------------------------
0x40	|		읽기/쓰기		|	카운터0			| 카운터에 사용할 값을 저장하거나
0x41	|		읽기/쓰기		|	카운터1			| 현재 카운터의 값을 읽을때 사용하는 레지스터
0x42	|		읽기/쓰기		|	카운터2			|
-------------------------------------------------------------------------------------------
PIT 컨트롤러의 컨트롤 레지스터의 크기는 1바이트이고, 카운터 레지스터의 크기는 2바이트이다.
I/O 포트는 PIT 컨트롤러와 1바이트 단위로 전송하므로,
컨트롤 레지스터에 전달하는 커맨드에 따라 카운터 I/O 포트로 읽고 쓰는 바이트 수가 결정된다.
컨트롤 레지스터는 4개의 필드로 구성되어 있다.
*/
#define I86_PIT_REG_COUNTER0 0x40
#define I86_PIT_REG_COUNTER1 0x41
#define I86_PIT_REG_COUNTER2 0x42
#define I86_PIT_REG_COMMAND 0x43

extern void SendEOI();

volatile uint32_t _pitTicks = 0;
DWORD _lastTickCount = 0;

/*
PUSHAD
범용 레지스터의 값을 스택에 집어 넣는 명령어이다. 
저장되는 순서는 EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
PUSHFD
플레그 값을 저장한다. EFL
*/
__declspec(naked) void InterruptPITHandler()
{
	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}
	
	/*
	_pitTicks 변수는 타이머 인터럽트 핸들러가 호출될 때마다 카운트가 증가된다.
	_lastTickCount 변수와의 차가 100이면 타이머 문자열을 출력한다.
	StartPITCounter 함수 호출 시 진동수 값을 100으로 설정했기 때문에 위의
	프로시저에서 두 개의 변수값의 차가 100일때, 즉 1초마다 문자열이 정확히 출력된다.*/
	if (_pitTicks - _lastTickCount >= 100)
	{
		_lastTickCount = _pitTicks;
		SkyConsole::Print("Timer Count : %d\n", _pitTicks);
	}

	_pitTicks++;

	
	/*
	EOI - End Of Interrupt
	EOI(End Of Interrupt) 는 PIC(Programmable Interrupt Controller)로 보내지는 신호이며,
	주어진 인터럽트에 대한 인터럽트 처리 완료를 나타낸다.
	*/
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


!!! StartPITCounter로 타이머를 시작하면 1초마다 1193181번의 숫자를 카운팅 하고 타이머 인터럽트를 발생시킨다
StartPITCounter(100,I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN)으로 함수를 호출하는데,

첫번재 인자는 진동수, 두번째 인자는 사용할 카운터 레지스터,
세번째 인자는 제어 레지스터의 1-3 비트에 설정하는 부분으로 카운터 방식을 설정하는데 쓰인다.
제어 레지스터의 구성은 Select Register(2bit) Read Write(2bit) MODE(3bit) BCD(1bit) 이다.
I86_PIT_OCW_MODE_SQUAREWAVEGEN의 값은 0x011(3bit) 이며 MODE의 3bit에 설정된다.

초당 진동수가 100이라는 것은 1초당 100번의 타이머 인터럽트가 발생한다는 의미이며,
이 값을 변화시킴에 따라 초당 인터럽트의 수나 숫자의 증가를 조절할 수 있다.
만약 진동수를 1로 한다면 인터럽트가 발생할 때 PIT의 내부 숫자 카운팅 값은 1193181이 될 것이다.
진동수가 100이 된다면 인터럽트가 발생했을 때의 PIT 내부 숫자 카운팅은 11931이 될 것이다. 이값을 Divisor라한다.
*/
void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode)
{
	if (freq == 0)
		return;

	//Divisor 인터럽트가 발생했을 때의 PIT 내부 숫자 카운팅 값
	uint16_t divisor = uint16_t(1193181 / (uint16_t)freq);

	// 커맨드 전송
	// OCW - Operation Command Word 
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

	/*
	이제 1바이트 제어 레지스터에 우리가 사용할 카운터 레지스터(00) 읽기/쓰기, 모드 값 등을 설정해서 명령을 보내고,
	0번째 카운팅 레지스터에 Divisor 데이터를 보냄으로써 PIT는 초기화 된다.
	그러면 PIT는 CPU에 우리가 설정한 진동수에 맞게 매번 인터럽트를 보낸다.
	*/
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
