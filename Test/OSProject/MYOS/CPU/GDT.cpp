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
