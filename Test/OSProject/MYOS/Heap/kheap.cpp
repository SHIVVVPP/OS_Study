#include "kheap.h"
#include "PageDirectoryEntry.h"
#include "PageTableEntry.h"
#include "VirtualMemoryManager.h"
#include "HAL.h"
#include "CYNConsole.h"
#include "CYNAPI.h"
#include "sprintf.h"

//=====================================================================================
//	PRIVATE MEMBER VARIABLE
//=====================================================================================
// end is defined in the linker script.
// extern u32int end;
// u32int placement_address = (u32int)&end;
heap_t	kheap;
DWORD	g_usedHeapSize = 0;

//=====================================================================================
//	STATIC FUNCTION - BLOCK TABLE expand() 확장, find_smallest_hole(), contract() 수축, 비교함수
//=====================================================================================
#pragma region STATIC FUNCTION
static void expand(u32int new_size, heap_t* heap)
{
	// CYNAPI CYN_ASSERT 추가
	CYN_ASSERT(new_size > heap->end_address - heap->start_address, "new_size > heap->end_address - heap->start_address");

	// Get the nearest following page boundary.
	if ((new_size & 0xFFFFF000) != 0)
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	// Make sure we are not overreaching ourselves.
	CYN_ASSERT(heap->start_address + new_size <= heap->max_address, "heap->start_address+new_size <= heap->max_address");

	// This should always be on a page boundary.
	u32int old_size = heap->end_address - heap->start_address;

	u32int i = old_size;
	while (i < new_size)
	{
		i += 0x1000 /* page size */;
	}
	heap->end_address = heap->start_address + new_size;
}

static s32int find_smallest_hole(u32int size, u8int page_align, heap_t* heap)
{
	u32int iterator = 0;
	while (iterator < heap->index.size)
	{
		header_t* header = (header_t*)lookup_ordered_array(iterator, &heap->index);
		// If the user has requested the memory be page-aligned
		if (page_align > 0)
		{
			//page-align the starting point of this header
			u32int location = (u32int)header;
			s32int offset = 0;
			if (((location + sizeof(header_t)) & 0xFFFFF000) != 0)
				offset = 0x1000 /* page size */ - (location + sizeof(header_t)) % 0x1000;
			s32int hole_size = (s32int)header->size - offset;
			// Can we fit now?
			if (hole_size >= (s32int)size)
				break;
		}
		else if (header->size >= size)
			break;
		iterator++;
	}
	//Why did the loop exit?
	if (iterator == heap->index.size)
		return -1;
	else
		return iterator;
}

static u32int contract(u32int new_size, heap_t* heap)
{
	char buf[256];
	sprintf(buf, "0x%x 0x%x\n", new_size, heap->end_address - heap->start_address);
	CYN_ASSERT(new_size < heap->end_address - heap->start_address, buf);

	//get the nearest following page boundary.
	if (new_size & 0x1000)
	{
		new_size &= 0x1000;
		new_size += 0x1000;
	}

	// don't contract too far
	if (new_size < HEAP_MIN_SIZE)
		new_size = HEAP_MIN_SIZE;

	u32int old_size = heap->end_address - heap->start_address;
	u32int i = old_size - 0x1000;
	while (new_size < i)
	{
		i -= 0x1000;
	}

	heap->end_address = heap->start_address + new_size;
	return new_size;
}

static s8int header_t_less_than(void* a, void* b)
{
	return (((header_t*)a)->size < ((header_t*)b)->size) ? 1 : 0;
}


#pragma endregion


//=====================================================================================
//	CREATE HEAP
//=====================================================================================
#pragma region CREATE HEAP
heap_t* create_kernel_heap(u32int start, u32int end_addr, u32int max, u8int supervisor, u8int readonly)
{
	heap_t* heap = &kheap;

	// All our assumptions are made on startAddress and endAddress being page-aligned.
	// 시작 주소와 끝 주소가 page-aligned 된 상태가 되어야한다.
	CYN_ASSERT(start % 0x1000 == 0, "start % 0x1000 == 0");
	CYN_ASSERT(end_addr % 0x1000 == 0, "end_addr % 0x1000 == 0");

	// Initialise the index.
	heap->index = place_ordered_array((void*)start, HEAP_INDEX_SIZE, &header_t_less_than);

	// Shift the start address forward to resemble where we can start putting data.
	start += sizeof(type_t) * HEAP_INDEX_SIZE;

	// Make sure the start address is page-aligned.
	if ((start & 0xFFFFF000) != 0)
	{
		start &= 0xFFFFF000; // page 
		start += 0x1000;
	}
	// Write the start, end and max addresses into the heap structure.
	heap->start_address = start;
	heap->end_address = end_addr;
	heap->max_address = max;
	heap->supervisor = supervisor;
	heap->readonly = readonly;

	// We start off with one large hole in the index.
	header_t* hole = (header_t*)start;
	hole->size = end_addr - start;
	hole->magic = HEAP_MAGIC;
	hole->is_hole = 1;
	insert_ordered_array((void*)hole, &heap->index);

	return heap;
}

