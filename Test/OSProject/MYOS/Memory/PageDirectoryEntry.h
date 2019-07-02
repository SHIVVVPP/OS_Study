#pragma once
#include <stdint.h>

namespace PageDirectoryEntry
{
	typedef uint32_t PDE;

	enum PAGE_PDE_FLAGS {

										//base address        ~|  |Attribute
		I86_PDE_PRESENT = 1,			//00000000 00000000 000 00 000 0000001
		I86_PDE_WRITABLE = 2,			//00000000 00000000 000 00 000 0000010
		I86_PDE_USER = 4,				//00000000 00000000 000 00 000 0000100
		I86_PDE_PWT = 8,				//00000000 00000000 000 00 000 0001000
		I86_PDE_PCD = 0x10,				//00000000 00000000 000 00 000 0010000
		I86_PDE_ACCESSED = 0x20,		//00000000 00000000 000 00 000 0100000
		I86_PDE_DIRTY = 0x40,			//00000000 00000000 000 00 000 1000000
		I86_PDE_4MB = 0x80,				//00000000 00000000 000 00 001 0000000
		I86_PDE_CPU_GLOBAL = 0x100,		//00000000 00000000 000 00 010 0000000
		I86_PDE_LV4_GLOBAL = 0x200,		//00000000 00000000 000 00 100 0000000
		I86_PDE_FRAME = 0x7FFFF000 		//01111111 11111111 111 00 000 0000000
	};

	void AddAttribute(PDE* entry, uint32_t attr);
	void DelAttribute(PDE* entry, uint32_t attr);
	void SetFrame(PDE* entry, uint32_t addr);
	bool IsPresent(PDE entry);
	bool IsWritable(PDE entry);
	uint32_t GetFrame(PDE entry);
	bool IsUser(PDE entry);
	bool Is4mb(PDE entry);
};