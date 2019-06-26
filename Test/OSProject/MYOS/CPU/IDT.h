#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>

//Interrupt Descriptor Table : IDT는 인터럽트를 다루는 인터페이스를 제공할 책임이 있다.
//인터페이스 설치, 요청, 그리고 인터럽트를 처리하는 콜백 루틴

//인터럽트 핸들러의 최대 개수 : 256
#define I86_MAX_INTERRUPTS		256

#define I86_IDT_DESC_BIT16		0x06	//00000110
#define I86_IDT_DESC_BIT32		0x0E	//00001110
#define I86_IDT_DESC_RING1		0x40	//01000000
#define I86_IDT_DESC_RING2		0x20	//00100000
#define I86_IDT_DESC_RING3		0x60	//01100000
#define I86_IDT_DESC_PRESENT	0x80	//10000000

//인터럽트 핸들러 정의 함수
//인터럽트 핸들러는 프로세서가 호출한다. 이때 스택 구성이 변하는데
//인터럽트를 처리하고 리턴할때 스택이 인터럽트 핸들러 호출직전의 구성과 똑같음을 보장해야 한다.
typedef void (_cdecl *I86_IRQ_HANDLER)(void);

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

//IDTR 구조체
/*
IDT를 구축하고 난뒤 IDT의 위치를 CPU에 알려주기 위한 IDTR 구조체를 만들어야 한다.
CPU는 IDTR 레지스터를 통해서 IDT에 접근한다.
*/
struct idtr {

	//IDT의 크기
	uint16_t limit;

	//IDT의 베이스 주소
	uint32_t base;
};

//인터럽트 디스크립터
/*
인터럽트 디스크립터의 크기는 8바이트이다.
이 8바이트 중에서 2바이트 크기를 차지하는 세그먼트 셀렉터를 통해서
GDT 내의 글로벌 디스크립터를 찾을 수 있고 이 글로벌 디스크립터를 통해서 세그먼트의 베이스 주소를 얻을 수 있다.
세그먼트 베이스 주소에서 오프셋 값을 더하면 ISR(인터럽트 서비스 루틴)의 주소를 얻을 수 있다.
offsetLow, offsetHigh 필드로 ISR의 4바이트 오프셋 값을 얻을 수 있다.
selector 필드로 GDT에서 디스크립터를 찾아 세그먼트의 베이스 주소를 얻을 수 있다.
*/
struct idt_descriptor {

	//인터럽트 핸들러 주소의 0-16 비트
	uint16_t		baseLo;

	//GDT의 코드 셀렉터
	uint16_t		sel;

	//예약된 값 0이어야 한다.
	uint8_t			reserved;

	//8비트 비트 플래그
	uint8_t			flags;

	//인터럽트 핸들러 주소의 16-32 비트
	uint16_t		baseHi;
};

#ifdef _MSC_VER
#pragma pack (pop)
#endif

//i번째 인터럽트 디스크립트를 얻어온다.
idt_descriptor* GetInterruptDescriptor(uint32_t i);

//인터럽트 핸들러 설치.
bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER);
bool IDTInitialize(uint16_t codeSel);
void InterrputDefaultHandler();

#endif
