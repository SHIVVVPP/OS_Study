﻿#include "kmain.h"
#include "PhysicalMemoryManager.h"
#include "VirtualMemoryManager.h"
#include "HeapManager.h"
#include "Exception.h"
#include "FPU.h"

//커널 엔트리 찾기

/*
GRUB이 커널을 호출할 준비가 되었지만,
커널 엔트리 포인트, 즉 커널 함수 시작 지점을 호출하기 위해 특정 명세(틀)를 준수해야 한다.
GRUB은 커널이 특정 명세를 지키는지 여부를 커널 최초 80KB(시작부터 ~ +0x14000) 부분을 검색해 특정 시그니처를 찾아 파악한다.
이 시그니처를 멀티 부트 헤더 구조체 (struct MULTIBOOT_HEADER) 라고 한다.
*/
//GRUB이 요구하는 시그니처( 멀티 부트 헤더 구조체 ) MULTIBOOT_HEADER형태로 채워진다.

//_declspec - decl-spcifier(extended-decl-modifier-seq)
/*
Stack Frame
	함수가 사용하게 되는 Stack 영역
	실행 중인 함수는 자신만의 Stack영역을 사용하게 된다.
	함수가 실행되면 Stack Frame을 열고 함수가 종료되면 StackFrame을 닫는다.

Frame Pointer
	함수의 StackFrame 시작위치를 가리키는 레지스터
	주로 StackFrame을 오픈할때 설정되며 클로즈 될때까지 변하지 않는다.
Function Prolog
	Stack Frame을 사용하는 경우 함수는 항상 FramePointer를 셋업하는 코드 구믄으로 시작하게된다.
	일반적인 셋업 코드 구문은
	push ebp // Caller의 StackFramePointer를 백업한다.
	move ebp, esp // esp값을 ebp에 복사함으로써 StackFramePointer를 초기화한다.
	sub esp, n // 할당된 지역변수를 위한 공간으로 esp 위치를 옮겨 확보한다.
Function Epliog
	StackFrame을 사용하는 경우 함수의 끝부분에는 FramePointer를 원래대로 복원하는 코드가 오게된다.
	일반적인 복원 코드 구문은
	move esp, ebp // ebp 즉 고정된 ebp 값을 esp에 줌으로써 지역변수를 위해 주었던 공간을 해제한다.
	pop ebp // ebp를 꺼내므로써 ret, call 하게 된다. 즉, 원래의 ebp 위치로 return값을 call 해온다.

_declspec(naked) 는 위의 Function Prolog와 Epilog를 사용하지 않는다.
	컴파일러가 함수 내에서 추가 코드를 자동으로 생성하지 않도록 하기 위해 사용한다.
	https://stackoverflow.com/questions/3021513/could-someone-explain-declspecnaked-please
*/
_declspec(naked) void multiboot_entry(void)
{
	__asm {
		align 4

		multiboot_header:
		//멀티부트 헤더 사이즈 : 0X20
		dd(MULTIBOOT_HEADER_MAGIC); magic number
		dd(MULTIBOOT_HEADER_FLAGS); flags
		dd(CHECKSUM); checksum
		dd(HEADER_ADRESS); //헤더 주소 KERNEL_LOAD_ADDRESS+ALIGN(0x100064)
		dd(KERNEL_LOAD_ADDRESS); //커널이 로드된 가상주소 공간
		dd(00); //사용되지 않음
		dd(00); //사용되지 않음
		dd(HEADER_ADRESS + 0x20); //커널 시작 주소 : 멀티부트 헤더 주소 + 0x20, kernel_entry
			
		kernel_entry :
		mov     esp, KERNEL_STACK; //스택 설정

		push    0; //플래그 레지스터 초기화
		popf

		//GRUB에 의해 담겨 있는 정보값을 스택에 푸쉬한다.
		push    ebx; //멀티부트 구조체 포인터
		push    eax; //매직 넘버

		//위의 두 파라메터와 함께 kmain 함수를 호출한다.
		call    kmain; //C++ 메인 함수 호출

		//루프를 돈다. kmain이 리턴되지 않으면 아래 코드는 수행되지 않는다.
		halt:
		jmp halt;
	}
}

void InitContext(multiboot_info* bootInfo);
void InitHardware();
bool InitMemoryManager(multiboot_info* pBootInfo);



// 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(->addr), 매직넘버(->magic)로 호출
void kmain(unsigned long magic, unsigned long addr)
{
	//add
	InitializeConstructors();

	multiboot_info* pBootInfo = (multiboot_info*)addr;

	InitContext(pBootInfo);
	

	// 하드웨어 초기화 과정중 인터럽트가 발생하지 않아야 하므로
	kEnterCriticalSection();

	InitHardware();
	// 물리/ 가상 메모리 매니저 초기화
	InitMemoryManager(pBootInfo);

	


	kLeaveCriticalSection();
	////타이머를 시작한다.
	
	StartPITCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);

	//TestInterrupt();

	for (;;);
}

// 하드웨어 초기화
void InitHardware()
{
	GDTInitialize();
	IDTInitialize(0x8);
	PICInitialize(0x20, 0x28);
	InitializePIT();

	CYNConsole::Print("Hardware Init Complete\n");

	SetInterruptVector();
	CYNConsole::Print("Interrupt Handler Init Complete\n");


	if (false == InitFPU())
	{
		CYNConsole::Print("[Warning] Floating Pointer Unit(FPU) Detection Fail!\n");
	}
	else
	{
		EnableFPU();
		CYNConsole::Print("FPU Init...\n");
		//TestFPU();
	}
}

//  메모리 매니저
bool InitMemoryManager(multiboot_info* pBootInfo)
{
	// 물리/가상 메모리 매니저를 초기화한다.
	// 기본 설정 시스템 메모리는 128MB
	CYNConsole::Print("Memory Manager Init Complete\n");

	PhysicalMemoryManager::EnablePaging(false);

	//물리 메모리 매니저 초기화
	PhysicalMemoryManager::Initialize(pBootInfo);
	PhysicalMemoryManager::Dump();

	//가상 메모리 매니저 초기화
	VirtualMemoryManager::Initialize(); 
	PhysicalMemoryManager::Dump();

	// 힙 초기화
	int heapFrameCount = 256 * 10 * 5; // 프레임 수 12800개, 52MB
	unsigned int requireHeapSize = heapFrameCount * PAGE_SIZE;

	// 요구되는 힙의 크기가 자유공간보다 크다면 그 크기를 자유공간 크기로 맞춘다음 반으로 줄인다.
	uint32_t memorySize = PhysicalMemoryManager::GetMemorySize();
	if (requireHeapSize > memorySize)
	{
		requireHeapSize = memorySize / 2;
		heapFrameCount = requireHeapSize / PAGE_SIZE / 2;
	}

	HeapManager::InitKernelHeap(heapFrameCount);
	CYNConsole::Print("Heap %dMB Allocated\n", requireHeapSize / 1048576); // 1MB = 1048576byte

	CYNConsole::Print("Memory Manager Init Complete\n");
	return true;
}

void InitContext(multiboot_info* pBootInfo)
{
	CYNConsole::Initialize();

	//add
	//헥사를 표시할 때 %X는 integer, %x는 unsigned integer의 헥사값을 표시한다.
	CYNConsole::Print("*** MY OS Console System Init ***\n");

	CYNConsole::Print("GRUB Information\n");
	CYNConsole::Print("Boot Loader Name : %s\n", (char*)pBootInfo->boot_loader_name);
}
