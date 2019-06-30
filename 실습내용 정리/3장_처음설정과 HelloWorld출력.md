

# 3장 직접 프로젝트 만들기

![Image](https://i.imgur.com/oCtWyvK.png)



## 프로젝트 생성 및 설정

- 왼쪽 상단 파일 -> 추가 -> 기존프로젝트 CommonLib.vcxproj 선택후 추가한뒤 빌드

- 빈프로젝트 생성
  

프로젝트 -> 속성

  - 일반탭
  
    > ![Image](https://i.imgur.com/fYYrpZF.png)
    >
    > 정리할 때 삭제할 확장명 : 
    >
    > ``*.exp*.obj%3b*.ilk%3b*.tlb%3b*.tli%3b*.tlh%3b*.tmp%3b*.rsp%3b*.pgc%3b*.pgd%3b$(TargetPath);$(ExtensionsToDeleteOnClean)``
  
  - C/C++ 
  
    > 일반
    >
    > ![Image](https://i.imgur.com/Te7okQV.png)
    >
    > 추가 포함 디렉터리는 디렉터리 경로에 따라 ./,../ 를 수정하여 설정
    >
    > > 상대 경로의 현재 작업디렉토리는 현재 프로젝트 파일(.vcxproj)이 있는 곳 부터 시작
    >
    > 디버그 정보 형식 : **프로그램 데이터베이스(/Zi)**  pdb 생성
  
    > 최적화
    >
    > ![Image](https://i.imgur.com/HyQCeI5.png)
  
    > 전처리기
    >
    > ![Image](https://i.imgur.com/Pm0pVIK.png)
  
    > 코드 생성
    >
    > ![Image](https://i.imgur.com/ua3miq5.png)
  
    > 언어![Image](https://i.imgur.com/pu1WMuq.png)
  
    > 고급
    >
    > ![Image](https://i.imgur.com/RMvwndq.png)
  
  - 링커
  
    > 일반
    >
    > ![Image](https://i.imgur.com/HKhv2WG.png)
  
    > 입력
    >
    > ![Image](https://i.imgur.com/27oQyXd.png)
  
    > 매니페스트 파일
    >
    > ![Image](https://i.imgur.com/ppbbT3T.png)
  
    > 디버깅
    >
    > ![Image](https://i.imgur.com/HzHdu5O.png)
  
    > 시스템
    >
    > ![Image](https://i.imgur.com/gHEo37B.png)
  
    > 최적화
    >
    > ![Image](https://i.imgur.com/z0bXxum.png)
  
    > 고급
    >
    > ![Image](https://i.imgur.com/lhWETAi.png)
  
    > 명령줄
    >
    > ![Image](https://i.imgur.com/iABg3Gn.png)

- MultiBoot.h 작성

  ```c++
  /* MultiBoot.h */
  #pragma once
  #include "windef.h"
  
  /*
  dd(x) 매크로 정의
  _emit 은 DB(Define Byte - 8bit)에 대하여 정의되어 있지만,
  DD(Define Double World-32bit)에 대하여 정의되어 있지 않으므로 매크로 함수로 정의해준다.
  _emit [Byte] 형은 인자로 넘겨진 바이트를 직접 기계어 코드로 삽입한다.
  */
  #define dd(x)						\
  	__asm 	_emit (x)		& 0xff	\
  	__asm 	_emit (x) >> 8 	& 0xff 	\
  	__asm	_emit (x) >> 16 & 0xff	\
  	__asm 	_emit (x) >> 24 & 0xff	
  
  /* ? */
  #define KERNEL_STACK				0X00400000
  #define FREE_MEMORY_SPACE_ADDRESS	0x00400000
  
  /* 
  ALIGN 은 GRUB으로 PE 커널을 로드할 수 있게 해주는 가장 중요한 것중 하나이다.
  PE header가 파일 시작 부분에 있으므로 코드 섹션이 이동되기 때문이다.
  코드 섹션을 이동시키는데 사용되는 값(0x400)은 linker 정렬 옵션인 /ALIGN:value 이며,
  헤더 사이즈의 합계가 512보다 크므로(최소 512이상), ALIGN 값은 그보다 더 커야 한다.
  */
  #define ALIGN 0X400
  
  /*
  GRUB이 0 ~ 1MB(0x100000)을 사용하기 때문에
  커널이 로드되는 주소는 1MB(0x100000)이상이어야 한다.
  */
  #define KERNEL_LOAD_ADDRESS	0x100000
  
  /*
  PE header를 제외한 코드섹션 선두의 주소
  */
  #define HEADER_ADRESS	KERNEL_LOAD_ADDRESS+ALIGN
  
  /*
  GRUB이 커널 함수 시작 지점을 호출하기 위해 커널은 특정 형태를 준수해야한다.
  GRUB은 커널이 특정 형태를 준수하고 있는지 여부를
  커널 파일에서 최초 80KB 부분을 검색해 특정 시그니처를 찾아냄으로서 확인한다.
   - 따라서 시그니처를 파일의 80KB 이내에 배치하는 작업이 필요하다.(MULTIBOOT_HEADER 구조체)
  GRUB은 MULTIBOOT_HEADER 구조체 내의 MULTIBOOT_HEADER_MAGIC 시그니처를 찾고 이 값이 자신의 정의한 값(0x1BADB002)와 같은지 확인한다.
  - 확인 뒤에는 이 파일이 커널임을 확정 짓고, 멀티부트 헤더 값중 엔트리 주소값을 담은 멤버값을 읽어와 해당 주소로 점프한다.
  */
  #define MULTIBOOT_HEADER_MAGIC		0x1BADB002
  #define MULTIBOOT_BOOTLOADER_MAGIC	0x2BADB002
  #define MULTIBOOT_HEADER_FLAGS		0x00010003
  #define STACK_SIZE					0x4000
  #define	CHECKSUM					-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
  
  #pragma pack(push,1) // 구조체 정렬 크기를 1Byte로 조절한다.
  
  struct MULTIBOOT_HEADER
  {
      uint32_t magic; // 32bit(4byte) unsigned int
      uint32_t flags;	
      uint32_t checksum;
      uint32_t header_addr;
      uint32_t load_addr; 
      uint32_t load_end_addr; 
      uint32_t bss_end_addr;  
      uint32_t entry_addr;	
  /*
      entry_addr 은 kernel_entry로 점프하는 값(HEADER_ADDRESS + 0x20)을 가진다.
  - 즉, MULTIBOOT_HEADER 구조체 시작으로부터 0x20 바이트 떨어진 곳에 kernel_entry 레이블이 있다.
  */
  };
  
  /*
  ELF = Excutable and Linkable Format
  실행파일, 목적파일, 공유 라이브러리 그리고 코어 덤프(작업중인 메모리 상태를 기록)를 위한 표준 파일 형식 (나중에 나온다.)
  */
  struct ELFHeaderTable
  {
      uint32_t num;
      uint32_t size;
      uint32_t addr;
      uint32_t shndx;
  };
  typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;
  
  /*
  a.out - (assembler output)
  과거 유닉스 계통 운영 체제에서 사용하던 실행 파일과 목적파일 형식. 이후에는 공유 라이브러리 형식으로도 사용되었다. 이후 ELF로 대체함.
  aout-symbol-table
  링크 에디터에 의해서 바이너리 파일 사이에서 이름 지어진 변수나 함수의 주소들의 기록
  */
  struct AOUTSymbolTable
  {
      unsigned int tabsize;
      unsigned int strsize;
      unsigned int addr;
      unsigned int reserved;
  };
  
  /*
  ...
  */
  
  
  /*
  GRUB은 보호 모드로 전환한 후 커널을 호출 하는데, 이렇게 되면
  커널 입장에서는 하드웨어 관련 정보를 얻기가 어려워진다.
  따라서 GRUB이 커널에 넘겨주는  multiboot_info 구조체를 활용해서
  메모리 사이즈, 부팅된 디바이스 정보에 대해 알아낼 수 있으며,
  OS는 GRUB이 넘겨준 이 구조체를 활용해 시스템 환경을 초기화한다.
  */
  struct multiboot_info
  {
      // 플래그 : 플래그 값을 확인해서 VESA 모드가 가능한지 여부를 확인할 수 있다.
      // VESA(비디오 전자공학 표준 : Video Electronics Standards Association)
      uint32_t flags;
      
      // 바이오스로부터 얻은 이용 가능한 메모리 영역 정보
      uint32_t mem_lower;
      uint32_t mem_upper;
      
      // 부팅 디바이스의 번호
      uint32_t boot_device;
      // 커널에 넘기는 command Line
      char* cmdline;
      
      // 부팅 모듈 리스트
      uint32_t mods_count;
      Module* Modules;
      
      // 리눅스 파일과 관계된 정보
      union
      {
          AOUTSymbolTable AOUTTable;
          ELFHeaderTable ELFTable;
      } SymbolTables;
      
      // 메모리 매핑 정보를 알려준다.
      // 이 정보를 통해 메모리 특정 블록을 사용할 수 있는지 파악 가능하다.
      uint32_t mmap_length;
      uint32_t mmap_addr;
      
      // 해당 PC에 존재하는 드라이브에 대한 정보
      uint32_t drives_length;
      drive_info* drives_addr;
      
      // ROM configuration table
      ROMConfigurationTable* ConfigTable;
      
      // 부트로더 이름
      char* boot_loader_name;
      
      // APM table
      APMTable* APMTable;
      
      // 비디오
      VbeInfoBlock* vbe_control_info;
      VbeModeInfo* vbe_mode_info;
      uint16_t vbe_mode;
      uint16_t vbe_interface_seg;
      uint16_t vbe_interface_off;
      uint16_t vbe_interface_len;
  };
  typedef struct multiboot_info multiboot_info_t;
  
  #pragma pack(pop) // 정렬을 원래의 상태로 돌림
  ```

  

- kmain.h , kmain.cpp

  ```c++
  /* kmain.h */
  #pragma once
  #include "stdint.h"
  #include "defines.h"
  #include "string.h"
  #include "sprintf.h"
  #include "MultiBoot.h"
  
  // skyconsole 을 생성하고 나서
  //#include "SkyConsole.h"
  
  void kmain(unsigned long, unsigned long);
  ```

  ```c++
  /* kmain.cpp */
  #include "kmain.h"
  
  //multiboot_entry함수는 항상(0x100400)에 배치된다.
  _declspec(naked) void multiboot_entry(void)
  {
      __asm{
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
          // 커널 시작 주소 : 멀티부트 헤더 주소 + 0x20, kernel_entry
  		dd(HEADER_ADRESS + 0x20);	
  
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
      // 나중에..
  }
  
  // 위 multiboot_entry에서 ebx eax에 푸쉬한 멀티 부트 구조체 포인터(addr)과 매직넘버(magic)
  void kmain(unsigned long magic, unsigned long addr)
  {
      InitializeConstructor(); // 글로벌 객체의 초기화
      
      //SkyConsole 생성 후
      SkyConsole::Initialize();
      
      SkyConsole::Print("Hello World!\n");
      
      for(;;); // 메인함수의 진행을 막는다.
  }
  ```

  > **_declspec(naked) 에 관하여**
  >
  > ​	보통 함수가 호출되면 함수가 사용하게 되는 Stack 영역인 StackFrame이 형성된다.
  >
  > ​		**StackFrame**
  >
  > - 함수가 사용하게 되는 Stack 영역
  >
  > - 실행중인 함수는 자신만의 Stack 영역을 사용하게 된다.
  >
  > - 함수가 실행되면 StackFrame을 열고 함수가 종료되면 닫는다.
  >
  >   **FramePointer**
  >
  > - 함수의 StackFrame 시작위치를 가리키는 레지스터
  >
  > - 주로 StackFrame이 오픈될때 설정되며 클로즈될 때 까지 변하지 않는다.
  >
  >   **Function Prolog**
  >
  > - Stack Frame을 사용하는 경우 함수는 항상 FramePointer를 셋업하는 코드 구문으로 시작하게된다.
  >   일반적인 셋업 코드 구문은
  >
  >   ```assembly
  >   push ebp // Caller의 StackFramePointer를 백업한다.
  >   move ebp, esp // esp값을 ebp에 복사함으로써 StackFramePointer를 초기화한다.
  >   sub esp, n // 할당된 지역변수를 위한 공간으로 esp 위치를 옮겨 확보한다.
  >   ```
  >
  >   Function Epilog
  >
  > - StackFrame을 사용하는 경우 함수의 끝부분에는 FramePointer를 원래대로 복원하는 코드가 오게된다.
  >   일반적인 복원 코드 구문은
  >
  >   ```assembly
  >   // ebp 즉 고정된 ebp 값을 esp에 줌으로써 지역변수를 위해 주었던 공간을 해제한다.
  >   move esp, ebp 
  >   // ebp를 꺼내므로써 ret, call 하게 된다. 즉, 원래의 ebp 위치로 return값을 call 해온다.
  >   pop ebp 
  >   ```
  >
  > **_declspec(naked)**는 위의 Function Prolog와 Epilog를 사용하지 않는다.
  > 	컴파일러가 함수 내에서 추가 코드를 자동으로 생성하지 않도록 하기 위해 사용한다.

  > 어셈블러 함수의 일반적인 형식
  >
  > ```assembly
  > push bp	;베이스 포인터 레지스터를 스택에 삽입
  > mov bp, sp	; 메이스 포인터 레지스터에 스택 포인터 레지스터의 값을 설정
  > 			; 베이스 포인터 레지스터를 이용해서 파라미터에 접근할 목적
  > 
  > push es		; ES 세그먼트 레지스터부터 DX 레지스터까지 스택에 삽입
  > push si		; 함수에서 임시로 사용하는 레지스터로 함수의 마지막 부분에서
  > push di		; 스택에 삽입된 값을 꺼내 원래 값으로 복원
  >  ... 생략 ...
  >  
  >  함수
  >  
  >  ... 생략 ...
  >  
  >  pop dx		; 함수에서 사용이 끝난 DX 레지스터부터 ES 레지스터까지를 스택에
  >  pop cx 	; 삽입된 값을 이용해서 복원
  >  pop ax		; 스택은 가장 마지막에 들어간 데이터가 가장 먼저 나오는
  >  pop di		; 자료구조이므로 삽입의 역순으로
  >  pop si		; 제거해야 한다.
  >  
  >  pop es
  >  pop bp		; 베이스 포인터 레지스터 복원
  >  ret		; 함수를 호출한 다음 코드의 위치로 복귀
  > ```
  >
  > 

- **출력을 위한 SkyConsole** 작성

  ```c++
  /* SkyConsole.h */
  #ifndef SKYCONSOLE_H
  #define SKYCONSOLE_H
  #include "windef.h"
  
  enum ConsoleColor
  {
     ...
  };
  
  #define VGA_COLOR_CRT_ADDRESS 0x3D4
  #define VGA_MONO_CRT_ADDRESS 0x3B4
  
  #define VGA_CRT_CURSOR_START 0x0A
  #define VGA_CRT_CURSOR_END 0x0B
  #define VGA_CRT_H_START_ADDRESS 0x0C
  #define VGA_CRT_H_END_ADDRESS 0x0D
  #define VGA_CRT_CURSOR_H_LOCATION 0x0E
  #define VGA_CRT_CURSOR_L_LOCATION 0x0F
  
  /*
  현 시점에서 싱글턴 객체로 만들 수 없어 namespace로 묶어 싱글턴 느낌만 살도록함
  */
  namespace SkyConsole
  {
  	void Initialize();
  	void Clear();
  	void WriteChar(char c, ConsoleColor textColour, ConsoleColor backColour);
  	void WriteString(const char* szString, ConsoleColor textColour = White, ConsoleColor backColour = Black);
  	void Write(const char *szString);
  	void WriteChar(char c);
  
  	void Print(const char* str, ...);
  
  	void MoveCursor(unsigned int  X, unsigned int  Y);
  	void GetCursorPos(uint& x, uint& y);
  	void SetCursorType(unsigned char  Bottom, unsigned char Top);
  	void scrollup();
  
  	void SetColor(ConsoleColor Text, ConsoleColor Back, bool blink);
  	unsigned char GetBackColor();
  	unsigned char GetTextColor();
  	void SetBackColor(ConsoleColor col);
  	void SetTextColor(ConsoleColor col);
  
  	void GetCommand(char* commandBuffer, int bufSize);
  	//KEYCODE	GetChar(); 
  	char	GetChar();
  }
  #endif
  ```

  ```c++
  /* SkyConsole.cpp */
  #include "SkyConsole.h"
  #include <stdarg.h>
  #include <stdint.h>
  #include <string.h>
  #include "sprintf.h"
  /*
  _outp, outpw 및 _outpd 함수는 각각 byte, word, double word를 지정된 출력포트에 쓴다.
  이런 함수는 I/O포트에 직접 쓰기 때문에 User Code에서는 사용할 수 없다.
  extern "C"는 name mangling을 막기위해 사용
  */
  extern "C" int _outp(unsigned short, int);
  
  // 포트에 해당 값(1byte) 쓰기
  void OutPortByte(ushort port, uchar value)
  {
      _outp(port,value);
  }
  
  namespace SkyConsole
  {
  ```

  **static 변수 정리 (싱글턴 처럼 사용하기 위해)**

  ```c++
  static ConsoleColor m_Color;
  static ConsoleColor m_Text;
  static ConsoleColor m_backGroundColor;
  
  static int m_xPos;
  static int m_yPos;
  
  static ushort* m_pVideoMemory;
  static unsigned int m_ScreenHeight;
  static unsigned int m_ScreenWidth;
  static unsigned short m_VideoCardType;
  ```

  **Initialize() - 초기화**

  ```c++
  void Initialize()
  {
      // 비디오카드가 VGA인지 흑백인지 확인한다.
      char c = (*(unsigned short*)0x410 & 0x30); 
      
      if(c == 0x30) // VGA이면 0x00이나 0x20이고 흑백이면 0x30이다.
      {
          // 흑백일 경우 비디오 메모리 주소는 0xb0000이다.
          m_pVideoMemory = (unsigned short*)0xb0000;
          m_VideoCardType = VGA_MONO_CRT_ADDRESS; // mono
      }
      else
      {
          // 컬러일 경우 비디오 메모리의 주소가 0xb8000이다.
          m_pVideoMemory = (unsigned short*)0xb8000;
          m_VideoCardType = VGA_COLOR_CRT_ADDRESS;// color
      }
      
      // 화면 사이즈 80 * 25
      m_ScreenHeight = 25;
      m_ScreenWdith = 80;
      
      // 커서 위치 초기화, 문자색은 흰색으로 배경은 검은색으로 설정
      m_xPos = 0;
      m_yPos = 0;
      m_Text = White;
      m_backGroundColor = Black;
      m_Color = (ConsoleColor)((m_backGroundColor << 4) | m_Text);
      
      Clear();
  }
  ```

  **Print() 함수**

  ```c++
  void Print(const char* str, ...)
  	{
  
  		if (!str)
  			return;
  
  		va_list		args;
  		va_start(args, str);
  		size_t i;
  
  		for (i = 0; i < strlen(str); i++) {
  
  			switch (str[i]) {
  
  			case '%':
  
  				switch (str[i + 1]) {
  
  				/*** 문자 ***/
  				case 'c': {
  					char c = va_arg(args, char);
  					WriteChar(c, m_Text, m_backGroundColor);
  					i++;		// go to next character
  					break;
  				}
  
  				/*** 문자열 ***/
  				case 's': {
  					const char * c = (const char *&)va_arg(args, char);
  					char str[256];
  					strcpy(str, c);
  					Write(str);
  					i++;		// go to next character
  					break;
  				}
  
  				/*** 정수 ***/
  				case 'd':
  				case 'i': {
  					int c = va_arg(args, int);
  					char str[32] = { 0 };
  					itoa_s(c, 10, str);
  					Write(str);
  					i++;		// go to next character
  					break;
  				}
  
  				/*** 16진수 ***/
  				/*int*/
  				case 'X': {
  					int c = va_arg(args, int);
  					char str[32] = { 0 };
  					itoa_s(c, 16, str);
  					Write(str);
  					i++;		// go to next character
  					break;
  				}
  				/*unsigned int*/
  				case 'x': {
  					unsigned int c = va_arg(args, unsigned int);
  					char str[32] = { 0 };
  					itoa_s(c, 16, str);
  					Write(str);
  					i++;		// go to next character
  					break;
  				}
  // 파라미터에 해당하지 않는 경우 특별한 처리 없이 문자를 화면에 찍는다.
  				default:
  					va_end(args);
  					return;
  				}
  
  				break;
  
  			default:
  				WriteChar(str[i], m_Text, m_backGroundColor);
  				break;
  			}
  
  		}
  
  		va_end(args);
  	}
  ```

  > 가변인자 처리 함수
  >
  > - va_start : 가변인자 처리의 시작을 알린다.
  > - va_arg : 가변인자를 처리한다.
  > - va_end : 가변인자 처리를 완료한다.
  > - itoa_s : 정수값을 아스키값으로 변환한다.

  **실제로 문자를 출력하는 Write 와 WriteChar, WriteString**

  ```c++
  void WriteChar(char c)
  {
      WriteChar(c, m_Text, m_backGroundColor);
  }
  
  void WriteChar(char c, ConsoleColor textColour, ConsoleColor backColour)
  {
      int t;
      switch (c)
      {
          case '\r':                         //-> carriage return
              m_xPos = 0;
              break;
  
          case '\n':                         // -> newline (with implicit cr)
              m_xPos = 0;
              m_yPos++;
              break;
  
          case 8:                            // -> backspace
              t = m_xPos + m_yPos * m_ScreenWidth;    // get linear address
              if (t > 0) t--;
              // 커서가 화면 왼쪽에 도달하지 않았을 경우에만 커서값을 감소시킨다.
              if (m_xPos > 0)
              {
                  m_xPos--;
              }
              else if (m_yPos > 0)
              {
                  m_yPos--;
                  m_xPos = m_ScreenWidth - 1;
              }
  			// 커서 위치에 있었던 문자를 지운다.
              *(m_pVideoMemory + t) = ' ' | ((unsigned char)m_Color << 8); 
              break;
  		
           // 아스키 문자가 아니면 모두 무시한다.
          default:		 
              if (c < ' ') break;				
              
              //See the article for an explanation of this. Don't forget to add support for new lines
              /*
  			비디오 메모리 영역에 값을 직접 써넣는다!
  			m_pVideoMemory 포인터는 비디오 버퍼를 가리키는 포인터로,
  			이 주소에 접근해서 값을 써 넣으면 화면에 그 값을 출력할 수 있다.
  			*/
              ushort* VideoMemory = m_pVideoMemory + m_ScreenWidth * m_yPos + m_xPos;
              uchar attribute = (uchar)((backColour << 4) | (textColour & 0xF));
  
              *VideoMemory = (c | (ushort)(attribute << 8));
              m_xPos++;
              break;
      }
  
      // 커서의 X 좌표가 화면 너비 이상의 커서 Y 좌표값을 증가시킨다.
      if (m_xPos >= m_ScreenWidth)
          m_yPos++;
  
      // 커서 Y 좌표가 화면 다음에 도달하면 화면을 스크롤시킨다.
      if (m_yPos == m_ScreenHeight)			
      {
          scrollup();			// scroll the screen up
          m_yPos--;			// and move the cursor back
      }
      
  	// 계산된 커서 좌표를 이용해서 커서를 적절한 위치로 옮긴다.
      MoveCursor(m_xPos + 1, m_yPos);
  }
  
  void Write(const char *szString)
  	{
  		while ((*szString) != 0)
  		{
  			WriteChar(*(szString++), m_Text, m_backGroundColor);
  		}
  	}
  
  void WriteString(const char* szString, ConsoleColor textColour, ConsoleColor backColour)
  	{
  		if (szString == 0)
  			return;
  
  		ushort* VideoMemory = 0;
  
  		for (int i = 0; szString[i] != 0; i++)
  		{
  			VideoMemory = m_pVideoMemory + m_ScreenWidth * m_yPos + i;
  			uchar attribute = (uchar)((backColour << 4) | (textColour & 0xF));
  
  			*VideoMemory = (szString[i] | (ushort)(attribute << 8));
  		}
  
  		m_yPos++;
  		MoveCursor(1, m_yPos);
  	}
  ```

  **나머지 함수들**

  ```c++
  void Clear()
  	{
  
  		for (uint i = 0; i < m_ScreenWidth * m_ScreenHeight; i++)				//Remember, 25 rows and 80 columns
  		{
  			m_pVideoMemory[i] = (ushort)(0x20 | (m_Color << 8));
  		}
  
  		MoveCursor(0, 0);
  	}
  
  	void GetCursorPos(uint& x, uint& y) { x = m_xPos; y = m_yPos; }
  
  	void MoveCursor(unsigned int  X, unsigned int  Y)
  	{
  		if (X > m_ScreenWidth)
  			X = 0;
  		unsigned short Offset = (unsigned short)((Y*m_ScreenWidth) + (X - 1));
  
  		OutPortByte(m_VideoCardType, VGA_CRT_CURSOR_H_LOCATION);
  		OutPortByte(m_VideoCardType + 1, Offset >> 8);
  		OutPortByte(m_VideoCardType, VGA_CRT_CURSOR_L_LOCATION);
  		OutPortByte(m_VideoCardType + 1, (Offset << 8) >> 8);
  
  		if (X > 0)
  			m_xPos = X - 1;
  		else
  			m_xPos = 0;
  
  		m_yPos = Y;
  	}
  	/* Sets the Cursor Type
  		0 to 15 is possible value to pass
  		Returns - none.
  		Example : Normal Cursor - (13,14)
  			  Solid Cursor - (0,15)
  			  No Cursor - (25,25) - beyond the cursor limit so it is invisible
  	*/
  	void SetCursorType(unsigned char  Bottom, unsigned char  Top)
  	{
  
  	}
  
  	void scrollup()		// scroll the screen up one line
  	{
  		unsigned int t = 0;
  
  		//	disable();	//this memory operation should not be interrupted,
  						//can cause errors (more of an annoyance than anything else)
  		for (t = 0; t < m_ScreenWidth * (m_ScreenHeight - 1); t++)		// scroll every line up
  			*(m_pVideoMemory + t) = *(m_pVideoMemory + t + m_ScreenWidth);
  		for (; t < m_ScreenWidth * m_ScreenHeight; t++)				//clear the bottom line
  			*(m_pVideoMemory + t) = ' ' | ((unsigned char)m_Color << 8);
  
  		//enable();
  	}
  
  	void SetTextColor(ConsoleColor col)
  	{						//Sets the colour of the text being displayed
  		m_Text = col;
  		m_Color = (ConsoleColor)((m_backGroundColor << 4) | m_Text);
  	}
  
  	void SetBackColor(ConsoleColor col)
  	{						//Sets the colour of the background being displayed
  		if (col > LightGray)
  		{
  			col = Black;
  		}
  		m_backGroundColor = col;
  		m_Color = (ConsoleColor)((m_backGroundColor << 4) | m_Text);
  	}
  
  	unsigned char GetTextColor()
  	{						//Sets the colour of the text currently set
  		return (unsigned char)m_Text;
  	}
  
  	unsigned char GetBackColor()
  	{						//returns the colour of the background currently set
  		return (unsigned char)m_backGroundColor;
  	}
  
  	void SetColor(ConsoleColor Text, ConsoleColor Back, bool blink)
  	{						//Sets the colour of the text being displayed
  		SetTextColor(Text);
  		SetBackColor(Back);
  		if (blink)
  		{
  			m_Color = (ConsoleColor)((m_backGroundColor << 4) | m_Text | 128);
  		}
  	}
  }
  ```

  

