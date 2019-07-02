#pragma once
#include <stdint.h>

namespace PageTableEntry
{
	//PageTableEntry ����
	//������ ���� �ּ�[31:12](20bit) Available[11:9](3bit) opation[8:0](9bit) �̹Ƿ�
	typedef uint32_t PTE;

	enum PAGE_PTE_FLAGS
	{
		
		I86_PTE_PRESENT = 1,			//00000000  00000000 00000000 00000001 �������� �Ҵ� �Ǿ��°�? 
		I86_PTE_WRITABLE = 2,			//00000000  00000000 00000000 00000010 ���� �����Ѱ�?
		I86_PTE_USER = 4,				//00000000  00000000 00000000 00000100
		I86_PTE_WRITETHOUGH = 8,		//00000000  00000000 00000000 00001000
		I86_PTE_NOT_CACHEABLE = 0x10,	//00000000  00000000 00000000 00010000
		I86_PTE_ACCESSED = 0x20,		//00000000  00000000 00000000 00100000
		I86_PTE_DIRTY = 0x40,			//00000000  00000000 00000000 01000000
		I86_PTE_PAT = 0x80,				//00000000  00000000 00000000 10000000
		I86_PTE_CPU_GLOBAL = 0x100,		//00000000  00000000 00000001 00000000
		I86_PTE_LV4_GLOBAL = 0x200,		//00000000  00000000 00000010 00000000
		I86_PTE_FRAME = 0x7FFFF000 		//01111111  11111111 11110000 00000000
	};

	void AddAttribute(PTE* entry, uint32_t attr);
	void DelAttribute(PTE* entry, uint32_t attr);
	void SetFrame(PTE* entry, uint32_t addr);
	bool IsPresent(PTE entry);
	bool IsWritable(PTE entry);
	uint32_t GetFrame(PTE entry);
}