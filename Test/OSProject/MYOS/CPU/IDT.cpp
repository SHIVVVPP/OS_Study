#include "IDT.h"
#include "string.h"
#include "memory.h"

//인터럽트 디스크립터 테이블
static struct idt_descriptor	_idt [I86_MAX_INTERRUPTS];

//CPU의 IDTR 레지스터를 초기화하는데 도움을 주는 IDTR 구조체
static struct idtr				_idtr;

//IDTR 레지스터에 IDT의 주소값을 넣는다.
static void IDTInstall() {
#ifdef _MSC_VER
	_asm lidt [_idtr]
#endif
}

/*
EOI - End Of Interrupt
EOI(End Of Interrupt) 는 PIC(Programmable Interrupt Controller)로 보내지는 신호이며,
주어진 인터럽트에 대한 인터럽트 처리 완료를 나타낸다.
*/
#define DMA_PICU1       0x0020 // IO base Address for master PIC
#define DMA_PICU2       0x00A0 // IO base address for slave PIC
__declspec(naked) void SendEOI() //PIT 구현하면서 추가 E
{
	_asm
	{
		PUSH EBP
		MOV  EBP, ESP
		PUSH EAX

		; [EBP] < -EBP
		; [EBP + 4] < -RET Addr
		; [EBP + 8] < -IRQ 번호

		MOV AL, 20H; EOI 신호를 보낸다.
		OUT DMA_PICU1, AL

		CMP BYTE PTR[EBP + 8], 7
		JBE END_OF_EOI
		OUT DMA_PICU2, AL; Send to 2 also

		END_OF_EOI :
		POP EAX
			POP EBP
			RET
	}
}


//다룰수 있는 핸들러가 존재하지 않을때 호출되는 기본 핸들러
__declspec(naked) void InterrputDefaultHandler () {

	//레지스터를 저장하고 인터럽트를 끈다. 
	_asm {
		cli // cliear interrupt 인터럽트 플래그를 0(interrupt disable)으로 설정
		pushad
	}

	// 레지스터를 복원하고 원래 수행하던 곳으로 돌아간다.
	_asm {
		mov al, 0x20
		out 0x20, al
		popad
		sti // set interrupt 인터럽트 플래그를 1(interrupt enable)로 설정
		iretd
	}
}

//i번째 인터럽트 디스크립트를 얻어온다.
idt_descriptor* GetInterruptDescriptor(uint32_t i) {

	if (i>I86_MAX_INTERRUPTS)
		return 0;

	return &_idt[i];
}

//인터럽트 핸들러 설치	//I86_IRQ_HANDLER - 인터럽트 핸들러 함수 
bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {

	if (i>I86_MAX_INTERRUPTS)
		return false;

	if (!irq)
		return false;

	//인터럽트(함수)의 베이스 주소를 얻어온다.
	uint64_t		uiBase = (uint64_t)&(*irq);
	
	if ((flags & 0x0500) == 0x0500) {
		_idt[i].sel = sel;
		_idt[i].flags = uint8_t(flags);
	}
	else
	{
		//포맷에 맞게 인터럽트 핸들러와 플래그 값을 디스크립터에 세팅한다.
		_idt[i].baseLo = uint16_t(uiBase & 0xffff);
		_idt[i].baseHi = uint16_t((uiBase >> 16) & 0xffff);
		_idt[i].reserved = 0;
		_idt[i].flags = uint8_t(flags);
		_idt[i].sel = sel;
	}

	return	true;
}

//IDT를 초기화하고 디폴트 핸들러를 등록한다
bool IDTInitialize(uint16_t codeSel) {

	//IDTR 레지스터에 로드될 구조체 초기화
	_idtr.limit = sizeof(struct idt_descriptor) * I86_MAX_INTERRUPTS - 1;
	_idtr.base = (uint32_t)&_idt[0];

	//NULL 디스크립터
	memset((void*)&_idt[0], 0, sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1);

	//디폴트 핸들러 등록
	for (int i = 0; i<I86_MAX_INTERRUPTS; i++)
		InstallInterrputHandler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32,
			codeSel, (I86_IRQ_HANDLER)InterrputDefaultHandler);

	//IDTR 레지스터를 셋업한다
	IDTInstall();

	return true;
}


