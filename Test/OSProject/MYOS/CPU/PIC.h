// 8259 Programmable Interrupt Contoroller
#pragma once
#include <stdint.h>

/*
하드웨어 장치와 데이터를 주고받을 수 있게 하는 틀을 마련한다.
*/

/*
PIC - Programmable Interrupt Controller
키보드나 마우스 등의 이번트 등을 CPU에 전달하는 제어기
ex) 유저가 키보드를 누르면 PIC가 그 신호를 감지하고 인터럽트를 발생시켜 운영체제에 등록된 예외 핸들러를 실행시킨다.
이런식으로 키보드, 마우스, 플로피디스크, 하드디스크 등 다양한 매체에 의해 인터럽트가 발생되고 OS는 이것을 감지한다.
 - OS는 디바이스 식별이 가능해야 한다.

OS는 IRQ(인터럽트 리퀘스트) 신호를 통해 해당 인터럽트의 고유 번호를 알 수 있고, 이 번호를 통해 해당 인터럽트의
출처가 어딘지를 알 수 있다.
IRQ 번호는 마스터에서 슬레이브 순으로 핀 번호를 할당한 것이다.
디바이스에 관련된 인터럽트 핀을 말할 때, 핀 번호 대신 IRQ 번호를 사용한다.
*/

//인터럽트를 발생시키기 위해 PIC1을 사용하는 디바이스 리스트 // Master
#define		I86_PIC_IRQ_TIMER			0
#define		I86_PIC_IRQ_KEYBOARD		1
#define		I86_PIC_IRQ_SERIAL2			3
#define		I86_PIC_IRQ_SERIAL1			4
#define		I86_PIC_IRQ_PARALLEL2		5
#define		I86_PIC_IRQ_DISKETTE		6
#define		I86_PIC_IRQ_PARALLEL1		7

//인터럽트를 발생시키기 위해 PIC2를 사용하는 디바이스 리스트 // Slave
#define		I86_PIC_IRQ_CMOSTIMER		0
#define		I86_PIC_IRQ_CGARETRACE		1
#define		I86_PIC_IRQ_AUXILIARY		4
#define		I86_PIC_IRQ_FPU				5
#define		I86_PIC_IRQ_HDC				6

/*
PIC의 IRQ핀의 작동
	마스터
	1. 마스터 PIC에서 인터럽트가 발생한다.
	2. 마스터 PIC는 자신의 INT에 신호를 싣고 CPU의 INT에 전달한다.
	3. CPU가 인터럽트를 받으면 EFLAG의 IE 비트를 1로 세팅하고 INTA를 통해 받았다는 신호를 PIC에 전달한다.
	4. PIC는 자신의 INTA를 통해 이 신호를 받고 어떤 IRQ에 연결된 장치에서 인터럽트가 발생했는지
	   데이터 버스를 통해 CPU로 전달한다.
	5. CPU는 현재 실행모드가 보호 모드라면 IDT 디스크립터를 찾아서 인터럽트 핸들러를 실행한다.

	슬레이브
	1. 슬레이브 PIC에서 인터럽트가 발생한다.
	2. 슬레이브 PIC는 자신의 INT핀에 신호를 싣고 마스터 PIC IRQ 2번에 인터럽트 신호를 보낸다.
	3. 마스터는 위에서 설명한 5가지의 절차를 진행한다. 
	   단, 이과정에서 CPU에 몇 번째 IRQ에서 인터럽트가 발생했는지 알려줄 때에는 8~15번이 된다.
*/

// PIC 컨트롤러는 크게 두 가지 타입의 커맨드를 제공한다.
// 하나는 초기화와 관련된 ICW(Initialization Command Word) 커맨드
// 다른 하나는 제어와 관련된 OCW(Operation Command Word) 커맨드이다.

//----------------------------------------------------------------
//		디바이스를 초기화하기 위한 커맨드 마스크(자리를 나타내는 비트)
//----------------------------------------------------------------

//! Initialization Control Word 1 bit masks
/*
	ICW1 커맨드의 필드는 마스터와 슬레이브가 같으며, 트리거 모드와 캐스케이드 여부, ICW4의 사용 여부를 설정한다.
	키보드와 마우스 같은 PC 디바이스는 엣지 트리거 방식을 사용하고, PIC 컨트롤러는 마스터-슬레이브 방식으로 동작하므로,
	LTIM 비트 = 0, SNGL = 0으로 설정하면 된다. 또한 PC는 8086 호환 모드로 동작하므로 ICW4 커맨드가 필요하다.
	따라서 IC4=1로 설정하여 ICW4 커맨드를 사용하겠다는 것을 알린다.
*/
#define		I86_PIC_ICW1_MASK_IC4			0x1		//00000001 ICW4를 사용 할 것인가?
#define		I86_PIC_ICW1_MASK_SNGL			0x2		//00000010 단일 구성방식 또는 케스케이드 구성방식 선택
#define		I86_PIC_ICW1_MASK_ADI			0x4		//00000100 0으로 고정
#define		I86_PIC_ICW1_MASK_LTIM			0x8		//00001000 레벨 트리거 방식 또는 엣지 트리거 방식
#define		I86_PIC_ICW1_MASK_INIT			0x10	//00010000 1

