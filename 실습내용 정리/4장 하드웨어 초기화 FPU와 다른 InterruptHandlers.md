# 4장 하드웨어 초기화 FPU와 다른 Interrupt Handlers



## 1. FPU 초기화와 FLOAT 출력 테스트

### 1. FPU.h

```c++
#pragma once

bool InitFPU();
bool EnableFPU();
```

### 2. FPU.cpp

```c++
#include "FPU.h"

extern "C" void __writecr4(unsigned __int64 Data);
extern "C" unsigned long __readcr4(void);

// FPU가 존재하는지 여부를 판단한다.
bool InitFPU()
{
	int result = 0;
	unsigned short temp;

	/*
	CR(Control Register)
	운영 모드를 변경, 현재 운영 중인 모드의 특정 기능을 제어하는 레지스터
	x86 프로세서에는 CR0~CR4 5개의 컨트롤 레지스터가 있으며
	x86-64 프로세서에는 CR8 추가

	CR0 - 운영모드를 제어하는 레지스터, 리얼모드에서 보호모드로 전환하는 역할, 캐시, 페이징 기능 등을 활성화
	CR1 - 프로세서에 의해 예약됨
	CR2 - 페이징 폴트 발생 시 페이지 폴트가 발생한 가상 주소가 저장되는 레지스터(페이징 활성화 후, 페이지 폴트시에만 유효)
	CR3 - 페이지 디렉터리의 물리 주소와 페이지 캐시에 관련된 기능을 설정
	CR4 - 프로세서에서 지원하는 확장 기능을 제어
		  페이지 크기 화장, 메모리 영역 확장 등의 기능을 활성화
	CR8 - 테스크 우선 순위 레지스터의 값을 제어,
		  프로세스 외부에서 발생하는 인터럽트 필터링, IA-32e 모드에서는 64비트로 확장
	*/
	__asm
	{
		pushad;			// 모든 레지스터를 스택에 저장한다.
		mov eax, cr0;	// eax = CR0
		and al, ~6;		// EM과 MP 플래그를 클리어한다. ~0110 => 1001
		mov cr0, eax;	// eax에 저장된 값을 cr0 레지스터에 저장한다.
		fninit; FPU;	// 상태를 초기화 한다.
		mov temp, 0x5A5A;	// FPU의 상태를 저장할 임시변수값을 0이 아닌 값으로 설정한다.
		fnstsw temp;		// FPU의 상태를 얻어온다.
		cmp temp, 0;		// 상태값이 0이면 FPU가 존재하지 않는다.
		jne noFPU;			// FPU가 존재하지 않으니 noFPU 레이블로 점프한다.
		fnstcw temp;		// FPU 제어값을 임시변수에 얻어오고
		mov ax, temp;		// 얻어온 값을 ax 레지스터에 저장한다.
		and ax, 0x103F;		// ax와 0x103F AND 연산을 수행한 뒤 ax에 저장한다.
		cmp ax, 0x003F;		// ax에 저장된 값과 0x003F 비교
		jne noFPU;			// 값이 틀리다면 FPU가 존재하지 않으므로 noFPU 레이블로 점프한다.
		mov result, 1;		// 이 구문이 실행되면 FPU가 존재하는 것이다.
	noFPU:
		popad;
	}

	return result == 1;
}


bool EnableFPU()
{
#ifdef _WIN32
	unsigned long regCR4 = __readcr4();
	__asm or regCR4, 0x200;
	__writecr4(regCR4);
#else
	/*__asm
	{
	 mov eax, cr4;
	 or eax, 0x200;
	 mov cr4, eax;
	}*/
#endif
}
```

### TEST : double float형 출력

#### 3. SkyConsole::Print() 에 %f 처리구문 추가 

```c++
case 'f':
{
    double double_temp;
    double_temp = va_arg(args, double);
    char buffer[512];
    ftoa_fixed(buffer, double_temp);
    Write(buffer);
    i++;
    break;
}
```

#### 4. kMain에 TestFPU() 추가와 FPU 초기화

