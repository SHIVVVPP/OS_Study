#include "HeapManager.h"
#include "CYNConsole.h"
#include "kheap.h"

using namespace VirtualMemoryManager;

bool g_heapInit = false;
extern DWORD g_usedHeapSize;

namespace HeapManager
{
	int m_heapFrameCount = 0;

	//Physical Heap Address
	void* m_pKernelHeapPhysicalMemory = 0;

	DWORD GetHeapSize() { return m_heapFrameCount * PAGE_SIZE; }

	DWORD GetUsedHeapSize() { return g_usedHeapSize; }

	bool InitKernelHeap(int heapFrameCount)
	{
		PageDirectory* curPageDirectory = GetKernelPageDirectory();

		//힙의 가상주소
		void* pVirtualHeap = (void*)(KERNEL_VIRTUAL_HEAP_ADDRESS);

		//프레임 수 만큼 물리 메모리 할당을 요청한다.
		m_heapFrameCount = heapFrameCount;
		m_pKernelHeapPhysicalMemory = PhysicalMemoryManager::AllocBlocks(m_heapFrameCount);

		if (m_pKernelHeapPhysicalMemory == NULL)
		{
#ifdef _HEAP_DEBUG
			CYNConsole::Print("kernel heap allocation fail. frame count : %d\n", m_heapFrameCount);
#endif

			return false;
		}
	}

	bool MapHeapToAddressSpace(PageDirectory* curPageDirectory)		
	{
		int endAddress = (uint32_t)KERNEL_VIRTUAL_HEAP_ADDRESS + m_heapFrameCount * PMM_BLOCK_SIZE;
		//int frameCount = (endAddress - KERNEL_VIRTUAL_HEAP_ADDRESS) / PAGE_SIZE;

		for (int i = 0; i < m_heapFrameCount; i++)
		{
			uint32_t virt = (uint32_t)KERNEL_VIRTUAL_HEAP_ADDRESS + i * PAGE_SIZE;
			uint32_t phys = (uint32_t)m_pKernelHeapPhysicalMemory + i * PAGE_SIZE;

			MapPhysicalAddressToVirtualAddress(curPageDirectory, virt, phys, I86_PTE_PRESENT | I86_PTE_WRITABLE);

		}

		return true;
	}


}