/*
	ICW2 커맨드의 필드도 ICW1과 같이 마스터와 슬레이브 모두 같고,
	인터럽트가 발생했을 때 프로세서에 전달할 벡터 번호를 설정하는 역할을 한다.
	벡터 0~31번은 프로세서가 예외를 처리하려고 예약해둔 영역이며, 이 영역을 피하려면 32번 이후로 설정해야 한다.
	PIC 컨트롤러의 인터럽트를 벡터 32번부터 할당하려면 PIC 컨트롤러의 ICW2를 0x20(32)로 설정하고,
	슬레이브 PIC 컨트롤러의 ICW2를 0x28(40)으로 설정하면 된다.
*/

/*
	ICW3 커맨드의 역할은 마스터 PIC 컨트롤러의 몇 번 핀에 슬레이브 PIC 컨트롤러가 연결되었나 하는 것이다.
	ICW3 커맨드의 의미는 두 컨트롤러가 같지만, 필드의 구성은 서로 다르다.
	마스터 PIC 컨트롤러의 ICW3 커맨드 슬레이브 PIC 컨트롤러가 연결된 핀의 위치를 비트로 설정하며,
	슬레이브 PIC 컨트롤러의 ICW3 커맨드는 마스터 PIC 컨트롤러에 연결된 핀의 위치를 숫자로 설정한다.
	PC의 슬레이브 PIC 컨트롤러는 마스터 PIC 컨트롤러의 2번째 핀에 연결되어 있으므로
	마스터 PIC 컨트롤러의 ICW3은 0x04(비트2 = 1)로 설정하고 슬레이브 PIC 컨트롤러의 ICW3은 0x02로 설정한다.
*/

//! Initialization Control Word 4 bit masks
/*
	ICW4 커맨드 필드 역시 마스터와 슬레이브가 같고, EOI 전송 모드와 8086 모드, 확장 기능을 설정한다.
	PC의 PIC 컨트롤러는 8086시절과 별로 변한 것이 없으므로 UMP 비트 = 1 로 설정하여 8086모드로 지정하고,
	이를 제외한 버퍼 모드 같은 확장 기능은 PC에서 사용하지 않으므로
	나머지 SFNM 비트, BUF비트, M/S 비트는 모두 0으로 설정한다.
*/
#define		I86_PIC_ICW4_MASK_UPM			0x1		//00000001 MCS-80/85(0) 모드 또는 8086/88(1)(PC) 모드 선택
#define		I86_PIC_ICW4_MASK_AEOI			0x2		//00000010 프로세서로 인터럽트를 전달한 후 자동으로 EOI 수행함(1)을 설정
#define		I86_PIC_ICW4_MASK_MS			0x4		//00000100 마스터(1) 또는 슬레이브(0) 지정(버퍼 모드를 사용할 때만 유효)
#define		I86_PIC_ICW4_MASK_BUF			0x8		//00001000 버퍼모드 사용 여부 설정 (사용 1 사용X 0)
#define		I86_PIC_ICW4_MASK_SFNM			0x10	//00010000 Special Fully Nested Mode 사용 여부 설정 (사용 1 사용X 0)
													//11100000 0으로 예약됨



//----------------------------------------------------------------
//		디바이스를 제어하기 위한 커맨드 마스크(자리를 나타내는 비트)
//----------------------------------------------------------------

// common word 2 bit masks.
#define		I86_PIC_OCW2_MASK_L1			1		//00000001  Rotate		EOI를 전송할 때 어떤 인터럽트에 대해서 EOI를 전송할지를 설정한다.
#define		I86_PIC_OCW2_MASK_L2			2		//00000010	Specific Level		001 - Non-specifie EOI Command, ISR 레지스터에 설정된 최상위 인터럽트를 제거
#define		I86_PIC_OCW2_MASK_L3			4		//00000100	End of Interrupt	011 : Specific EOI Command, L2~L10 필드에 설정된 오프셋 비트 제거
													//00011000  0으로 예약됨
#define		I86_PIC_OCW2_MASK_EOI			0x20	//00100000  인터럽트 핀 번호를 의미
#define		I86_PIC_OCW2_MASK_SL			0x40	//01000000  OCW2 필드의 SL 비트를 1로 설정하여, 
#define		I86_PIC_OCW2_MASK_ROTATE		0x80	//10000000  특정 인터럽트 핀을 지정할 때 사용한다.

//! Command Word 3 bit masks.
#define		I86_PIC_OCW3_MASK_RIS			1		//00000001 
#define		I86_PIC_OCW3_MASK_RIR			2		//00000010 
#define		I86_PIC_OCW3_MASK_MODE			4		//00000100 
#define		I86_PIC_OCW3_MASK_SMM			0x20	//00100000 
#define		I86_PIC_OCW3_MASK_ESMM			0x40	//01000000 
#define		I86_PIC_OCW3_MASK_D7			0x80	//10000000 



void PICInitialize(uint8_t base0, uint8_t base1);

// PIC로부터 1바이트를 읽는다.
uint8_t ReadDataFromPIC(uint8_t picNum);

// PIC로 데이터를 보낸다.
void SendDataToPIC(uint8_t data, uint8_t picNum);

// PIC로 명령어를 전송한다.
void SendCommandToPIC(uint8_t cmd, uint8_t picNum);

