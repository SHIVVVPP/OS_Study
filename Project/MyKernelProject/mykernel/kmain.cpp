#include "kmain.h"

//커널 엔트리 찾기

/*
GRUB이 커널을 호출할 준비가 되었지만,
커널 엔트리 포인트, 즉 커널 함수 시작 지점을 호출하기 위해 특정 명세(틀)를 준수해야 한다.
GRUB은 커널이 특정 명세를 지키는지 여부를 커널 최초 80KB(시작부터 ~ +0x14000) 부분을 검색해 특정 시그니처를 찾아 파악한다.
이 시그니처를 멀티 부트 헤더 구조체 (struct MULTIBOOT_HEADER) 라고 한다.
*/
//GRUB이 요구하는 시그니처( 멀티 부트 헤더 구조체 ) MULTIBOOT_HEADER형태로 채워진다.
_declspec(naked) void multiboot_entry(void)
{
	__asm {
		align 4
		
		multiboot_header : 
		// 멀티 부트 헤더 사이즈 : 0x20
		dd(MULTIBOOT_HEADER_MAGIC); magic number
		dd(MULTIBOOT_HEADER_FLAGS); flags
		dd(CHECKSUM);				checksum
		dd(HEADER_ADRESS);			// 헤더 주소 KERNEL_LOAD_ADDRESS+ALIGN(0x100064)
		dd(KERNEL_LOAD_ADDRESS);	// 커널이 로드된 가상주소 공간
		dd(00);						// 사용되지 않음
		dd(00);						// 사용되지 않음
		dd(HEADER_ADRESS + 0x20);	// 커널 시작 주소 : 멀티부트 헤더 주소 + 0x20, kernel_entry

	kernel_entry :
		mov		esp, KERNEL_STACK;	// 스택 설정

		push 0;						// 플래그 레지스터 초기화
		popf

		//GRUB에 의해 담겨 있는 정보값을 스택에 푸쉬
		push	ebx;				// 멀티 부트 구조체 포인터
		push	eax;				// 매직넘버

		//위의 두 파라미터와 함께 kmain 함수를 호출한다.
		call	kmain;				// C++ 메인 함수 호출

		// 루프를 돈다. kmain이 리턴되지 않으면 아래 코드는 수행되지 않는다.
		halt:
		jmp halt;

	}
}

void InitializeConstructor()
{
	//내부 구현은 나중에 추가한다.
}

// 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(->addr), 매직넘버(->magic)로 호출
void kmain(unsigned long magic, unsigned long addr)
{
	InitializeConstructor(); //글로벌 객체 초기화

	SkyConsole::Initialize(); //화면에 문자열을 찍기 위해 초기화한다.

	SkyConsole::Print("Hello World!!\n");

	for (;;); //메인함수의 진행을 막음, 루프
}