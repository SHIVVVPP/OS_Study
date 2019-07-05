#pragma once

#include "windef.h"
#include "ordered_array.h"

#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC		0x123890AB
#define HEAP_MIN_SIZE	0x70000

/*
	hole / block 의 사이즈 정보
*/
// header
typedef struct
{
	u32int magic;	// Magic Number, 힙 손상이 발생했는지를 확인할 수 있는 체크섬 값
	u8int is_hole;	// 1이면 hole, 0이면 block
	u32int size;	// header와 footer를 포함한 블록의 총 크기
} header_t;

// footer
typedef struct
{
	u32int magic;		// 4바이트 매직넘버
	header_t* header;	// 블록 헤더를 가리키는 포인터
} footer_t;

typedef struct
{
	ordered_array_t index;
	u32int start_address;	// 할당된 공간의 시작주소
	u32int end_address;		// 할당된 공간의 끝주소, max_address까지 확장될 수 있다.
	u32int max_address;		// 힙이 확장될 수 있는 한계 주소
	u8int supervisor;		// Should extra pages requested by us be mapped as supervisor-only?
	u8int readonly;			// Should extra pages requested by us be mapped as read-only?
} heap_t;

#ifdef __cplusplus
extern "C" {
#endif

// 새로운 힙 생성
heap_t* create_kernel_heap(u32int start, u32int end_addr, u32int max, u8int supervisor, u8int readonly);
heap_t* create_heap(u32int start, u32int end_addr, u32int max, u8int supervisor, u8int readonly);

// size 만큼 연속적인 메모리 공간을 할당한다.
// 만약 page_align == 1 이라면, 페이지 경계에서 부터 블록을 생성한다.
/*
시스템 메모리는 "페이지"라고 블리는 단위로 나누어진다.
인텔 아키텍처의 경우 4096Byte(4K)로 구성된다.
메모리 주소가 페이지 시작주소라면 "페이지 정렬되었다.(page-aligned)"고 한다.
그러므로 4K 페이지 크기의 아키텍처에서 
4096, 12,288(3*4096), 413,696(101*4096)은 페이지 정렬된 메모리 주소의 인스턴스들이다.

힙 메모리 관리자와 같은 몇몇의 프로세스들은 페이지 정렬이 요구된다.
*/
void* memory_alloc(u32int size, u8int page_align, heap_t* heap);

// 'alloc'을 통해할당된 block을 해제한다.
void free(void* p, heap_t* heap);

/*
	sz 크기만큼 메모리를 할당한다.
	만약 align == 1이면, 메모리는 page-align되어야 하고,
	만약 phys != 0 이면, 할당된 메모리의 위치는 phys에 저장된다.

	이것은 kmalloc의 내부 버전이다. 사용자 친화적
	매개변수 표현은 kmalloc, kmalloc_a로 사용할 수 있다.
	kmalloc_ap, kmalloc_p.
*/
u32int kmalloc_int(u32int sz, int align, u32int* phys);

u32int kmalloc_a(u32int sz);

// sz 만큼 메모리를 할당해 phys로 리턴한다.
u32int kmalloc_p(u32int sz, u32int* phys);

// sz 만큼 메모리를 할당하고 물리주소는 phys에 리턴된다.
u32int kmalloc_ap(u32int sz, u32int* phys);

// 일반적인 할당함수
u32int kmalloc(u32int sz);
u32int malloc(u32int sz);
u32int calloc(u32int count, u32int size);

// 일반적인 해제 함수
void kfree(void* p);

#ifdef __cplusplus
}
#endif