heap_t* create_heap(u32int start, u32int end_addr, u32int max, u8int supervisor, u8int readonly)
{
	heap_t* heap = (heap_t*)kmalloc(sizeof(heap_t));

	// All our assumptions are made on startAddress and endAddress being page-aligned.
	CYN_ASSERT(start % 0x1000 == 0, "start%0x1000 == 0");
	CYN_ASSERT(end_addr % 0x1000 == 0, "end_addr%0x1000");

	// Initialise the index.
	heap->index = place_ordered_array((void*)start, HEAP_INDEX_SIZE, &header_t_less_than);



	// Shift the start address forward to resemble where we can start putting data.
	start += sizeof(type_t) * HEAP_INDEX_SIZE;

	// Make sure the start address is page-aligned.
	if ((start & 0xFFFFF000) != 0)//???
	{
		start &= 0xFFFFF000;
		start += 0x1000;
	}
	// Write the start, end and max addresses into the heap structure.
	heap->start_address = start;
	heap->end_address = end_addr;
	heap->max_address = max;
	heap->supervisor = supervisor;
	heap->readonly = readonly;

	// We start off with one large hole in the index.
	header_t* hole = (header_t*)start;
	hole->size = end_addr - start;
	hole->magic = HEAP_MAGIC;
	hole->is_hole = 1;
	insert_ordered_array((void*)hole, &heap->index);

	return heap;
}
#pragma endregion



//=====================================================================================
//	MEMORY ALLOCATION
//=====================================================================================
#pragma region MEMORY ALLOCATION
u32int malloc(u32int sz)					{	return kmalloc(sz);					}
u32int calloc(u32int count, u32int size)	{	return count * size;				}

u32int kmalloc_a(u32int sz)					{	return kmalloc_int(sz, 1, 0);		}
u32int kmalloc_p(u32int sz, u32int* phys)	{	return kmalloc_int(sz, 0, phys);	}
u32int kmalloc_ap(u32int sz, u32int* phys)	{	return kmalloc_int(sz, 1, phys);	}

u32int kmalloc(u32int sz)
{
	kEnterCriticalSection();
	g_usedHeapSize += sz + sizeof(footer_t) + sizeof(header_t);

	u32int buffer = kmalloc_int(sz, 0, 0);
	kLeaveCriticalSection();

	return buffer;
}

u32int kmalloc_int(u32int sz, int align, u32int* phys)
{
	void* addr = memory_alloc(sz, (u8int)align, &kheap);

	return (u32int)addr;
}

