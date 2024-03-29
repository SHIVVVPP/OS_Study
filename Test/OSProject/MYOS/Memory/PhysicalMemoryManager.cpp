#include "CYNOS.h"
#include "Exception.h"

namespace PhysicalMemoryManager
{
//==========================================================
// LIKE PRIVATE MEMBER VARIABLE
//==========================================================
	uint32_t m_memorySize = 0;
	uint32_t m_usedBlocks = 0;

	// 이용할 수 있는 최대 블록 개수
	uint32_t m_maxBlocks = 0;

	// 비트맵 배열, 각 비트는 메모리 블럭을 표현한다.
	uint32_t*	m_pMemoryMap = 0; // 메모리 맵의 시작 주소
	uint32_t	m_memoryMapSize = 0;

	//memorySize : 전체 메모리의 크기 (바이트 사이즈)
	//bitmapAddr : 커널 다음에 배치되는 비트맵 배열
	uint32_t g_totalMemorySize = 0;

//==========================================================
// Getter & Setter
//==========================================================
#pragma region GETTER & SETTER

	uint32_t GetKernelEnd()			{ return (uint32_t)m_pMemoryMap;				}	
	uint32_t GetMemoryMapSize()		{ return m_memoryMapSize;						}
	size_t	 GetMemorySize()		{ return m_memorySize;							}

	uint32_t GetTotalBlockCount()	{ return m_maxBlocks;							}
	uint32_t GetUsedBlockCount()	{ return m_usedBlocks;							}
	uint32_t GetFreeBlockCount()	{ return m_maxBlocks - m_usedBlocks;			}

	uint32_t GetFreeMemory()		{ return GetFreeBlockCount() * PMM_MEMORY_PER_BLOCKS; }
	uint32_t GetBlockSize()			{ return PMM_MEMORY_PER_BLOCKS;						}
#pragma endregion



//==========================================================
// Variable Initialize from BootInfo
//==========================================================
#pragma region Variable Initialize from BootInfo

	uint32_t GetTotalMemory(multiboot_info* bootinfo)
	{
		uint64_t endAddress = 0;

		//	bootinfo->mmap_length
		//	mmap_버퍼는 multiboot_memory_map_t 구조체로 이루어져 있다.
		//	mmapEntryNum은 multiboot_memory_map 구조체 배열의 원소 개수이다.
		uint32_t mmapEntryNum = bootinfo->mmap_length / sizeof(multiboot_memory_map_t);
		// bootinfo->mmap_addr은 주소
		multiboot_mmap_entry* mmapAddr = (multiboot_mmap_entry*)bootinfo->mmap_addr;

#ifdef _CYN_DEBUG
		CYNConsole::Print("Memory Map Entry Num : %d\n", mmapEntryNum);
#endif

		for (uint32_t i = 0; i < mmapEntryNum; i++)
		{
			uint64_t areaStart = mmapAddr[i].addr;
			uint64_t areaEnd = areaStart + mmapAddr[i].len;

			//CYNConsole::Print("0x%q 0x%q\n", areaStart, areaEnd);

			if (mmapAddr[i].type != 1)
			{
				continue;
			}

			if (areaEnd > endAddress)
				endAddress = areaEnd;
		}

		if (endAddress > 0xFFFFFFFF) {
			endAddress = 0xFFFFFFFF;
		}

		return (uint32_t)endAddress;
	}

