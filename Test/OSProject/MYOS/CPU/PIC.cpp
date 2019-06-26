#ifndef ARCH_X86
#error "[PIC.cpp for i86] requires i86 architecture. Define ARCH_X86"
#endif

#include "PIC.h"
#include "HAL.h"
//-----------------------------------------------
//	64비트 멀티 코어 OS 415page
//-----------------------------------------------

//-----------------------------------------------
//	Controller Registers
//-----------------------------------------------

/*
PIC 컨트롤러는 키보드 컨트롤러와 마찬가지로 I/O 포트 방식으로 연결되어 있다.
우리가 사용하는 PC는 PIC 컨트롤러에 각각 두 개의 I/O 포트를 할당하며,
마스터 PIC 컨트롤러는 0x20과 0x21을 사용하고
슬레이브 PIC 컨트롤러는 0xA0와 0xA1를 사용한다.
PIC 컨트롤러에 할당된 I/O 포트는 읽기와 쓰기가 모두 가능하다.

I/O포트와 PIC 컨트롤러와의 관계

				포트 번호							읽기 수행			쓰기수행
마스터 PIC 컨트롤러		슬레이브 PIC 컨트롤러
---------------------------------------------------------------------------
0x20				|	0xA0				|	IRR 레지스터	|	ICW1 커맨드
					|						|	ISR 레지스터	|	OCW2 커맨드
					|						|				|	OCW3 커맨드
---------------------------------------------------------------------------
0x21				|	0xA1				|	IMR 레지스터	|	ICW2 커맨드
					|						|				|	ICW3 커맨드
					|						|				|	ICW4 커맨드
					|						|				|	OCW1 커맨드
---------------------------------------------------------------------------
*/

//! PIC 1 레지스터 포트 주소
#define I86_PIC1_REG_COMMAND	0x20
#define I86_PIC1_REG_STATUS		0x20
#define I86_PIC1_REG_DATA		0x21
#define I86_PIC1_REG_IMR		0x21

//! PIC 2 레지스터 포트 주소
#define I86_PIC2_REG_COMMAND	0xA0
#define I86_PIC2_REG_STATUS		0xA0
#define I86_PIC2_REG_DATA		0xA1
#define I86_PIC2_REG_IMR		0xA1


//-----------------------------------------------
//	Initialization Command 1 제어 비트
//-----------------------------------------------

#define I86_PIC_ICW1_IC4_EXPECT				1			//1
#define I86_PIC_ICW1_IC4_NO					0			//0

#define I86_PIC_ICW1_SNGL_YES				2			//10
#define I86_PIC_ICW1_SNGL_NO				0			//00

#define I86_PIC_ICW1_ADI_CALLINTERVAL4		4			//100
#define I86_PIC_ICW1_ADI_CALLINTERVAL8		0			//000

#define I86_PIC_ICW1_LTIM_LEVELTRIGGERED	8			//1000
#define I86_PIC_ICW1_LTIM_EDGETRIGGERED		0			//0000

#define I86_PIC_ICW1_INIT_YES				0x10		//10000
#define I86_PIC_ICW1_INIT_NO				0			//00000

//-----------------------------------------------
//	Initialization Command 4 제어 비트
//-----------------------------------------------

#define I86_PIC_ICW4_UPM_86MODE			1			//1
#define I86_PIC_ICW4_UPM_MCSMODE		0			//0

#define I86_PIC_ICW4_AEOI_AUTOEOI		2			//10
#define I86_PIC_ICW4_AEOI_NOAUTOEOI		0			//0

#define I86_PIC_ICW4_MS_BUFFERMASTER	4			//100
#define I86_PIC_ICW4_MS_BUFFERSLAVE		0			//0

#define I86_PIC_ICW4_BUF_MODEYES		8			//1000
#define I86_PIC_ICW4_BUF_MODENO			0			//0

#define I86_PIC_ICW4_SFNM_NESTEDMODE	0x10		//10000
#define I86_PIC_ICW4_SFNM_NOTNESTED		0			//a binary 2 (futurama joke hehe ;)




