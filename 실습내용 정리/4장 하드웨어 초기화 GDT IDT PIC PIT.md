# 4장 하드웨어 초기화 GDT IDT PIC  PIT

![Image](https://i.imgur.com/oCtWyvK.png)

## CPU

### 1. GDT (*Global Descriptor Table*)

#### GDT.h

```c++
#pragma once
#include <stdint.h>

/*
GDT(Global Descriptor Table)이란
연속된 디스크립터의 집합.
즉, OS에서 사용하는 코드 세그먼트 디스크립터와 데이터 세그먼트 디스크립터를 연속된 어셈블리어 코드로 나타낸 것
다만, NULL 디스크립터를 가장 앞부분에 추가해야 한다.
*/



// 최대 디스크립터 수 10
#define MAX_DESCRIPTORS 10

/*** GDT 디스크립터 접근 bit flags		***/


//0. ACCESS 엑세스 비트로, 스레드가 이 세그먼트에 접근했을 때, 1로 설정된다. 한번 설정된 이후에는 클리어 되지 않는다.
#define I86_GDT_DESC_ACCESS	0x0001  // 00000001
//1.READWIRTE 설정되었을 경우 디스크립터는 읽고 쓰기가 가능하다.
#define I86_GDT_DESC_READWRITE 0x0002 // 00000010
//2.EXPANSION 확장비트로 설정
#define I86_GDT_DESC_EXPANSION 0x0004 // 00000100
//3.EXEC_CODE 기본은 데이터 세그먼트이며, 설정했을 경우 코드세그먼트를 의미하게 된다.
#define I86_GDT_DESC_EXE_CODE 0x0008 // 00001000
//4.CODEDATA 시스템에 의해 정의된 세그먼트일 경우 0, 코드 또는 데이터 세그먼트일 경우 1
#define I86_GDT_DESC_CODEDATA 0x0010 // 00010000
//5.DPL 2bit 플래그로, DPL 특권 레벨을 나타낸다.
#define I86_GDT_DESC_DPL	0x0060	// 01100000
//6.MEMORY 해당 세그먼트는 물리 메모리에 올라와 있어 접근 가능함을 의미한다. 0일 경우 해당 세그먼트 디스크립터가 가리키는 메모리는 접근할 수 없다.
#define I86_GDT_DESC_MEMORY	0x0080	// 10000000

/***	GDT Descriptor Granularity(세분) bit Flag	***/

// 1. LIMITHI_MASK
//	이 4bit 값과 Segment Limit 값 16bit를 합쳐서 20bit를 나타낸다.
//	G 비트가 설정되어 있으며 20bit로 4GB 주소공간 표현이 가능하다. 
#define I86_GDT_GRAND_LIMITHI_MASK	0x0f // 00001111
// 2. OS
//	세그먼트가 64bit 네이티브 코드를 포함하고 있는지를 의미하며,
//	이 비트가 설정되면 D/B 필드는 0으로 설정되어야 한다.
#define I86_GDT_GRAND_OS	0x10	// 00010000
//3. 32BIT
//	32bit 모드일 때 지정된다.
#define I86_GDT_GRAND_32BIT	0x40	// 01000000
//4. 4K
//	이 플래그가 1이면 세그먼트 단위는 4k(2^2 * 2^10 = 2^12)(12bit)가 된다.
#define I86_GDT_GRAND_4K	0x80	// 10000000

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

/*
GDT 다시 설명
	GDT(Global Descriptor Table)는 CPU의 보호 모드 기능을 사용하기 위해
	운영체제 개발자가 작성해야 하는 테이블이다.

	// 일단 운영체제가 보호모드에 진입하면 물리 메모리 주소에 직접 데이터를 읽거나 쓰는 행위가 불가능하다.
	// 또한, 실행 코드도 메모리 주소를 직접 접근해서 가져오는 것이 아니라 GDT를 거쳐서 조건에 맞으면
	// 가져와 실행하며, 그렇지 않으면 예외를 일으킨다.

*/


/*
GDT Descriptor 구조체
	GDT 디스크립터 구조체 크기는 8byte이다.

	기준주소[31:24](baseHi 8bit) {G D/B L AVL 세그먼트크기[19:16]}(grand 8bit) {P DPL S 타입[11:8]}(flag 8bit) 기준주소(23:16)[7:0](baseMid 8bit)
	기준주소(15:00)[31:16](baseLo 16bit) 세그먼트 크기[15:0](limit 16bit)



	baseLow, baseMiddle, baseHigh 필드를 통해서 베이스 주소를 얻어내고
	세그먼트의 크기는 segmentLimit 2Byte의 필드와 grand 4bit를 통해서 얻어낼 수 있다.

	// 여기서 세그먼트의 크기는 20bit가 되는데, 베이스 주소가 0이라고 하면 20bit로는
	// 4GB의 메모리 주소를 나타내기에 부족하다.
	// 하지만 세그먼트 크기 단위가 4k(12bit) 이므로
	// 20bit의 크기를 12bit 왼쪽으로 쉬프트 하여 32bit의 세그먼트 크기를 나타낼 수 있어 4GB 표현이 가능하다.
*/
struct gdt_descriptor {


	// 세그먼트 크기 [15:0](16bit)
	uint16_t		limit; 

	// 기준주소 [15:00](baseLo 16bit) [23:16](baseMid 8bit)
	uint16_t		baseLo;
	uint8_t			baseMid;

	//Access Flag {P DPL S Type[11:8]}(Type 8bit)
	uint8_t			flags;

	// Grand {G D/B L AVL 세그먼트크기[19:16]}(grand 8bit)
	uint8_t			grand;

	// 기준주소 [31:24](baseHi 16bit)
	uint8_t			baseHi;
};


/*
GDTR 레지스터 구조체
	세그먼트 디스크립터는 메모리상에 위치하는 자료구조의 일종으로 GDT에 모여있는데
	GDT를 만들었으면 GDT가 어디에 위치하는지 CPU에 알려주어야 한다.
	CPU는 GDTR 레지스터를 참조해서 GDT에 접근하므로 GDTR레지스터에 적절한 값을 설정해야 한다.
	어셈블리 명령중 lgdt 명령어가 GDTR 레지스터에 값을 설정한다.
*/
struct gdtr {

	//! Global Descriptor Table의 크기
	uint16_t		m_limit;

	//! Global Descriptor Table의 기준 주소
	uint32_t		m_base;
};

#ifdef _MSC_VER
#pragma pack (pop)
#endif

// GDT(Global Descriptor Table) 설정 함수
extern void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand);

//! returns descritor
extern gdt_descriptor* i86_gdt_get_descriptor (int i);
// GDT를 설정하고 GDTR 레지스터에 초기화 하는 함수
extern	int GDTInitialize();
```

#### GDT.cpp

```c++
#include "GDT.h"
#include "string.h"
#include "memory.h"
#include "windef.h"
#include "defines.h"

//! global descriptor table is an array of descriptors
static struct gdt_descriptor	_gdt [MAX_DESCRIPTORS];
//! gdtr data
static struct gdtr				_gdtr;

// install gdtr
// gdtr을 불러온다. To load the GDTR, the instruction LGDT is used: 
static void InstallGDT () {
#ifdef _MSC_VER
	_asm lgdt [_gdtr]
#endif
}

//! Setup a descriptor in the Global Descriptor Table
void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand)
{
	if (i > MAX_DESCRIPTORS)
		return;

	//! null out the descriptor
	memset ((void*)&_gdt[i], 0, sizeof (gdt_descriptor));

	//! set limit and base addresses
	_gdt[i].baseLo	= uint16_t(base & 0xffff);
	_gdt[i].baseMid	= uint8_t((base >> 16) & 0xff);
	_gdt[i].baseHi	= uint8_t((base >> 24) & 0xff);
	_gdt[i].limit	= uint16_t(limit & 0xffff);

	//! set flags and grandularity bytes
	_gdt[i].flags = access;
	_gdt[i].grand = uint8_t((limit >> 16) & 0x0f);
	_gdt[i].grand |= grand & 0xf0;
}


//! returns descriptor in gdt
gdt_descriptor* i86_gdt_get_descriptor (int i) {

	if (i > MAX_DESCRIPTORS)
		return 0;

	return &_gdt[i];
}

//GDT 초기화 및 GDTR 레지스터에 GDT 로드
int GDTInitialize()
{
	//GDTR 레지스터에 로드될 _gdtr 구조체의 값 초기화
	//_gdtr 구조체의 주소는 페이징 전단계이며 실제 물리주소에 해당 변수가 할당되어 있다.
	//디스크립터의 수를 나타내는 MAX_DESCRIPTORS의 값은 5이다.
	//NULL 디스크립터, 커널 코드 디스크립터, 커널 데이터 디스크립터, 유저 코드 디스크립터
	//유저 데이터 디스크립터 이렇게 총 5개이다.
	//디스크립터당 8바이트이므로 GDT의 크기는 40바이트다.
	_gdtr.m_limit = (sizeof(struct gdt_descriptor) * MAX_DESCRIPTORS) - 1;
	_gdtr.m_base = (uint32_t)&_gdt[0];

	//NULL 디스크립터의 설정
	gdt_set_descriptor(0, 0, 0, 0, 0);

	//커널 코드 디스크립터의 설정
	gdt_set_descriptor(1, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	//커널 데이터 디스크립터의 설정
	gdt_set_descriptor(2, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

	//유저모드 디스크립터의 설정
	gdt_set_descriptor(3, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	//유저모드 데이터 디스크립터의 설정
	gdt_set_descriptor(4, 0, 0xffffffff, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);


	/*
	디스크립터는 모드 코드나 세그먼트이며 I86_GDT_DESC_CODEDATA
	메모리 상에 세그먼트가 존재하고 I86_GDT,DESC_MEMORY
	세그먼트 크기는 20bit이지만 4GB 주소 접근이 가능하며 I86_GDT_GRAND_4K,G플래그 활성화(I86_GDT_GRAND_LIMITHI_MASK)
	세그먼트는 32bit 코드를 담고 있다. I86_GDT_GRAND_32BIT
	코드 세그먼트일 경우에만 코드 수행이 가능하도록 I86_GDT_DESC_EXEC_CODE 플래그를 설정한다.

	디스크립터 값을 설정하는 부분(2, 3번째 매개변수)을 보면
	모든 디스크립터의 세그먼트 베이스 주소는 0이며, 세그먼트 크기는  4GB(0xffffffff)로 설정했다.
	*/

	//GDTR 레지스터에 GDT 로드
	InstallGDT();

	return 0;
}

```



<hr>

### 2. IDT (*Interrupt Descriptor Table*)

#### IDT.h

```c++
#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>

//Interrupt Descriptor Table : IDT는 인터럽트를 다루는 인터페이스를 제공할 책임이 있다.
//인터페이스 설치, 요청, 그리고 인터럽트를 처리하는 콜백 루틴

//인터럽트 핸들러의 최대 개수 : 256
#define I86_MAX_INTERRUPTS		256

// idt_descriptor 구조체의 flags 설정 define {P(1) DPL(2) 0(1) Type(4)} 8bit 
#define I86_IDT_DESC_BIT16		0x06	//00000110 Type(^000)은 D로 게이트의 사이즈를 뜻함 1-32bit 0 16bit
#define I86_IDT_DESC_BIT32		0x0E	//00001110	   (0^^^)은 게이트 종류 트랩게이트(111) 인터럽트게이트(110)
#define I86_IDT_DESC_RING1		0x40	//01000000 RING 특권레벨 
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

	// base = 핸들러 오프셋
	// 인터럽트 또는 예외 핸들러의 엔트리 포인트

	//인터럽트 핸들러 주소의 0-16 비트[0:15]
	uint16_t		baseLo;

	//GDT의 코드 셀렉터[16:31]
	//인터럽트 또는 예외 핸들러 수행 시 사용할 코드 세그먼트 디스크립터
	uint16_t		sel;

	//예약된 값 0이어야 한다.{00000 IST[2:0]}
	// IST 인터럽트나 예외 발생 시, 사용할 스택 어드레스(Top)
	// 0이 아닌 값으로 설정하면 인터럽트 발생 시 강제로 스택을 교환한다.
	uint8_t			reserved;

	//8비트 비트 플래그 {P DPL[14:13] 0 Type[11:8]}
	// Type (IDT 게이트의 종류)
	// 인터럽트 게이트(0110) 트랩 게이트(0111)
	// 인터럽트 게이트로 설정하면 핸들러를 수행하는 동안 인터럽트가 발생하지 못하며,
	// 트랩 게이트로 설정시 다른 인터럽트 발생가능
	// DPL(Descriptor Privilege Level) 해당 디스크립터를 사용하는 데 필요한 권한 (0~3의 값)
	// P(Present) 현재 디스크립터가 유효한 디스크립터인지 표시 유효(1) 유효X(0)
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

```

#### IDT.cpp

```c++
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
```

<hr>

### 3. PIC(*Programmable Interrupt Controller*)

#### PIC.h

```c++
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


```

#### PIC.cpp

```c++
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


```

<hr>
### 4. HAL(*Hardware Abstraction Layer*) 

**공통적으로 사용될 만한 것들을 모아둠**

#### HAL.h

```c++
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
```

#### HAL.cpp

```c++
#include "HAL.h"
#include "IDT.h"

void OutPortByte(ushort port, uchar value)
{
	_outp(port, value);
}

void OutPortWord(ushort port, ushort value)
{
	_outpw(port, value);
}

void OutPortDWord(ushort port, unsigned int value)
{
	_outpd(port, value);
}

long InPortDWord(unsigned int port)
{
	return _inpd(port);
}

uchar InPortByte(ushort port)
{
#ifdef _MSC_VER
	_asm {
		mov		dx, word ptr[port]
		in		al, dx
		mov		byte ptr[port], al
	}
#endif
	return (unsigned char)port;
}

ushort InPortWord(ushort port)
{
	return _inpw(port);
}
```

<hr>

### 5. PIT (*Programmable Interval Timer*)

#### PIT.h

```c++
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
```

#### PIT.cpp

```c++
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

```

#### IDT에 SendEOI 추가

```assembly
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
```

#### HAL에 setvect (인터럽트핸들러 설치) 함수 추가

```c++
/*  HAL.h */
void setvect(int intno, void(&vect)(), int flags = 0);

/* HAL.cpp */
// 인터럽트 서비스 프로시저
void setvect(int intno, void(&vect)(), int flags)
{
	InstallInterrputHandler(intno, (uint16_t)(I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32 | flags), 0x8, vect);
}

```



<hr>

### 6. KMain에 하드웨어 초기화 및 타이머 테스트 추가

#### kMain.h 에 include 추가

```c++
#include "GDT.h"
#include "IDT.h"
#include "PIC.h"
#include "PIT.h"
```

#### kMain.cpp 수정

```c++
/* 기존에 있던 것들 중 multiboot_entry 를 제외하고 수정 */

void HardwareInitialize();

// 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(->addr), 매직넘버(->magic)로 호출
void kmain(unsigned long magic, unsigned long addr)
{
	SkyConsole::Initialize();

	SkyConsole::Print("*** MY OS Console System Init ***\n");

	// 하드웨어 초기화 과정중 인터럽트가 발생하지 않아야 하므로
	HardwareInitialize();
	SkyConsole::Print("Hardware Init Complete\n");
	

	//타이머를 시작한다.
	StartPITCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);
}

// 하드웨어 초기화
void HardwareInitialize()
{
	GDTInitialize();
	IDTInitialize(0x8);
	PICInitialize(0x20, 0x28);
	InitializePIT();
}
```

이 상태로 시작하면 HardwareInitialize() 중에 계속 인터럽트가 발생해 출력이 되지 않는다. 

> 따라서 임계역역으로 설정해주어서 HardwareInitialize()가 간섭받지 않도록 해준다.

#### SkyAPI 를 생성해 임계영역에 들어가는 함수와 나오는 함수 설정 

#### SkyAPI.h

```c++
#pragma once
#include "windef.h"
#include "SkyStruct.h"
#include "Hal.h"
#include "PIT.h"
#include "time.h"

/////////////////////////////////////////////////////
//						동기화
/////////////////////////////////////////////////////
typedef struct _CRITICAL_SECTION
{
	LONG LockRecursionCount;
	HANDLE OwningThread;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

extern CRITICAL_SECTION g_criticalSection;

void SKYAPI kEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void SKYAPI kInitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void SKYAPI kLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
```

#### SkyAPI.cpp

```c++
#include "SkyAPI.h"
#include "SkyConsole.h"
#include "Hal.h"
#include "string.h"
#include "va_list.h"
#include "stdarg.h"
#include "sprintf.h"

CRITICAL_SECTION g_criticalSection;

void SKYAPI kInitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	lpCriticalSection->LockRecursionCount = 0;
	lpCriticalSection->OwningThread = 0;
}

void SKYAPI kEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	if (lpCriticalSection->LockRecursionCount == 0)
	{
		_asm cli // clear interrupt 인터럽트 플래그를 0(interrupt disable)로 설정
	}

	lpCriticalSection->LockRecursionCount++;

}

void SKYAPI kLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	lpCriticalSection->LockRecursionCount--;

	{
		_asm sti // set interrupt 인터럽트 플래그를 1(interrupt enable)로 설정
	}
}
```



#### kMain.cpp의 HardwareInitialize() 앞 뒤에 임계역역 설정

```c++
/*  kMain.cpp  */
...
// 하드웨어 초기화 과정중 인터럽트가 발생하지 않아야 하므로
	kEnterCriticalSection(&g_criticalSection);

	HardwareInitialize();
	SkyConsole::Print("Hardware Init Complete\n");
	
	kLeaveCriticalSection(&g_criticalSection);
...
```



### 7. 이제 실행하면 타이머 카운트가 제대로 동작한다.

![image](https://user-images.githubusercontent.com/34773827/60170040-c44aa300-9842-11e9-83a9-c2549b400724.png)

