#include "VirtualMemoryManager.h"
#include "PhysicalMemoryManager.h"
#include "string.h"
#include "memory.h"
#include "SkyConsole.h"
#include "MultiBoot.h"
#include "SkyAPI.h"

PageDirectory* g_pageDirectoryPool[MAX_PAGE_DIRECTORY_COUNT];
bool g_pageDirectoryAvailable[MAX_PAGE_DIRECTORY_COUNT];

namespace VirtualMemoryManager
{
	// Current Directory Table
	PageDirectory* _kernel_directory = 0;
	PageDirectory* _cur_directory = 0;

	bool Initialize()
	{
		SkyConsole::Print("Virtual Memory Manager Init..\n");

		for (int i = 0; i < MAX_PAGE_DIRECTORY_COUNT; i++)
		{
			g_pageDirectoryPool[i] = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
			g_pageDirectoryAvailable[i] = true;
		}

		PageDirectory* dir = CreateCommonPageDirectory();

		if (nullptr == dir)
			return false;

		// 페이지 디렉토리를 PDBR 레지스터에 로드한다.
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);

		SetPageDirectory(dir);

		// 페이징 기능을 다시 활성화 한다.
		PhysicalMemoryManager::EnablePaging(true);

		return false;
	}


//=========================================================================
//			PAGE DIRECTORY
//=========================================================================
#pragma region PAGE DIRECTORY
	PageDirectory* CreateCommonPageDirectory()
	{
		// 페이지 디렉토리를 생성한다.
		// 4GB를 표현하기 위해서 페이지 디렉토리는 하나면 충분하다. (1024 * 1024 * 4MB)
		int index = 0;
		for (; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
			if (g_pageDirectoryAvailable[index] == true)
				break;
		}

		if (index == MAX_PAGE_DIRECTORY_COUNT)
			return nullptr;

		PageDirectory* dir = g_pageDirectoryPool[index];

		if (dir == NULL)
			return nullptr;

		g_pageDirectoryAvailable[index] = false;
		memset(dir, 0, sizeof(PageDirectory));

		uint32_t frame = 0x00000000;
		uint32_t virt = 0x00000000;

		//페이지 테이블 생성
		for (int i = 0; i < 2; i++)
		{
			PageTable* identityPageTable = (PageTable*)PhysicalMemoryManager::AllocBlock();
			if (identityPageTable == NULL)
				return nullptr;

			memset(identityPageTable, 0, sizeof(PageTable));

			//물리 주소를 가상 주소와 동일하게 매핑시킨다.
			for (int j = 0; j < PAGES_PER_TABLE; j++, frame += PAGE_SIZE, virt += PAGE_SIZE)
			{
				PTE page = 0;
				PageTableEntry::AddAttribute(&page, I86_PTE_PRESENT);

				PageTableEntry::SetFrame(&page, frame);

				identityPageTable->m_entries[PAGE_TABLE_INDEX(virt)] = page;
			}

			// 페이지 디렉토리에 페이지 디렉토리 엔트리(PDE)를 한개 세트한다.
			// 0번째 인덱스에 PDE를 세트한다.(가상주소가 0x00000000일시 참조됨)
			// 앞에서 생성한 아이덴티티 페이지 테이블을 세팅한다.
			// 가상주소 = 물리주소

			PDE* identityEntry = &dir->m_entries[PAGE_DIRECTORY_INDEX((virt - 0x00400000))];
			PageDirectoryEntry::AddAttribute(identityEntry, I86_PDE_PRESENT | I86_PDE_WRITABLE);
			PageDirectoryEntry::SetFrame(identityEntry, (uint32_t)identityPageTable);
		}
		return dir;
	}

	void SetPageDirectory(PageDirectory* dir)
	{
		_asm
		{
			mov eax, [dir]
			mov cr3, eax  // PDBR is cr3 register in i86
		}
	}

	bool SetCurPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return false;

		_cur_directory = dir;

		return true;
	}

	PageDirectory* GetCurPageDirectory()
	{
		return _cur_directory;
	}

	
	bool SetKernelPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return false;

		_kernel_directory = dir;

		return true;
	}

	PageDirectory* GetKernelPageDirectory()
	{
		return _kernel_directory;
	}


	

	PDE* GetPDE(PageDirectory* dir, uint32_t addr)
	{
		if (dir == NULL)
			return NULL;

		// GetPageTableIndex함수를 써도 DirectoryIndex를 구할 수 있다.
		return &dir->m_entries[GetPageTableIndex(addr)];

	}


	void FreePageDirectory(PageDirectory* dir)
	{
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;
		for (int i = 0; i < PAGES_PER_DIRECTORY; i++)
		{
			PDE& pde = pageDir[i];

			if (pde != 0)
			{
				/* get Mapped Frame*/
				void* frame = (void*)(pageDir[i] & 0x7FFFF000);
				PhysicalMemoryManager::FreeBlock(frame);
				pde = 0;
			}
		}

		for (int index = 0; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
			if (g_pageDirectoryPool[index] == dir)
			{
				g_pageDirectoryAvailable[index] = true;
				break;
			}
		}

		PhysicalMemoryManager::EnablePaging(true);
	}

	void ClearPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return;

		memset(dir, 0, sizeof(PageDirectory));
	}


	PageDirectory* CreatePageDirectory()
	{
		PageDirectory* dir = NULL;

		/* allocate page directory */
		dir = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
		if (!dir)
			return NULL;

		//memset(dir, 0, sizeof(PageDirectory));

		return dir;
	}

	