void* memory_alloc(u32int size, u8int page_align, heap_t* heap)
{
	// Make sure we take the size of header/footer into account.
	u32int new_size = size + sizeof(header_t) + sizeof(footer_t);
	// Find the smallest hole that will fit.
	
	s32int iterator = find_smallest_hole(new_size, page_align, heap);

	if (iterator == -1) // 크기에 맞는 블록이 없다면 힙을 늘려서 할당 받아야한다.
	{
		// 기존 힙의 크기와 끝 주소를 old_length와 old_end_address에 임시로 저장해둔다.
		u32int old_length = heap->end_address - heap->start_address;
		u32int old_end_address = heap->end_address;

		// new_size만큼 힙의 크기를 늘린다.
		expand(old_length + new_size, heap);
		u32int new_length = heap->end_address - heap->start_address;

		// 주소상 가장 마지막에 위치한 헤더를 찾는다.
		iterator = 0;
		// idx는 테이블에서의 인덱스 값을 저장하고 value는 실제 주소값을 저장한다.
		u32int idx = 0xFFFFFFFF; u32int value = 0x0;
		while (iterator < (s32int)heap->index.size)
		{
			u32int tmp = (u32int)lookup_ordered_array(iterator, &heap->index); // lookup_ordered_array는 해당 인덱스의 블록 주소를 반환
			if (tmp > value)
			{
				value = tmp;
				idx = iterator;
			}
			iterator++;
		}

		// 만약 아무 헤더도 찾지 못했다면, 하나를 추가한다. (아무 블록도 없는 상태)
		if (idx == -1)
		{
			header_t* header = (header_t*)old_end_address; 
			header->magic = HEAP_MAGIC;
			header->size = new_length - old_length;
			header->is_hole = 1;
			footer_t* footer = (footer_t*)(old_end_address + header->size - sizeof(footer_t));
			footer->magic = HEAP_MAGIC;
			footer->header = header;
			insert_ordered_array((void*)header, &heap->index);
		}
		else
		{
			// 찾은 헤더(위치상 가장 마지막)를 추가된 사이즈에 맞게 수정해준다.
			// The last header needs adjusting.
			header_t* header = (header_t*)lookup_ordered_array(idx, &heap->index);
			header->size += new_length - old_length; // new_length - old_length = 추가된 길이
			// footer 또한 그에 맞게 수정해준다.
			footer_t* footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
			footer->header = header;
			footer->magic = HEAP_MAGIC;
		}

		// We now have enough space. Recurse, and call the function again.
		// 이제 할당하기에 충분한 공간을 만들었으므로, 다시 memory_alloc을 호출한다.
		return memory_alloc(size, page_align, heap);
	}

	// 기존 hole의 값을 저장해두고,
	header_t* orig_hole_header = (header_t*)lookup_ordered_array(iterator, &heap->index);
	u32int orig_hole_pos = (u32int)orig_hole_header;
	u32int orig_hole_size = orig_hole_header->size;

	// 이 홀을 할당할때 나누어서 할당할지 결정한다.
	// 할당한 후 남는 메모리가 header와 footer를 담기에 충분하지 않다면
	// 할당하는 크기를 그 홀의 크기로 맞춘다.
	if (orig_hole_size - new_size < sizeof(header_t) + sizeof(footer_t))
	{
		// Then just increase the requested size to the size of the hole we found.
		size += orig_hole_size - new_size;
		new_size = orig_hole_size;
	}

	// 데이터를 페이지 정렬 할 필요가 있다면 현재 배열에 있는 블록에서 페이지 정렬을 위해 남은 offset 값만 블록에 남기도록한다. 
	// If we need to page-align the data, do it now and make a new hole in front of our block.
	if (page_align && orig_hole_pos & 0xFFFFF000)
	{
		// 정렬시킨 위치 ex) orig_hole_pos 가 0x111이라면 0x1111 - 0x111해서 0x1000(페이지 정렬됨) - sizeof(header_t)(9byte) 의 값을 할당하는 주소로 설정한다.
		u32int new_location = orig_hole_pos + 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(header_t);
		header_t* hole_header = (header_t*)orig_hole_pos;
		hole_header->size = 0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(header_t); // 할당하고 남은 사이즈 값
		hole_header->magic = HEAP_MAGIC;
		hole_header->is_hole = 1;
		footer_t* hole_footer = (footer_t*)((u32int)new_location - sizeof(footer_t));
		hole_footer->magic = HEAP_MAGIC;
		hole_footer->header = hole_header;
		orig_hole_pos = new_location;
		orig_hole_size = orig_hole_size - hole_header->size;
	}
	else
	{
		// 이미 정렬되어 있거나, 정렬 option이 설정되어있지 않다면 배열에서 삭제한다.
		remove_ordered_array(iterator, &heap->index);
	}

	// 리턴할 블록 설정
	// Overwrite the original header...
	header_t* block_header = (header_t*)orig_hole_pos;
	block_header->magic = HEAP_MAGIC;
	block_header->is_hole = 0; // 블록의 상태를 할당됨 으로 바꿈
	block_header->size = new_size; // 할당한 크기로설정
	// ...And the footer
	footer_t* block_footer = (footer_t*)(orig_hole_pos + sizeof(header_t) + size);
	block_footer->magic = HEAP_MAGIC;
	block_footer->header = block_header;

	// We may need to write a new hole after the allocated block.
	// We do this only if the new hole would have positive size...
	// 원래 hole의 사이즈가 할당하는 크기보다 크다면
	// 블록 배열에 추가해준다.
	if (orig_hole_size - new_size > 0)
	{
		header_t* hole_header = (header_t*)(orig_hole_pos + sizeof(header_t) + size + sizeof(footer_t));
		hole_header->magic = HEAP_MAGIC;
		hole_header->is_hole = 1;
		hole_header->size = orig_hole_size - new_size;
		footer_t* hole_footer = (footer_t*)((u32int)hole_header + orig_hole_size - new_size - sizeof(footer_t));
		if ((u32int)hole_footer < heap->end_address)
		{
			hole_footer->magic = HEAP_MAGIC;
			hole_footer->header = hole_header;
		}
		// Put the new hole in the index;
		insert_ordered_array((void*)hole_header, &heap->index);
	}

	// ...And we're done!
	return (void*)((u32int)block_header + sizeof(header_t));
}
#pragma endregion

