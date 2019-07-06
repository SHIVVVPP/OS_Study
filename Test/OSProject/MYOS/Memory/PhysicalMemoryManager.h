#pragma once
#include "windef.h"
#include "stdint.h"
#include "MultiBoot.h"
#include "HAL.h"

// 메모리 맵은 4바이트당 32KB 공간을 다룰 수 있다.
// 메모리 한 블록을 4KB로 설정하였으므로 물리 메모리를 통한 최소 메모리 할당 크기는 4KB(4 * 1024 Byte)이다.


//PMM - Physical Memory Manager
// 1바이트당 블록 수 (1비트가 하나의 블록을 나타낸다.)
#define PMM_BLOCKS_PER_BYTE	8	
// 블록 하나당 메모리 할당 크기 4KB(4 * 1024 Byte)
#define PMM_MEMORY_PER_BLOCKS		4096
#define PMM_BLOCK_ALIGN		BLOCK_SIZE
// 인덱스 한 요소에 있는 bit 수
#define PMM_BITS_PER_INDEX	32

namespace PhysicalMemoryManager
{
	//==========================================================
	// Getter & Setter
	//==========================================================

	uint32_t GetMemoryMapSize();	// 메모리 맵의 크기를 얻어낸다.
	uint32_t GetKernelEnd();		// 로드된 커널 시스템의 마지막 주소를 얻어낸다.
	size_t	GetMemorySize(); 		// 메모리 사이즈를 얻는다.

	uint32_t GetTotalBlockCount();	// 블록의 전체 수를 리턴한다.
	uint32_t GetUsedBlockCount();	// 사용된 블록수를 리턴한다.
	uint32_t GetFreeBlockCount();	// 사용되지 않은 블록수를 리턴한다.

	uint32_t GetFreeMemory();
	uint32_t GetBlockSize();		// 블록의 사이즈를 리턴한다. 4KB

//==========================================================
// Functions
//==========================================================

// INITIALIZE
	void Initialize(multiboot_info* bootinfo);
	// 물리 메모리의 사용 여부에 따라 초기에 프레임들을 Set하거나 Unset한다.
	void SetAvailableMemory(uint32_t base, size_t size);
	void SetDeAvailableMemory(uint32_t base, size_t size);

	// MEMORY BITMAP
		// 페이지, 블록, 프레임은 모두 같은 의미이다.
		// 페이지는 주로 가상 주소에서 쓰이고, 프레임은 물리주소에서 쓰인다.
		// 우리가 만드는 운영체제 에서는 페이지(4K) 프레임(4K)로,
		// 하나의 페이지, 프레임이 동일한 메모리 크기를 나타낸다.
		// 프레임들을 Set하거나 Unset한다.
	void SetBit(int bit);
	void UnsetBit(int bit);

	bool TestMemoryMap(int bit);

	// MEMORY ALLOCATION
		// 사용할 수 있는 프레임 번호를 얻는다.
	unsigned int GetFreeFrame();
	unsigned int GetFreeFrames(size_t size);

	// 블록을 할당한다.
	void* AllocBlock();
	void* AllocBlocks(size_t size);

	// 블록을 해제한다. 
	void	FreeBlock(void* p);
	void	FreeBlocks(void* p, size_t size);


	//==========================================================
	// Control Register
	//==========================================================
	void EnablePaging(bool state);
	bool IsPaging();

	void		LoadPDBR(uint32_t physicalAddr);
	uint32_t	GetPDBR();



	//DEBUG
	void Dump();
}