	uint32_t GetKernelEnd(multiboot_info* bootinfo)
	{
		// 커널 구역이 끝나는 이후부터 메모리 비트맵 구역을 설정한다.
		// GRUB로 PE 커널을 로드할 때는 커널 크기를 확인할 방법이 없다.
		// 따라서 마지막에 로드된 모듈이 끝나는 주소로 활용한다.
		// 즉, 커널의 크기는
		//		마지막에 로드된 모듈이 끝나는 주소 - 0x100000이 된다.
		// 순수 커널 자체의 크기는 최초로 로드되는 모듈이 시작되는 주소 - 0x100000이 될 것이다.
		// 커널의 크기를 구하기 위해서는 모듈을 기준으로 구하므로 최소 하나 이상은 로드 되어야 한다.
		uint64_t endAddress = 0;
		uint32_t mods_count = bootinfo->mods_count;   /* Get the amount of modules available */
		uint32_t mods_addr = (uint32_t)bootinfo->Modules;     /* And the starting address of the modules */
		for (uint32_t i = 0; i < mods_count; i++) {
			Module* module = (Module*)(mods_addr + (i * sizeof(Module)));     /* Loop through all modules */

			uint32_t moduleStart = PAGE_ALIGN_DOWN((uint32_t)module->ModuleStart); // HAL.h 에 PAGE_ALIGNE_DOWN, PAGE_ALIGN_UP 추가
			uint32_t moduleEnd = PAGE_ALIGN_UP((uint32_t)module->ModuleEnd);

			if (endAddress < moduleEnd)
			{
				endAddress = moduleEnd;
			}

			CYNConsole::Print("%x %x\n", moduleStart, moduleEnd);
		}

		return (uint32_t)endAddress;
	}

#pragma endregion


//==========================================================
// like PUBLIC Method
//==========================================================
	void Initialize(multiboot_info* bootinfo)
	{
		CYNConsole::Print("Physical Memory Manager Init..\n");
		// 초기 멤버 변수 세팅
		g_totalMemorySize = GetTotalMemory(bootinfo);
		// 사용된 블록 수 (400KB 메모리가 사용중이라면, 사용된 블록의 수는 100개 이다.)
		m_usedBlocks = 0;
		// 자유 메모리 크기 128MB(기본 설정으로 정의)
		m_memorySize = g_totalMemorySize;
		// 사용할 수 있는 최대 블록 수 : 128MB/ 4KB(qemu에서 128MB로 설정) (블록 하나당 4KB를 나타내므로)
		m_maxBlocks = m_memorySize / PMM_MEMORY_PER_BLOCKS;

		// 페이지의 수.
		// 하나의 페이지는 4KB
		// maxBlocks(블록의 총 수) / PMM_BLOCKS_PER_BYTE(블록 하나당 바이트) / 페이지 하나의 크기
		// 블록들이 차지하는 페이지 수 = 메모리 맵을 할당하기 위해 필요한 페이지 수
		int pageCount = m_maxBlocks / PMM_BLOCKS_PER_BYTE / PAGE_SIZE;
		if (pageCount == 0)
			pageCount = 1;

		m_pMemoryMap = (uint32_t*)GetKernelEnd(bootinfo);

		// 1MB = 1048576 byte
		CYNConsole::Print("Total Memory (%dMB)\n", g_totalMemorySize / 1048576);
		CYNConsole::Print("BitMap Start Address(0x%x)\n", m_pMemoryMap);
		CYNConsole::Print("Page Count (%d)\n", pageCount);
		CYNConsole::Print("BitMap Size(0x%x)\n", pageCount * PAGE_SIZE);

		//블럭들의 최대 수는 8의 배수로 맞추고 나머지는 버린다
		//m_maxBlocks = m_maxBlocks - (m_maxBlocks % PMM_BLOCKS_PER_BYTE);

		// 메모리맵의 바이트크기
		// m_memoryMapSize = 비트맵 배열의 크기, 이 크기가 4KB라면 128MB의 메모리를 관리할 수 있다.
		m_memoryMapSize = m_maxBlocks / PMM_BLOCKS_PER_BYTE;
		m_usedBlocks = GetTotalBlockCount();

		// 메모리 맵으 크기
		int tempMemoryMapSize = (GetMemoryMapSize() / 4096) * 4096;

		// 메모리 맵의 크기는 블록 단위로 정해져야 한다.
		// PMM_MEMORY_PER_BLOCK = 4096이기 때문에 4096 단위로 크기가 정해져야 한다.
		if (GetMemoryMapSize() % 4096> 0) 
			tempMemoryMapSize += 4096;

		m_memoryMapSize = tempMemoryMapSize;

		//모든 메모리 블럭들이 사용중에 있다고 설정한다.	
		unsigned char flag = 0xff;
		memset((char*)m_pMemoryMap, flag, m_memoryMapSize);
		//// 이용 가능한 메모리 블록을 설정한다.
		SetAvailableMemory((uint32_t)m_pMemoryMap, m_memorySize);
	}