#pragma endregion
//=========================================================================

	

//=========================================================================
//			PAGE TABLE
//=========================================================================
#pragma region PAGE TABLE

	bool CreatePageTable(PageDirectory* dir, uint32_t virt, uint32_t flags)
	{
		PDE* pageDirectory = dir->m_entries;
		if (pageDirectory[virt >> 22] == 0)
		{
			void* pPageTable = PhysicalMemoryManager::AllocBlock();
			if (pPageTable == nullptr)
				return false;

			memset(pPageTable, 0, sizeof(PageTable));
			pageDirectory[virt >> 22] = ((uint32_t)pPageTable) | flags;
		}
		return true;
	}
	
	bool AllocPage(PTE* e)
	{
		void* p = PhysicalMemoryManager::AllocBlock();

		if (p == NULL)
			return false;

		PageTableEntry::SetFrame(e, (uint32_t)p);
		PageTableEntry::AddAttribute(e, I86_PTE_PRESENT);

		return true;
	}

	void FreePage(PTE* e)
	{
		void* p = (void*)PageTableEntry::GetFrame(*e);
		if (p)
			PhysicalMemoryManager::FreeBlock(p);

		PageTableEntry::DelAttribute(e, I86_PTE_PRESENT);
	}

	void ClearPageTable(PageTable* p)
	{
		if (p != NULL)
			memset(p, 0, sizeof(PageTable));
	}

	void UnmapPageTable(PageDirectory* dir, uint32_t virt)
	{
		PDE* pageDir = dir->m_entries;
		if (pageDir[virt >> 22] != 0)
		{
			/* get mapped frame */
			void* frame = (void*)(pageDir[virt >> 22] & 0x7FFFF000);

			/* unmap Frame*/
			PhysicalMemoryManager::FreeBlock(frame);
			pageDir[virt >> 22] = 0;
		}
	} 

	void UnmapPhysicalAddress(PageDirectory* dir, uint32_t virt)
	{
		PDE* pagedir = dir->m_entries;
		if (pagedir[virt >> 22] != 0)
			UnmapPageTable(dir, virt);
	}

	PTE* GetPTE(PageTable* p, uint32_t addr)
	{
		if (p == NULL)
			return NULL;

		return &p->m_entries[GetPageTableEntryIndex(addr)];
	}

	uint32_t GetPageTableEntryIndex(uint32_t addr)
	{
		return (addr >= PTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

	uint32_t GetPageTableIndex(uint32_t addr)
	{
		return (addr >= DTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

#pragma endregion
//=========================================================================

//=========================================================================
//			ADDRESS MAPPING
//=========================================================================
	//PDE나 PTE의 플래그는 같은 값을 공유한다.
	// 가상주소를 물리 주소에 매핑시킨다.
	void MapPhysicalAddressToVirtualAddress(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{

		//SkyAPI에 추가
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);

		PDE* pageDir = dir->m_entries;

		if (pageDir[virt >> 22] == 0)
			CreatePageTable(dir, virt, flags);

		uint32_t mask = (uint32_t)(~0xfff); // 11111111 11111111 11110000 0000 0000
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		pageTable[virt << 10 >> 10 >> 12] = phys | flags;

		PhysicalMemoryManager::EnablePaging(true);
		kLeaveCriticalSection();
	}

	void* GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress)
	{
		PDE* pagedir = directory->m_entries;
		if (pagedir[virtualAddress >> 22] == 0)
			return NULL;

		return (void*)((uint32_t*)(pagedir[virtualAddress >> 22] & ~0xfff))[virtualAddress << 10 >> 10 >> 12];
	}

}