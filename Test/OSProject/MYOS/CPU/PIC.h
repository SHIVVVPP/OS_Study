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


//--------------------------------------------
//		디바이스를 제어하기 위한 커맨드
//--------------------------------------------

// common word 2 bit masks. 커맨드를 보낼 때 사용한다.
#define		I86_PIC_OCW2_MASK_L1		1		//00000001
#define		I86_PIC_OCW2_MASK_L2		2		//00000010
#define		I86_PIC_OCW2_MASK_L3		4		//00000100
#define		I86_PIC_OCW2_MASK_EOI		0x20	//00100000
#define		I86_PIC_OCW2_MASK_SL		0x40	//01000000
#define		I86_PIC_OCW2_MASK_ROTATE	0x80	//10000000

//! Command Word 3 bit masks. 커맨드를 보낼때 사용
#define		I86_PIC_OCW3_MASK_RIS		1		//00000001
#define		I86_PIC_OCW3_MASK_RIR		2		//00000010
#define		I86_PIC_OCW3_MASK_MODE		4		//00000100
#define		I86_PIC_OCW3_MASK_SMM		0x20	//00100000
#define		I86_PIC_OCW3_MASK_ESMM		0x40	//01000000
#define		I86_PIC_OCW3_MASK_D7		0x80	//10000000


void PICInitialize(uint8_t base0, uint8_t base1);

// PIC로부터 1바이트를 읽는다.
uint8_t ReadDataFromPIC(uint8_t picNum);

// PIC로 데이터를 보낸다.
void SendDataToPIC(uint8_t data, uint8_t picNum);

// PIC로 명령어를 전송한다.
void SendCommandToPIC(uint8_t cmd, uint8_t picNum);