	// 메모리 들의 사용 여부를 설정한다.
	void SetAvailableMemory(uint32_t base, size_t size)
	{
		// 여기서 메모리 맵이 메모리를 사용하고 있기 때문에
		// 메모리 맵에서 사용되는 블록 수 만큼의 메모리는  사용되고 있다고 처리한다.
		int usedBlock = GetMemoryMapSize() / PMM_MEMORY_PER_BLOCKS;
		int blocks = GetTotalBlockCount();

		// (KernelEndAddress 부터 사용되기 때문에 초기 i를 usedBlock으로 설정)
		for (int i = usedBlock; i < blocks; i++) 
		{
			UnsetBit(i);
			m_usedBlocks--;
		}
	}

	void SetDeAvailableMemory(uint32_t base, size_t size)
	{
		int align = base / PMM_MEMORY_PER_BLOCKS;
		int blocks = size / PMM_MEMORY_PER_BLOCKS;

		for (; blocks > 0; blocks--) 
		{
			SetBit(align++);
			m_usedBlocks++;
		}
	}

	/*
	8번째 메모리 블럭이 사용중임을 표시하기 위해 1로 세팅하려면
	배열 첫번째 요소 (4바이트) 바이트의 8번째 비트에 접근해야 한다.
	메모리 비트맵은 4바이트당 32개의 블록을 표현할 수 있으므로,
	1301 메모리 블록을 예로 들면
	1301 / 32 = 40, 1301 % 32 = 1
	즉, 인덱스 1301 블록이 사용되고 있음을 나타내기 위해
	m_pMemoryMap[40]에 접근한 뒤,
	4바이트 즉 32비트 중에서 두 번째 비트를 1로 세팅하면 1301 블록이 사용중임을 나타낼 수 있다.
	1 << (1301 % 32 = 21) => 00100000 00000000 00000000 00000000
	사용중이지 않음을 나타낼 때는
	~(1 << (1301 % 32 = 21)) => 11011111 11111111 11111111 11111111
	*/
	void SetBit(int bit)
	{
		m_pMemoryMap[bit / 32] |= (1 << (bit % 32));
	}

	void UnsetBit(int bit)
	{
		m_pMemoryMap[bit / 32] &= ~(1 << (bit % 32));
	}

	// 해당 비트가 세트되어 있는지 여부를 체크한다.
	// 비트가 유효값을 벗어나면 0을 리턴한다.
	bool TestMemoryMap(int bit)
	{
		return (m_pMemoryMap[bit / 32] & (1 << (bit % 32))) > 0;
	}

	

	

	//비트가 0인 프레임 인덱스를 얻어낸다.
	unsigned int GetFreeFrame()
	{
		for (uint32_t i = 0; i < GetTotalBlockCount() / 32; i++)
		{
			if (m_pMemoryMap[i] != 0xffffffff)		// i 번째 메모리 맵이 모두 사용중인지 체크
				for (unsigned int j = 0; j < PMM_BITS_PER_INDEX; j++)
				{
					unsigned int bit = 1 << j;	// 자리수 비트
					if ((m_pMemoryMap[i] & bit) == 0)	// 해당 자리가 사용중이 아니라면
						return i * PMM_BITS_PER_INDEX + j;	// 몇번째 블록인지 계산해 리턴한다. i * m_pMemoryMap 한 요소당 블록 수 + j
				}
		}

		return 0xffffffff;
	}
	   
	// 연속된 빈 프레임(블럭)들을 얻어낸다.
	unsigned int GetFreeFrames(size_t size)
	{
		if (size == 0)
			return 0xffffffff;

		if (size == 1)
			return GetFreeFrame();



		for (uint32_t i = 0; i < GetTotalBlockCount() / 32; i++)
		{
			if (m_pMemoryMap[i] != 0xffffffff)
			{
				for (unsigned int j = 0; j < 32; j++)
				{
					unsigned int bit = 1 << j;
					if ((m_pMemoryMap[i] & bit) == 0)
					{

						unsigned int startingBit = i * PMM_BITS_PER_INDEX + j;

						// 연속된 빈 프레임의 갯수를 증가시킨다.

						uint32_t free = 0;
						for (uint32_t count = 0; count < size; count++)
						{
							//메모리맵을 벗어나는 상황
							if (startingBit + count >= m_maxBlocks)
								return 0xffffffff;

							if (TestMemoryMap(startingBit + count) == false)
								free++;
							else
								break;

							//연속된 빈 프레임들이 존재한다. 시작 비트 인덱스는 startingBit
							if (free == size)
								return startingBit;
						}
					}
				}
			}
		}

		return 0xffffffff;
	}

