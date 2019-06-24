# 4장 하드웨어 초기화 (CPU-IDT)

![Image](https://i.imgur.com/oCtWyvK.png)

## IDT(*Interrput Descriptor Table*)

IDT는 소프트웨어 예외가 발생하거나 하드웨어 인터럽트가 발생할 때,<br>이를 처리하기 위한 서비스 루틴을 기술한 디스크립터의 모음 테이블이다.

> 키보드 입력이나 프로그램에서 0으로 나누는 연산 등을 수행했을 때 CPU는 인터럽트 서비스 루틴을 실행하기 위해 이 IDT를 참조한다.<BR>

> **개발자가 생성한 코드를 실행하기 위해 필요한 디스크립터 테이블이 GDT** 라고 한다면,<BR>**하드웨어 관련 처리를 수행하기 위해 필요한 디스크립터 테이블은 IDT** 라고 보면 된다.

IDT의 구조 및 해당 인터럽트 서비스 루틴을 찾아가는 과정은 다음과 같다.

![image](https://user-images.githubusercontent.com/34773827/59986670-b7775500-9672-11e9-87d9-c31633a4e5a4.png)

> 인터럽트가 발생했다고 가정할 때,
>
> 1. CPU는 IDT에서 인터럽트 디스크립터를 찾는다.
>
> 이 디스크립터에는 GDT의 디스크립터 인덱스, 즉 세그먼트 셀렉터 값과 세그먼트의 베이스 어드레스에서 해당하는 ISR(인터럽트 서비스 루틴)까지의 Offset 값이 들어있다.
>
> 2. (1)의 Offset 값과 GDT의 디스크립터에서 얻은 세그먼트 베이스 주소를 더하면 ISR 주소를 구할 수 있다.



### 인터럽트 서비스

인터럽트 번호는 처음 *0X1F* 까지는 예외 핸들러를 위해 할당되어 있으며,<BR>*0X1F* 보다 큰 값에는 소프트웨어 인터럽트 서비스 루틴을 위한 번호로 할당할 수 있다.

| 인터럽트 번호 | 인터럽트 내용                 | 비고                                             |
| ------------- | ----------------------------- | ------------------------------------------------ |
| 0x00          | Division by Zero              | 0으로 나누기                                     |
| 0x01          | Debugger                      | 디버거                                           |
| 0x02          | NMI                           | NMI                                              |
| 0x03          | BreakPoint                    | 브레이크 포인트                                  |
| 0x04          | Overflow                      | 오버 플로우                                      |
| 0x05          | Bounds                        |                                                  |
| 0x06          | Invalid Opcode                | 유효하지 않은 OPCODE                             |
| 0x07          | Coprocessor not available     | 보조 프로세서 이용할 수 없음                     |
| 0x08          | Double Fault                  | 더블 폴트                                        |
| 0x09          | Coprocessor Segment Overrun   | 보조 프로세서 세그먼트 오버런                    |
| 0x0A          | Invalid Task State Segment    | 유효하지 않은 TSS                                |
| 0x0B          | Segment not present           | 세그먼트가 존재하지 않음                         |
| 0x0C          | Stack Fault                   | 스택 폴트                                        |
| 0x0D          | General Protection Fault      | 일반 보호 오류                                   |
| 0x0E          | Page Fault                    | 페이지 폴트                                      |
| 0x0F          | Reserved                      | 예약                                             |
| 0x10          | Math Fault                    |                                                  |
| 0x11          | Alignment Check               | 정렬 체크                                        |
| 0x12          | Machine Check                 | 머신 체크                                        |
| 0x13          | SIMD Floating-Point Exception | SIMD(Single Instruction Multiple Data) 실수 예외 |

인터럽트는 **하드웨어 인터럽트** 와 **예외 인터럽트** 로 나뉘며,<br>**예외 인터럽트**는 트랩(trab), 폴트(fault), abort 세 가지 유형으로 나뉜다.

- **폴트**

  이 예외 인터럽트가 발생하면 시스템이 망가지지 않았다고 판단하며,<br>예외 처리를 통해서 시스템을 복구할 1차 기회를 제공한다.<br>예외 핸들러 수행이 끝나면 문제가 있던 코드로 복귀해서 다시 해당 코드를 수행한다.

  ![image](https://user-images.githubusercontent.com/34773827/59988588-88fb7900-9676-11e9-96cb-e65cafba8540.png)

- **abort**

  폴트와 같이 문제가 발생하면 호출되지만 프로세스가 망가져서 시스템을 복구할수 없음을 의미한다.<br>이런 경우 윈도우 운영체제에서는 프로세스를 더 이상 수행시키지 않고 종료시킨다.

  ![image](https://user-images.githubusercontent.com/34773827/59988688-f7403b80-9676-11e9-909d-d3a1886aeea1.png)

- **트랩**

  소프트웨어 인터럽트라고도 하며 의도적으로 인터럽트를 발생시킨 경우에 해당한다.<br>이 경우 예외처리를 수행하고 나서 복귀할 경우 예외가 발생했던 명령어의 **다음 명령어부터** 코드를 실행한다.

   ![image](https://user-images.githubusercontent.com/34773827/59988851-936a4280-9677-11e9-9902-03c0bb5fa6b8.png)



### IDT 설정 관련 구조체

IDT를 구성하는 인터럽트 디스크립터의 구조는 다음과 같다.

![image](https://user-images.githubusercontent.com/34773827/59989095-a3ceed00-9678-11e9-9809-0dc6e1a62cc8.png)

> 인터럽트 디스크립터는 크기가 8Byte이며,
>
> 이중 2Byte를 차지하는 **세그먼트 셀렉터**를 통해서 GDT내 글로벌 디스크립터를 찾을 수 있다.
>
> 찾아낸 **글로벌 디스크립터**로 세그먼트의 베이스 주소를 얻을 수 있는데,<br>여기에 Offset 값을 더하면 **ISR(*interrupt service routine*)**의 주소를 얻을 수 있다.

다음은 인터럽트 디스크립터를 나타내는 구조체이다.

```c++
typedef struct tag_idt_descriptor
{
    USHORT	offsetLow;	// 인터럽트 핸들러 주소의 0 ~ 16 bit
    USHORT	selector;	// GDT의 코드 셀렉터
    BYTE	reversed;	// 예약된 값 (0이어야 한다.)
    BYTE	flags;		// 8bit 비트플래그
    USHORT	offsetHigh;	// 인터럽트 핸들러 주소의 16 ~ 32 bit
}idt_descriptor;
```

> **offsetLow, offsetHigh** 필드
>
> - ISR의 4Byte Offset 값을 얻어낼 수 있다.
>
> **selector** 필드
>
> - selector필드를 이용해서 GDT의 디스크립터를 찾아 세그먼트 베이스 주소를 얻어낼 수 있다.

위 작업을 진행하려면 우선 IDT(인터럽트 디스크립터 테이블)를 구축하고 난 뒤,<br>이 위치를 CPU에 알려주기 위한 IDTR 구조체를 만들어야 한다.

> CPU는 IDTR 레지스터를 통해서 IDT에 접근한다.

```c++
typedef struct tag_idtr
{
	USHORT	limit;
	UINT	base;
}idtr;
```

> 메모리에 위 형식의 구조체를 선언하고 값을 설정한 뒤,<br>IDTR 레지스터에 이 값을 로드하면 이후부터는 인터럽트 발생 시 해당 인터럽트 관련 서비스 루틴을 호출할 수 있다.



### IDT 관련 함수

IDT 설정 함수

```C++
idt_descriptor* GetInterruptDescriptor(uint32_t i);
bool InstallInterruptHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER);
bool IDTInitialize(uint16_t codeSel);
static struct idt_descriptor _idt[I86_MAX_INTERRUPTS];
static struct idtr	_idtr;
void IDTInstall();
```

각 함수의 역할은 다음과 같다.

| IDT 클래스 함수         | 설명                                                         |
| ----------------------- | ------------------------------------------------------------ |
| GetInterruptDescriptor  | 특정 디스크립터의 값을 얻어온다.                             |
| InstallInterruptHandler | 인터럽트 서비스 루틴을 설치한다.                             |
| IDTInitialize           | IDT를 초기화한다.                                            |
| IDTInstall              | CPU에 IDT의 위치를 알려준다.                                 |
| _idt                    | IDT를 나타낸다.<br>총 256개의 인터럽트 디스크립터가 존재하며,<br>처음 디스크립터는 항상 NULL로 설정한다. |
| _idtr                   | idtr 레지스터에 로드될 값.<br>IDT의 메모리 주소 및 IDT의 크기를 담고 있다. |



다음은 IDT를 초기화하는 함수이다.

```c++
// IDT를 초기화하고 디폴트 핸들러를 등록한다.
bool IDTInitialize(uint16_t codeSel)
{
    
    // IDTR 레지스터에 로드될 구조체 초기화
    _idtr.limit = sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1;
    _idtr.base = (uint32_t)&_idt[0];
    
    // NULL 디스크립터
    memset((void*)&_idt[0],0,sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1);
    
    // 디폴트 핸들러 등록
    for( int i = 0; i < I86_MAX_INTERRUPTS; i++)
    {
        InstallInterrputHandler(
            i,
            I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32,
            codeSel,
            (I86_IRQ_HANDLER)InterrputDefaultHandler // Default 핸들러 등록
        );
    }
    
    // IDTR 레지스터를 셋업한다.
    IDTInstall();
    
    return true;
}
```

> 현재 시점에서는 예외에 대한 고유 핸들러를  등록하지 않고 모든 예외에서 공통으로 사용되는 핸들러를 등록했다. ( ***InterruptDefaultHandler*** )

InterruptDefaultHandler의 코드는 다음과 같다.

```c++
//다룰 수 있는 핸들러가 존재하지 않을 때 호출되는 기본 핸들러
__declspec(naked) void InterruptDefaultHanlder()
{
    // 레지스터를 저장하고 인터럽트를 종료한다.
    _asm
    {
        PUSHAD
        PUSHFD
        CLI
    }
    SendEOI();
    // 레지스터를 복원하고 원래 수행했던 곳으로 돌아가게한다.
    _asm
    {
        POPFD
        POPAD
        IRETD
    }
}
```

> 예외처리를 하는 도중에 예외가 발생할 수 있으므로 이를 차단하기 위해서 인터럽트가 발생하지 않도록 어셈블리 명령어 CLI를 호출한다.
>
> 위의 프로시저는 어떠한 액션도 취하지 않고 핸들러 수행을 끝내는 코드다. (어셈블리는 나중에..)

IDT는 시스템에서 예외가 발생했을 때 이를 처리할 수 있는 서비스 루틴을 제공해주는 게이트 역할을 한다.

> 특히 응용프로그램에서 에러가 발생하면 예외 핸들러를 통해서 어떤 주소에서 예외가 발생했는지 확인할 수 있고, 프로세스 덤프를 생성하는 것도 가능하게 해주어 프로그램의 버그를 쉽게 수정할 수 있도록 한다.
>
> 비주얼 스튜디오에서 프로그램을 작성하고 디버거를 실행한 뒤 프로그램에서 버그가 발생하면 디버거에서 에러가 발생한 곳을 감지할 수 있는데, 이런 처리가 가능한 이유가 IDT를 통해서 설정한 예외 핸들러 덕분이다.



<hr>

<hr>

## 정리

하드웨어의 보호모드 기능을 사용하기 위해서는 **글로벌 디스크립터 테이블(GDT)**와 **인터럽트 디스크립터 테이블(IDT)**을 정의해야 한다.

**글로벌 디스크립터**는 스레드가 접근할 수 있는 주소 범위의 제한과 권한을 정의한다.

**인터럽트 디스크립터**는 예외가 발생할 때 이를 처리할 수 있는 예외 핸들러의 위치에 관해 기술한다.

CPU는 이 GDT와 IDT의 위치를 알고 있어야 하며 이 두 테이블의 위치는 각각 **GDTR, IDTR** 두 레지스터를 통해 확인할 수 있다.

> 우리는 이 GDT와 IDT를 만들어서 CPU에게 알려주어야 한다.

| 테이블 | 역할                                                         |
| ------ | ------------------------------------------------------------ |
| GDT    | 커널, 유저 애플리케이션에 권한을 부여해 서로간 침범을 방지<br>프로세스의 잘못된 영역 침범 방지 |
| IDT    | 하드웨어나 소프트웨어의 인터럽트(예외)를 처리하기 위해 필요  |

GDT와 IDT 의 설정으로 기본적인 하드웨어 초기화는 끝났으나, 추가로 세 가지 작업을 해주어야 한다.

| 항목            | 내용                                                         |
| --------------- | ------------------------------------------------------------ |
| PIC             | CPU에 하드웨어 인터럽트 신호를 보낸다.<br>PIC 활성화를 통해 키보드 이벤트나 마우스 이벤트 등을 CPU가 인지하도록 한다. |
| PIT             | 특정 주기로 타이머 이벤트를 발생시킨다.<br>PIT 활성화를 통해 멀티태스킹이 가능해진다. |
| 인터럽트 핸들러 | 각각의 인터럽트 종류에 따른 핸들러를 구현해 주어야 한다.     |

> 위 세 가지 항목을 구현하면 기본적인 프로그래밍을 할 수 있는 여건이 마련된다.