```c++
/*  kMain.cpp */
#include "kMain.h"
#include "FPU.h" // FPU 헤더 추가
...
   
void TestFPU()
{
	float sampleFloat = 0.3f;
	sampleFloat *= 5.482f;
	SkyConsole::Print("Sample Float Value %f\n", sampleFloat);
}

// 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(->addr), 매직넘버(->magic)로 호출
void kmain(unsigned long magic, unsigned long addr)
{
	SkyConsole::Initialize();

	SkyConsole::Print("*** MY OS Console System Init ***\n");

	// 하드웨어 초기화 과정중 인터럽트가 발생하지 않아야 하므로
	kEnterCriticalSection(&g_criticalSection);

	HardwareInitialize();
	SkyConsole::Print("Hardware Init Complete\n");
	
	kLeaveCriticalSection(&g_criticalSection);
	
    // FPU 초기화 추가
	if (false == InitFPU())
	{
		SkyConsole::Print("[Warning] Floating Pointer Unit(FPU) Detection Fail!\n");
	}
	else
	{
		EnableFPU();
		SkyConsole::Print("FPU Init...\n");
		TestFPU();
	}

	//타이머를 시작한다.
	//StartPITCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);
}
```

### FPU TEST 실행화면

![image](https://user-images.githubusercontent.com/34773827/60235610-50140c00-98e3-11e9-8ab6-5404d48f333f.png)



## 2. 다른 Interrupt Handler 추가하기

### 1. Exception.h ,cpp 추가

```c++
/* Exception.h */

#pragma once
#include "stdint.h"

// 0으로 나눌 때 오류
void kHandleDivideByZero();
...
//싱글 스텝
// non maskable interrupt trab
// 브레이크 포인트
// 오버 플로우
// Bound check
// 유효하지 않은 OPCODE 또는 명령어
// 디바이스를 이용할 수 없음
// 더블 폴트
// 유효하지 않은 TSS(Task State Segment)
// 세그먼트 존재하지 않음
// 스택 폴트
// 일반 보호 오류(General Protection Fault)
// 페이지 폴트
// Floating Point Unit(FPU) error
// alignment check
// machine check
// Floating Point Unit(FPU) Single Instruction Multiple Data (SIMD) error
    
void HaltSystem(const char* errMsg);
```

```c++
/* Exception.cpp */

#include "Exception.h"
#include "HAL.h" // interrupt(declspec(naked)) 와 register_t 정의
#include <stdint.h>
#include "SkyConsole.h"
#include "sprintf.h"
#include "string.h"

#pragma warning (disable:4100)

static char* sickpc = " (>_<) MY OS Error!\n\n";

void _cdecl kExceptionMessageHeader()
{
	char* disclamer =
		"We apologize, OS has encountered a problem and has been shut down\n\
to prevent damage to your computer. Any unsaved work might be lost.\n\
We are sorry for the inconvenience this might have caused.\n\n\
Please report the following information and restart your computer\n\
The system has been halted.\n\n";

	SkyConsole::MoveCursor(0, 0);
	SkyConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	SkyConsole::Clear();
	SkyConsole::Print(sickpc);
	SkyConsole::Print(disclamer);
}


void HandleDivideByZero(registers_t regs)
{
	kExceptionMessageHeader();
	SkyConsole::Print("Divide by 0 at Address[0x%x:0x%x]\n", regs.cs, regs.eip);
	SkyConsole::Print("EFLAGS[0x%x]\n", regs.eflags);
	SkyConsole::Print("ss : 0x%x\n", regs.ss);
	for (;;);
}

// interrupt _declspec(naked) => HAL에 정의
interrupt void kHandleDivideByZero()
{
	_asm {
		cli
		pushad 	// HandleDivideByZero의 매개변수인 registers_t 값을 채우는 것

		push ds
		push es
		push fs
		push gs
	}

	_asm
	{
		call HandleDivideByZero // 위이 HandleDivideByZero함수 호출
	}

	_asm {

		pop gs
		pop fs
		pop es
		pop ds

		popad

		mov al, 0x20
		out 0x20, al
		sti
		iretd
	}
}

interrupt void kHandleSingleStepTrap()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Single step\n");
	for (;;);
}
...
// non maskable interrupt trab
// 브레이크 포인트
// 오버 플로우
// Bound check
// 유효하지 않은 OPCODE 또는 명령어
// 디바이스를 이용할 수 없음
// 더블 폴트
// 유효하지 않은 TSS(Task State Segment)
// 세그먼트 존재하지 않음
// 스택 폴트
// 일반 보호 오류(General Protection Fault)
// 페이지 폴트
// Floating Point Unit(FPU) error
// alignment check
// machine check
// Floating Point Unit(FPU) Single Instruction Multiple Data (SIMD) error 
    
void HaltSystem(const char* errMsg)
{
	SkyConsole::MoveCursor(0, 0);
	SkyConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	SkyConsole::Clear();
	SkyConsole::Print(sickpc);

	SkyConsole::Print("*** STOP: %s", errMsg);

	__asm
	{
	halt:
		jmp halt;
	}
}
```

### HAL.h 에 정의 추가

```c++
/* HAL.h */

#ifdef _MSC_VER
#define interrupt __declspec(naked)
#else
#define interrupt
#endif

#pragma pack (push, 1)
typedef struct registers
{
	u32int ds, es, fs, gs;                  // Data segment selector
	u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pushad.
	//u32int int_no, err_code;    // Interrupt number and error code (if applicable)
	u32int eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;
#pragma pack (pop)
```

### InitKernel (SetInterruptVector()) 추가

```c++
/* InitKernel.h */
#pragma once
#include "stdint.h"
#include "MultiBoot.h"

extern void __cdecl InitializeConstructors();

void SetInterruptVector();
```

```c++
/* InitKernel.cpp */
#include "InitKernel.h"
#include "HAL.h"
#include "Exception.h"
#include "IDT.h"

void SetInterruptVector()
{
	setvect(0, (void(__cdecl&)(void))kHandleDivideByZero);
	setvect(1, (void(__cdecl&)(void))kHandleSingleStepTrap);
	setvect(2, (void(__cdecl&)(void))kHandleNMITrap);
	setvect(3, (void(__cdecl&)(void))kHandleBreakPointTrap);
	setvect(4, (void(__cdecl&)(void))kHandleOverflowTrap);
	setvect(5, (void(__cdecl&)(void))kHandleBoundsCheckFault);
	setvect(6, (void(__cdecl&)(void))kHandleInvalidOpcodeFault);
	setvect(7, (void(__cdecl&)(void))kHandleNoDeviceFault);
	setvect(8, (void(__cdecl&)(void))kHandleDoubleFaultAbort);
	setvect(10, (void(__cdecl&)(void))kHandleInvalidTSSFault);
	setvect(11, (void(__cdecl&)(void))kHandleSegmentFault);
	setvect(12, (void(__cdecl&)(void))kHandleStackFault);
	setvect(13, (void(__cdecl&)(void))kHandleGeneralProtectionFault);
	setvect(14, (void(__cdecl&)(void))kHandlePageFault);
	setvect(16, (void(__cdecl&)(void))kHandleFPUFault);
	setvect(17, (void(__cdecl&)(void))kHandleAlignedCheckFault);
	setvect(18, (void(__cdecl&)(void))kHandleMachineCheckAbort);
	setvect(19, (void(__cdecl&)(void))kHandleSIMDFPUFault);
	//...
	setvect(33, (void(__cdecl&)(void))InterrputDefaultHandler);
	setvect(38, (void(__cdecl&)(void))InterrputDefaultHandler);
}
```

### TEST : Divide by Zero 실행

#### kMain에 추가

```c++
/* kMain.h */
...
#include "InitKernel.h" // SkyConsle 아래 추가
...
```

```c++
/* kMain.cpp */
...
int _divider = 0;
int _dividend = 100;
void TestInterrupt()
{
	int result = _dividend / _divider;

	if (_divider != 0)
		result = _dividend / _divider;

	SkyConsole::Print("Result is %d, divider : %d\n", result, _divider);
}

// 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(->addr), 매직넘버(->magic)로 호출
void kmain(unsigned long magic, unsigned long addr)
{
	SkyConsole::Initialize();

	SkyConsole::Print("*** MY OS Console System Init ***\n");

	// 하드웨어 초기화 과정중 인터럽트가 발생하지 않아야 하므로
	kEnterCriticalSection(&g_criticalSection);

	HardwareInitialize();
	SkyConsole::Print("Hardware Init Complete\n");
	
	SetInterruptVector();

	kLeaveCriticalSection(&g_criticalSection);
	
	if (false == InitFPU())
	{
		SkyConsole::Print("[Warning] Floating Pointer Unit(FPU) Detection Fail!\n");
	}
	else
	{
		EnableFPU();
		SkyConsole::Print("FPU Init...\n");
		//TestFPU();
	}

	//타이머를 시작한다.
	StartPITCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);

	TestInterrupt();

	for (;;);
}
```

### TEST 실행화면

![image](https://user-images.githubusercontent.com/34773827/60241312-f7963c00-98ed-11e9-8322-271774fd6103.png)

