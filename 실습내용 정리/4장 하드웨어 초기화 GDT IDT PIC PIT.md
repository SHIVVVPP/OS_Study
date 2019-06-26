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

  ```c++
/*  GDT.cpp */
  
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
  /*  IDT.cpp */
  #include "idt.h"
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
  		cli
  		pushad
  	}
  
  	// 레지스터를 복원하고 원래 수행하던 곳으로 돌아간다.
  	_asm {
  		mov al, 0x20
  		out 0x20, al
  		popad
  		sti
  		iretd
  	}
  }
  
  //i번째 인터럽트 디스크립트를 얻어온다.
  idt_descriptor* GetInterruptDescriptor(uint32_t i) {
  
  	if (i>I86_MAX_INTERRUPTS)
  		return 0;
  
  	return &_idt[i];
  }
  
  //인터럽트 핸들러 설치
  bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {
  
  	if (i>I86_MAX_INTERRUPTS)
  		return false;
  
  	if (!irq)
  		return false;
  
  	//인터럽트의 베이스 주소를 얻어온다.
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
  
  - **PIC.h PIC.cpp**
  
  ```c++
  /*  PIC.h */
  // 8259 Programmable Interrupt Contoroller
  #pragma once
  #include <stdint.h>
  
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
  ```
  
  ```c++
  /* PIC.cpp */
  #ifndef ARCH_X86
  #error "[PIC.cpp for i86] requires i86 architecture. Define ARCH_X86"
  #endif
  
  #include "PIC.h"
  #include "HAL.h"
  
  //http://www.eeeguide.com/programming-8259/
  //-----------------------------------------------
  //	Controller Registers
  //-----------------------------------------------
  
  //! PIC 1 register port addresses
  #define I86_PIC1_REG_COMMAND	0x20
  #define I86_PIC1_REG_STATUS		0x20
  #define I86_PIC1_REG_DATA		0x21
  #define I86_PIC1_REG_IMR		0x21
  
  //! PIC 2 register port addresses
  #define I86_PIC2_REG_COMMAND	0xA0
  #define I86_PIC2_REG_STATUS		0xA0
  #define I86_PIC2_REG_DATA		0xA1
  #define I86_PIC2_REG_IMR		0xA1
  
  //-----------------------------------------------
  //	Initialization Command Bit Masks
  //-----------------------------------------------
  
  //! Initialization Control Word 1 bit masks
  #define I86_PIC_ICW1_MASK_IC4			0x1			//00000001
  #define I86_PIC_ICW1_MASK_SNGL			0x2			//00000010
  #define I86_PIC_ICW1_MASK_ADI			0x4			//00000100
  #define I86_PIC_ICW1_MASK_LTIM			0x8			//00001000
  #define I86_PIC_ICW1_MASK_INIT			0x10		//00010000
  
  //! Initialization Control Words 2 and 3 do not require bit masks
  
  //! Initialization Control Word 4 bit masks
  #define I86_PIC_ICW4_MASK_UPM			0x1			//00000001
  #define I86_PIC_ICW4_MASK_AEOI			0x2			//00000010
  #define I86_PIC_ICW4_MASK_MS			0x4			//00000100
  #define I86_PIC_ICW4_MASK_BUF			0x8			//00001000
  #define I86_PIC_ICW4_MASK_SFNM			0x10		//00010000
  
  //-----------------------------------------------
  //	Initialization Command 1 control bits
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
  //	Initialization Command 4 control bits
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
  
  
  
  
  void PICInitialize(uint8_t base0, uint8_t base1)
  {
  	/*
  	IRQ 하드웨어 인터럽트가 발생할 때 적절히 작동하도록 하기 위해 PIC가 가진 IRQ를 초기화 해주어야 한다.
  	이를 위해 마스터 PIC의 명령 레지스터로 명령을 전달해야 하는데 이때 ICW(Initialization Control Word)가 사용된다.
  	이 ICW는 4가지의 초기화 명령어로 구성된다.
  	*/
  	uint8_t	icw = 0;
  
  	// PIC 초기화 ICW1 명령을 보낸다.
  	icw = (icw & ~I86_PIC_ICW1_MASK_INIT) | I86_PIC_ICW1_INIT_YES;
  	icw = (icw & ~I86_PIC_ICW1_MASK_IC4) | I86_PIC_ICW1_IC4_EXPECT;
  
  	SendCommandToPIC(icw, 0);
  	SendCommandToPIC(icw, 1);
  
  	//! PIC에 ICW2 명령을 보낸다. base0와 base1은 IRQ의 베이스 주소를 의미한다.
  	SendDataToPIC(base0, 0);
  	SendDataToPIC(base1, 1);
  
  	//PIC에 ICW3 명령을 보낸다. 마스터와 슬레이브 PIC와의 관계를 정립한다.
  
  	SendDataToPIC(0x04, 0);
  	SendDataToPIC(0x02, 1);
  
  	//ICW4 명령을 보낸다. i86 모드를 활성화 한다.
  
  	icw = (icw & ~I86_PIC_ICW4_MASK_UPM) | I86_PIC_ICW4_UPM_86MODE;
  
  	SendDataToPIC(icw, 0);
  	SendDataToPIC(icw, 1);
  	//PIC 초기화 완료
  
  }
  
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
  
  - **HAL.h HAL.cpp**
  
  ```c++
  /* HAL.h */
  .vs/ProjectSettings.json
  .vs/VSWorkspaceState.json
  *.suo
  /.vs/
  *.bin
  .vs/
  *.ipch
  *.tlog
  *.pdb
  *.obj
  *.zip
  
  /Test/CommonLib/*
  /Test/Include/*
  /Test/OSProject/CommonLib/*
  /Test/OSProject/Debug/*
  /Test/OSProject/.vs/*
  /Test/OSProject/SkyOS32/Intermediate/*
  *.pptx
  ```
  
  