/*
PIC 컨트롤러의 초기화 작업은 마스터 및 슬레이브 PIC 컨트롤러에 개별적으로 수행해야 한다.
초기화에 사용하는 값은 마스터 PIC 컨트롤러와 슬레이브 PIC 컨트롤러가 거의 같다.
*/
// PICInitialize(0x20, 0x28); 0x20으로 설정하면 PIC 컨트롤러 0~7번 핀의 인터럽트는 프로세서의 벡터 0x20~0x27로 전달된다.
// 따라서 0x20, 0x28 로설정한 것은 마스터 pic는 벡터 0x20~0x27, 슬레이브 PIC는 벡터 0x28~0x2F
void PICInitialize(uint8_t base0, uint8_t base1)
{
	/*
	IRQ 하드웨어 인터럽트가 발생할 때 적절히 작동하도록 하기 위해 PIC가 가진 IRQ를 초기화 해주어야 한다.
	이를 위해 마스터 PIC의 명령 레지스터로 명령을 전달해야 하는데 이때 ICW(Initialization Control Word)가 사용된다.
	이 ICW는 4가지의 초기화 명령어로 구성된다.
	*/

	/*
	PIC 컨트롤러를 초기화 하는 작업은 ICW1 커맨드를 I/O 포트 0x20 또는 0xA0에 쓰는 것으로 시작한다.
	0x20나 0xA0 포트로 ICW1을 보내면 0x21나 0xA1 포트에 쓰는 데이터는 ICW2,ICW3,ICW4 순으로 해석되며,
	전송이 완료되면 PIC 컨트롤러는 수신된 데이터를 바탕으로 자신을 초기화 한다.
	*/

	// 0 마스터 PIC 포트, 1 슬레이브 PIC 포트

	uint8_t	icw = 0;

	// PIC 초기화 ICW1 명령을 보낸다.
	// (icw & ~bit mask) | option 
	// 기존 icw의 비트 값을 유지하면서 비트 마스크 부분만 바꾼다.
	icw = (icw & ~I86_PIC_ICW1_MASK_INIT) | I86_PIC_ICW1_INIT_YES;
	icw = (icw & ~I86_PIC_ICW1_MASK_IC4) | I86_PIC_ICW1_IC4_EXPECT;

	//SendCommandToPIC(icw, 0); // 0x11 (0001 0001) LTIM 비트 = 0, SNGL 비트 = 0, IC4 비트 = 1
	//SendCommandToPIC(icw, 1);
	SendCommandToPIC(0x11, 0);
	SendCommandToPIC(0x11, 1);

	//! PIC에 ICW2 명령을 보낸다. base0와 base1은 IRQ의 베이스 주소를 의미한다.
	SendDataToPIC(base0, 0); // 마스터 PIC에 인터럽트 벡터를 0x20(32) 부터 차례로 할당
	SendDataToPIC(base1, 1); // 슬레이브 PIC에 인터럽트 벡터를 0x28(40) 부터 차례로 할당

	//PIC에 ICW3 명령을 보낸다. 마스터와 슬레이브 PIC와의 관계를 정립한다.
	SendDataToPIC(0x04, 0);
	SendDataToPIC(0x02, 1);

	//ICW4 명령을 보낸다. i86 모드를 활성화 한다.
	icw = (icw & ~I86_PIC_ICW4_MASK_UPM) | I86_PIC_ICW4_UPM_86MODE;

	SendDataToPIC(icw, 0);
	SendDataToPIC(icw, 1);
	//PIC 초기화 완료

	/*
	PIC가 초기화 되면,
	EX) 유저가 키보드를 누르면 PIC가 그 신호를 감지하고 인터럽트를 발생시켜
	운영체제에 *등록된* 예외 핸들러를 실행시킨다! 
	*/
}

// 0이면 0x20포트(마스터 PIC 포트) 1이면 0xA0포트(슬레이브 PIC 포트)
//PIC로 명령을 보낸다
inline void SendCommandToPIC(uint8_t cmd, uint8_t picNum) {

	if (picNum > 1)
		return;

	uint8_t	reg = (picNum == 1) ? I86_PIC2_REG_COMMAND : I86_PIC1_REG_COMMAND;
	OutPortByte(reg, cmd);
}


//PIC로 데이터를 보낸다.
inline void SendDataToPIC(uint8_t data, uint8_t picNum) {

	if (picNum > 1)
		return;

	uint8_t	reg = (picNum == 1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
	OutPortByte(reg, data);
}


//PIC로부터 1바이트를 읽는다
inline uint8_t ReadDataFromPIC(uint8_t picNum) {

	if (picNum > 1)
		return 0;

	uint8_t	reg = (picNum == 1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
	return InPortByte(reg);
}