	void* AllocBlock()
	{
		// 이용할 수 있는 블록이 없다면 할당 실패
		if (GetFreeBlockCount() <= 0)
			return NULL;
		// 사용되고 있지 않은 블록의 인덱스를 얻는다.
		unsigned int frame = GetFreeFrame();

		// GetFreeFrame에서 유효한 프레임이 없다면 -1을 리턴하므로 예외처리
		if (frame == -1)
			return NULL;

		// 메모리 비트맵에 해당 블록이 사용되고 있음을 세팅한다.
		SetBit(frame);

		// 할당된 물리 메모리 주소를 리턴한다.
		uint32_t addr = frame * PMM_MEMORY_PER_BLOCKS + (uint32_t)m_pMemoryMap;
		m_usedBlocks++;

		return (void*)addr;
	}

	void FreeBlock(void* p)
	{
		uint32_t addr = (uint32_t)p;
		int frame = addr / PMM_MEMORY_PER_BLOCKS;

		UnsetBit(frame);

		m_usedBlocks--;
	}


	void* AllocBlocks(size_t size)
	{
		
		if (GetFreeBlockCount() <= size)
		{
			return NULL;
		}


		int frame = GetFreeFrames(size);

		if (frame == -1)
		{
			return NULL;	//연속된 빈 블럭들이 존재하지 않는다.
		}

		for (uint32_t i = 0; i < size; i++)
			SetBit(frame + i);

		uint32_t addr = frame * PMM_MEMORY_PER_BLOCKS + (uint32_t)m_pMemoryMap;
		m_usedBlocks += size;

		return (void*)addr;
	}


	void FreeBlocks(void* p, size_t size)
	{
		uint32_t addr = (uint32_t)p - (uint32_t)m_pMemoryMap;
		int frame = addr / PMM_MEMORY_PER_BLOCKS;

		for (uint32_t i = 0; i < size; i++)
			UnsetBit(frame + i);

		m_usedBlocks -= size;
	}
	 
//==========================================================
// Control Register (PAGING & PDBR)
//==========================================================
	void EnablePaging(bool state)
	{
#ifdef _MSC_VER
		_asm
		{
			mov	eax, cr0
			cmp[state], 1
			je	enable
			jmp disable
			enable :
			or eax, 0x80000000		//set bit 31
			mov	cr0, eax
			jmp done
			disable :
			and eax, 0x7FFFFFFF		//clear bit 31
				mov	cr0, eax
				done :
		}
#endif
	}

	bool IsPaging()
	{
		uint32_t res = 0;

#ifdef _MSC_VER
		_asm {
			mov	eax, cr0
			mov[res], eax
		}
#endif
		// 8000 0000 = 10000000 00000000 00000000 00000000
		return (res & 0x80000000) ? false : true;
	}

	void LoadPDBR(uint32_t physicalAddr)
	{
#ifdef _MSC_VER
		_asm
		{
			mov eax, [physicalAddr]
			mov cr3, eax // PDBR is cr3 register in i86
		}
#endif
	}

	uint32_t GetPDBR()
	{
#ifdef _MSC_VER
		_asm
		{
			mov eax, cr3
			ret
		}
#endif
	}

	void Dump()
	{
		CYNConsole::Print("Memory Size : 0x%x\n", m_memorySize);
		CYNConsole::Print("Memory Map Address : 0x%x\n", m_pMemoryMap);
		CYNConsole::Print("Memory Map Size : %d bytes\n", m_memoryMapSize);
		CYNConsole::Print("Max Block Count : %d\n", m_maxBlocks);

		CYNConsole::Print("Used Block Count : %d\n", m_usedBlocks);
	}

}