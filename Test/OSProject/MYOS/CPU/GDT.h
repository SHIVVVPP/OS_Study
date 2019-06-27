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