//=====================================================================================
//	MEMORY RELEASE
//=====================================================================================
#pragma region MEMORY RELEASE
void kfree(void* p)
{
	kEnterCriticalSection();
	free(p, &kheap);
	kLeaveCriticalSection();
}

void free(void* p, heap_t* heap)
{
	// Exit gracefully for null pointers.
	if (p == 0)
		return;

	// Get the header and footer associated with this pointer.
	header_t* header = (header_t*)((u32int)p - sizeof(header_t));
	footer_t* footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));

	g_usedHeapSize -= header->size;

	// Sanity checks.
	//ASSERT(header->magic == HEAP_MAGIC, "header->magic == HEAP_MAGIC");
	//ASSERT(footer->magic == HEAP_MAGIC, "footer->magic == HEAP_MAGIC");

	CYN_ASSERT(header->magic == HEAP_MAGIC, "header->magic == HEAP_MAGIC");
	CYN_ASSERT(footer->magic == HEAP_MAGIC, "footer->magic == HEAP_MAGIC");

	// Make us a hole.
	header->is_hole = 1;

	// Do we want to add this header into the 'free holes' index?
	char do_add = 1;

	// Unify left 왼쪽과 합치기
	// If the thing immediately to the left of us is a footer...
	footer_t* test_footer = (footer_t*)((u32int)header - sizeof(footer_t));
	if (test_footer->magic == HEAP_MAGIC &&
		test_footer->header->is_hole == 1)
	{
		u32int cache_size = header->size; // Cache our current size.
		header = test_footer->header;     // Rewrite our header with the new one.
		footer->header = header;          // Rewrite our footer to point to the new header.
		header->size += cache_size;       // Change the size.
		do_add = 0;                       // Since this header is already in the index, we don't want to add it again.		
	}

	// Unify right 오른쪽과 합치기
	// If the thing immediately to the right of us is a header...
	header_t* test_header = (header_t*)((u32int)footer + sizeof(footer_t));
	if (test_header->magic == HEAP_MAGIC &&
		test_header->is_hole)
	{
		//SkyConsole::Print("test_header : 0x%x\n", test_header);
		//SkyConsole::Print("test_header_size : 0x%x\n", test_header->size);
		header->size += test_header->size; // Increase our size.

		test_footer = (footer_t*)((u32int)test_header + // Rewrite it's footer to point to our header.
			test_header->size - sizeof(footer_t));

		footer = test_footer;

		//20180419 Heap Bug Fix
		footer->header = header;

		// Find and remove this header from the index.
		u32int iterator = 0;
		while ((iterator < heap->index.size) &&
			(lookup_ordered_array(iterator, &heap->index) != (void*)test_header))
			iterator++;

		// Make sure we actually found the item.
		CYN_ASSERT(iterator < heap->index.size, "iterator < heap->index.size");
		// Remove it.
		remove_ordered_array(iterator, &heap->index);

		//SkyConsole::Print("C : 0x%x  0x%x 0x%x 0x%x 0x%x\n", p, header, footer, heap->end_address, heap->start_address);
	}

	// If the footer location is the end address, we can contract.
	if ((u32int)footer + sizeof(footer_t) == heap->end_address)
	{
		u32int old_length = heap->end_address - heap->start_address;
		u32int newSize = (u32int)footer - heap->start_address;

		u32int new_length = contract(newSize, heap);

		// Check how big we will be after resizing.
		if (header->size - (old_length - new_length) > 0)
		{
			// We will still exist, so resize us.
			header->size -= old_length - new_length;
			footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
			footer->magic = HEAP_MAGIC;
			footer->header = header;
		}
		else
		{
			// We will no longer exist :(. Remove us from the index.
			u32int iterator = 0;
			while ((iterator < heap->index.size) &&
				(lookup_ordered_array(iterator, &heap->index) != (void*)test_header))
				iterator++;
			// If we didn't find ourselves, we have nothing to remove.
			if (iterator < heap->index.size)
				remove_ordered_array(iterator, &heap->index);
		}
	}

	// If required, add us to the index.
	if (do_add == 1)
		insert_ordered_array((void*)header, &heap->index);
}
#pragma endregion