# 4장 하드웨어 초기화 GDT IDT PIC  PIT

![Image](https://i.imgur.com/oCtWyvK.png)

- **CPU 폴더 추가**

  - **GDT.h GDT.cpp**

  ```c++
  /*  GDT.h */
  #pragma once
  #include <stdint.h>
  #include "string.h"
  #include "memory.h"
  #include "windef.h"
  #include "defines.h"
  
  // 최대 디스크립터 수 10
  #define MAX_DESCRIPTORS 10
  
  /*** GDT 디스크립터 접근 bit flags		***/
  
  /*
  1. ACCESS
  	엑세스 비트로, 스레드가 이 세그먼트에 접근했을 때, 1로 설정된다.
  	한번 설정된 이후에는 클리어 되지 않는다.
  */
  #define I86_GDT_DESC_ACCESS	0x0001  // 00000001
  
  /*
  2.READWIRTE
  	설정되었을 경우 디스크립터는 읽고 쓰기가 가능하다.
  */
  #define I86_GDT_DESC_READWRITE 0x0002 // 00000010
  
  /*
  3.EXPANSION
  	확장비트로 설정
  */
  #define I86_GDT_DESC_EXPANSION 0x0004 // 00000100
  
  /*
  4.EXEC_CODE
  	기본은 데이터 세그먼트이며,
  	설정했을 경우 코드세그먼트를 의미하게 된다.
  */
  #define I86_GDT_DESC_EXE_CODE 0x0008 // 00001000
  
  /*
  5.CODEDATA
  	시스템에 의해 정의된 세그먼트일 경우 0,
  	코드 또는 데이터 세그먼트일 경우 1
  */
  #define I86_GDT_DESC_CODEDATA 0x0010 // 00010000
  
  /*
  6.DPL
  	2bit 플래그로, DPL 특권 레벨을 나타낸다.
  */
  #define I86_GDT_DESC_DPL	0x0060	// 01100000
  
  /*
  7.MEMORY
  	해당 세그먼트는 물리 메모리에 올라와 있어 접근 가능함을 의미한다.
  	0일 경우 해당 세그먼트 디스크립터가 가리키는 메모리는 접근할 수 없다.
  */
  #define I86_GDT_DESC_MEMORY	0x0080	// 10000000
  
  /***		GDT Descriptor Granularity(세분) bit Flag***/
  
  /*
  1. LIMITHI_MASK
  	이 4bit 값과 Segment Limit 값 16bit를 합쳐서 20bit를 나타낸다.
  	G 비트가 설정되어 있으며 20bit로 4GB 주소공간 표현이 가능하다.
  */
  #define I86_GDT_GRAND_LIMITHI_MASK	0x0f // 00001111
  
  /*
  2. OS
  세그먼트가 64bit 네이티브 코드를 포함하고 있는지를 의미하며,
  이 비트가 설정되면 D/B 필드는 0으로 설정되어야 한다.
  */
  #define I86_GDT_GRAND_OS	0x10	// 00010000
  
  
  /*
  3. 32BIT
  	32bit 모드일 때 지정된다.
  */
  #define I86_GDT_GRAND_32BIT	0x40	// 01000000
  
  /*
  4. 4K
  	이 플래그가 1이면 세그먼트 단위는 4k(2^2 * 2^10 = 2^12)(12bit)가 된다.
  */
  #define I86_GDT_GRAND_4K	0x80	// 10000000
  
  //============================================================================
  //    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
  //============================================================================
  //============================================================================
  //    INTERFACE STRUCTURES / UTILITY CLASSES
  //============================================================================
  
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
  
  	baseLow, baseMiddle, baseHigh 필드를 통해서 베이스 주소를 얻어내고
  	세그먼트의 크기는 segmentLimit 2Byte의 필드와 grand 4bit를 통해서 얻어낼 수 있다.
  
  // 여기서 세그먼트의 크기는 20bit가 되는데, 베이스 주소가 0이라고 하면 20bit로는
  // 4GB의 메모리 주소를 나타내기에 부족하다. (32bit가 필요하다.)
  // 하지만 세그먼트 크기 단위가 4k(12bit) 이므로
  // 20bit의 크기를 12bit 왼쪽으로 쉬프트 하여 32bit의 세그먼트 크기를 나타낼 수 있어 4GB 표현이 가능하다.
  */
  #ifdef _MSC_VER
  #pragma pack(push,1)
  #endif
  struct gdt_descriptor
  {
  	// bits 0 - 15 of segment limit
  	USHORT	limit;
  
  	// bits 0 - 23 of base address
  	USHORT	baseLo;
  	BYTE		baseMid;
  
  	// descriptor access flag
  	BYTE		flags;
  
  	BYTE		grand;
  
  	// bits 24 - 32 of base address
  	BYTE		baseHi;
  };
  
  
  /*
  GDTR 레지스터 구조체
  	GDT를 만들었으면 GDT가 어디에 위치하는지 CPU에 알려주어야 한다.
  	CPU는 GDTR 레지스터를 참조해서 GDT에 접근하므로 GDTR레지스터에 적절한 값을 설정해야 한다.
  	어셈블리 명령중 lgdt 명령어가 GDTR 레지스터에 값을 설정한다.
  */
  typedef struct tag_gdtr
  {
  	USHORT m_limit; // GDT 크기
  	UINT m_base;	// GDT 시작주소
  }gdtr;
  
  #ifdef _MSC_VER
  #pragma pack(pop)
  #endif
  
  // GDT(Global Descriptor Table) 설정 함수
  extern void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint64_t acess, uint8_t grand);
  
  // Return Descriptor
  extern gdt_descriptor* i86_gdt_get_descriptor(int i);
  
  // GDT를 설정하고 GDTR 레지스터에 초기화 하는 함수
  extern int GDTInitialize();
  ```

  - IDT

  ```c++
  /*  IDT.h */
  #ifndef _IDT_H
  #define _IDT_H
  #include <stdint.h>
  #include "windef.h"
  
  // Interrupt Descriptor Table : IDT는 인터럽트를 다루는 인터페이스를 제공한다.
  // 인터페이스 설치, 요청, 인터럽트 처리 콜백 루틴
  
  //인터럽트 핸들러의 최대 개수 : 256
  #define I86_MAX_INTERRUPTS 256	
   
  #define I86_IDT_DESC_BIT16		0x06	//00000110
  #define I86_IDT_DESC_BIT32		0x0E	//00001110
  #define I86_IDT_DESC_RING1		0x40	//01000000
  #define I86_IDT_DESC_RING2		0x20	//00100000
  #define I86_IDT_DESC_RING3		0x60	//01100000
  #define I86_IDT_DESC_PRESENT	0x80	//10000000
  
  // 인터럽트 핸들러 정의 함수
  // 인터럽트 핸들러는 프로세서가 호출한다. 이대 스택 구성이 변하는데
  // 인터럽트를 처리하고 리턴할 때 스택이 인터럽트 핸들러 호출 직전의 구성과 똑같음을 보장해야한다.
  // _cdecl 스택 정리 책임 - 호출자  인자 전달 순서 - 오른쪽->왼쪽
  typedef void(_cdecl* I86_IRQ_HANDLER)(void);
  
  #ifdef _MSC_VER
  #pragma pack(push,1)
  #endif
  
  /*
  IDT를 구축하고 난뒤 IDT의 위치를 CPU에 알려주기 위한 IDTR 구조체를 만들어야 한다.
  CPU는 IDTR 레지스터를 통해서 IDT에 접근한다.
  */
  struct idtr
  {
  	//IDT 크기
  	uint16_t limit;
  
  	//IDT 베이스 주소
  	uint32_t base;
  };
  
  /*
  인터럽트 디스크립터의 크기는 8바이트이다.
  이 8바이트 중에서 2바이트 크기를 차지하는 세그먼트 셀렉터를 통해서
  GDT 내의 글로벌 디스크립터를 찾을 수 있고 이 글로벌 디스크립터를 통해서 세그먼트의 베이스 주소를 얻을 수 있다.
  세그먼트 베이스 주소에서 오프셋 값을 더하면 ISR(인터럽트 서비스 루틴)의 주소를 얻을 수 있다.
  offsetLow, offsetHigh 필드로 ISR의 4바이트 오프셋 값을 얻을 수 있다.
  selector 필드로 GDT에서 디스크립터를 찾아 세그먼트의 베이스 주소를 얻을 수 있다.
  */
  typedef struct tag_idt_descriptor
  {
  	USHORT offsetLow;	// 인터럽트 핸들러 주소의 0 - 15 bit
  	USHORT selector;	// GDT의 코드 셀렉터
  	BYTE reserved;		// 예약된 값 0이어야 한다.
  	BYTE flags;			// 8비트 비트 플래그
  	USHORT offsetHigh;	// 인터럽트 핸들러 주소의 16 - 31 bit
  }idt_descriptor;
  
  #ifdef _MSC_VER
  #pragma pack(pop)
  #endif
  
  // i번째 인터럽트 디스크립트를 얻어온다.
  extern idt_descriptor* GetInterruptDescriptor(uint32_t i);
  
  // 인터럽트 핸들러 설치
  extern bool InstallInterruptHandler(uint32_t i, uint32_t flags, uint16_t sel, I86_IRQ_HANDLER);
  extern bool IDTInitialize(uint16_t codeSel);
  
  #endif
  ```

  ```c++
  /* IDT.cpp */
  #include "idt.h"
  #include "string.h"
  #include "memory.h"
  #include <hal.h>
  //#include "SkyAPI.h"
  
  //인터럽트 디스크립터 테이블
  // 총 256개의 인터럽트 디스크립터가 존재하며 처음 디스크립터는 항상 NULL로 설정한다.
  static idt_descriptor	_idt[I86_MAX_INTERRUPTS];
  
  //CPU의 IDTR 레지스터를 초기화하는데 도움을 주는 IDTR 구조체
  //IDT의 메모리 주소 및 IDT의 크기를 담고 있다.
  static struct idtr				_idtr;
  
  // lidt를 통해 CPU에 IDT 위치를 알려준다.
  //IDTR 레지스터에 IDT의 주소값을 넣는다.
  static void IDTInstall() {
  #ifdef _MSC_VER
  	_asm lidt[_idtr]
  #endif
  }
  
  
  #pragma region Interrupt Handler
  
  #define DMA_PICU1       0x0020
  #define DMA_PICU2       0x00A0
  
  __declspec(naked) void SendEOI()
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
  __declspec(naked) void InterruptDefaultHandler() {
  
  	//레지스터를 저장하고 인터럽트를 끈다.
  	_asm
  	{
  		PUSHAD
  		PUSHFD
  		CLI
  	}
  
  	SendEOI();
  
  	// 레지스터를 복원하고 원래 수행하던 곳으로 돌아간다.
  	_asm
  	{
  		POPFD
  		POPAD
  		IRETD
  	}
  }
  
  #pragma endregion
  
  
  
  //i번째 인터럽트 디스크립트를 얻어온다.
  idt_descriptor* GetInterruptDescriptor(uint32_t i) {
  
  	if (i > I86_MAX_INTERRUPTS)
  		return 0;
  
  	return &_idt[i];
  }
  
  // 인터럽트 서비스 루틴을 설치한다.
  bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {
  
  	if (i > I86_MAX_INTERRUPTS)
  		return false;
  
  	if (!irq)
  		return false;
  
  	//인터럽트의 베이스 주소를 얻어온다.
  	uint64_t		uiBase = (uint64_t) & (*irq);
  
  	if ((flags & 0x0500) == 0x0500) {
  		_idt[i].selector = sel;
  		_idt[i].flags = uint8_t(flags);
  	}
  	else
  	{
  		//포맷에 맞게 인터럽트 핸들러와 플래그 값을 디스크립터에 세팅한다.
  		_idt[i].offsetLow = uint16_t(uiBase & 0xffff);
  		_idt[i].offsetHigh = uint16_t((uiBase >> 16) & 0xffff);
  		_idt[i].reserved = 0;
  		_idt[i].flags = uint8_t(flags);
  		_idt[i].selector = sel;
  	}
  
  	return	true;
  }
  
  //IDT를 초기화하고 디폴트 핸들러를 등록한다
  bool IDTInitialize(uint16_t codeSel) {
  
  	//IDTR 레지스터에 로드될 구조체 초기화
  	_idtr.limit = sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1;
  	_idtr.base = (uint32_t)& _idt[0];
  
  	//NULL 디스크립터
  	memset((void*)& _idt[0], 0, sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1);
  
  	//디폴트 핸들러 등록
  	for (int i = 0; i < I86_MAX_INTERRUPTS; i++)
  		InstallInterrputHandler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32,
  			codeSel, (I86_IRQ_HANDLER)InterruptDefaultHandler);
  
  	//IDTR 레지스터를 셋업한다
  	IDTInstall();
  
  	return true;
  }
  ```

  

