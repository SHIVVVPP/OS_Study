# 4장 하드웨어 초기화 (CPU-PIC,PIT,InterruptHandler)

![Image](https://i.imgur.com/oCtWyvK.png)

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



<hr>

## PIC

**PIC(*Programmable Interrupt Controller*)**는 키보드나 마우스 등의 이벤트 등을 CPU에 전달하는 제어기이다.

유저가 키보드를 누르면 PIC가 신호를 감지하고 인터럽트를 발생시켜 등록된 예외핸들러를 실행시킨다.

> 이런 이벤트는 키보드, 마우스, 플로피디스크, 하드디스크 등 다양한 매체에 의해 발생할 수 있다.

**운영체제는 다양한 매체에 의해 인터럽트가 발생 했다는 것을 감지 할 수 있어야 한다.**

> 즉, 디바이스 식별이 가능해야 한다.

![image](https://user-images.githubusercontent.com/34773827/59996536-970cc200-9695-11e9-8544-1cbadbcbecb4.png)

> 위 그림에서 알 수 있듯이 IRQ(인터럽트 리퀘스트) 신호를 통해 우리는 해당 인터럽트의 고유 번호를 알 수 있고, 이 번호를 통해 해당 인터럽트의 출처가 어디인지를 알 수 있다.

> PIC는 두 개의 모듈, 마스터와 슬레이브로 구성되며,  오른쪽 그림의 각각의 핀은 하드웨어와 연결된다.

PIC(*programmable Interrupt Controller*)와 IRQ(*Interrupt Request*)의 관계

| PIC구분 | BIT  |  IRQ  | 설명                   |
| :-----: | :--: | :---: | ---------------------- |
| Master  |  0   | IRQ0  |                        |
|    -    |  1   | IRQ1  | 타이머                 |
|    -    |  2   | IRQ2  | 키보드                 |
|    -    |  3   | IRQ3  | 슬레이브 PC            |
|    -    |  4   | IRQ4  | COM2                   |
|    -    |  5   | IRQ5  | COM1                   |
|    -    |  6   | IRQ6  | 프린터 포트2           |
|    -    |  7   | IRQ7  | 플로피 디스크 컨트롤러 |
|  Slave  |  0   | IRQ8  | 리얼타임 클럭          |
|    -    |  1   | IRQ9  | X                      |
|    -    |  2   | IRQ10 | X                      |
|    -    |  3   | IRQ11 | X                      |
|    -    |  4   | IRQ12 | PS/2 마우스            |
|    -    |  5   | IRQ13 | 보조 프로세서          |
|    -    |  6   | IRQ14 | 하드 디스크1           |
|    -    |  7   | IRQ15 | 하드 디스크2           |

PIC의 IRQ 핀의 동작

- Master ( 마스터 )
  1. 마스터 PIC에서 인터럽트가 발생한다.
  2. 마스터 PIC는 자신의 INT에 신호를 싣고 CPU의 INT에 전달한다.
  3. CPU가 인터럽트를 받으면 EFLAG의 IE bit를 1로 세팅하고 INTA를 통해 받았다는 신호를 PIC에 전달한다.
  4. PIC는 자신의 INTA를 통해 이 신호를 받고 어떤 IRQ에 연결된 장치에서 인터럽트가 발생했는지 데이터 버스를 통해 CPU로 전달한다.
  5. CPU는 현재 실행모드가 보호 모드라면 IDT 디스크립터를 찾아서 인터럽트 핸들러를 실행한다.
- Slave ( 슬레이브 )
  1. 슬레이브 PIC에서 인터럽트가 발생한다.
  2. 슬레이브 PIC는 자신의 INT핀에 신호를 싣고 마스터 PIC IRQ 2번에 인터럽트 신호를 보낸다.
  3. 마스터는 위에서 설명한 5가지의 절차를 진행한다.<BR>단, 이 과정에서 CPU에 몇 번째 IRQ에서 인터럽트가 발생했는지 알려줄 때에는 8 *~* 15번이 된다.

그리고 IRQ 하드웨어 인터럽트가 발생할 때 적절히 작동하도록 하기 위해 PIC가 가진 각 IRQ를 초기화 해주어야 한다. 이를 위해 마스터 PIC의 명령 레지스터로 명령을 전달해야 하는데 이때 **ICW(*Initialization Control Word*)**가 사용된다.

> 이 ICW는 4가지의 초기화 명령어로 구성된다.



PIC의 초기화

```C++
void PICInitialize(uint8_t base0, uint8_t base1)
{
    uint8_t	icw = 0;
    
    // PIC 초기화 ICW1 명령을 보낸다.
    icw = (icw & ~I86_PIC_ICW1_MASK_INIT) | I86_PIC_ICW1_INIT_YES;
    icw = (icw & ~I86_PIC_ICW1_MASK_IC4)  | I86_PIC_ICW1_IC4_EXPECT;
    
    SendCommandToPIC(icw,0);
    SendCommandToPIC(icw,1);
    
    // PIC에 ICW2 명령을 보낸다. base0 와 base1은 IRQ의 베이스 주소를 의미한다.
    SendDataToPIC(base0,0);
    SendDataToPIC(base1,1);
    
    // PIC에 ICW3 명령을 보낸다. 마스터와 슬레이브  PIC와의 관계를 정립한다.
    SendDataToPIC(0x04, 0);
    SendDataToPIC(0x02, 1);
    
    // ICW4 명령을 보낸다. i86 모드를 활성화한다.
    icw = (icw & ~I86_PIC_ICW4_MASK_UPM)   | I86_PIC_ICW4_UPM_86MODE;
    
    SendDataToPIC(icw,0);
    SendDataToPIC(icw,1);
    // PIC 초기화 완료
}
```

PIC와 통신하는데 쓰이는 메소드는 다음과 같다.

| 메소드           | 내용                        |
| ---------------- | --------------------------- |
| ReadDataFromPIC  | PIC로부터 1바이트를 읽는다. |
| SendDataToPIC    | PIC로 데이터를 보낸다.      |
| SendCommandToPIC | PIC로 명령어를 전송한다.    |

해당 메소드는 내부읲 PIC에 데이터를 전송하기 위해 다음과 같은 메소드를 사용한다.

| 메소드       | 내용                            |
| ------------ | ------------------------------- |
| OutPortByte  | 1바이트를 PIC에 쓴다.           |
| OutPortWord  | 2바이트를 PIC에 쓴다.           |
| OutPortDWord | 4바이트를 PIC에 쓴다.           |
| InPortWord   | 4바이트를 PIC로부터 읽어들인다. |
| InPortByte   | 1바이트를 PIC로부터 읽어들인다. |
| InPortWord   | 2바이트를 PIC로부터 읽어들인다. |

> 메소드 이름에 *Port*라는 단어가 공통으로 들어가 있는데 이 *Port*는 네트워크상에서 사용하는 *Port*의 의미와 똑같다.
>
> 모든 메소드는 해당 액션을 수행하기 위해 *Port*를 지정해야 한다.

PIC 관련 작업은 이것이 전부이며 이 작업을 통해서 우리는 이제 하드웨어 장치와 데이터를 주고 받을 수 있다.



<hr>

## PIT

**PIT(*Programmable Interval Timer*)**는 일상적인 타이밍 제어 문제를 해결하기 위해 설계된 카운터/ 타이머 디바이스이다.

![image](https://user-images.githubusercontent.com/34773827/59997884-fe784100-9698-11e9-84c6-9e9caae04f21.png)

[인텔 8254 PIT 위키백과](https://www.google.co.kr/url?sa=i&rct=j&q=&esrc=s&source=images&cd=&cad=rja&uact=8&ved=2ahUKEwj-3p6YxIHjAhVHUbwKHXoYBDQQjhx6BAgBEAM&url=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FIntel_8253&psig=AOvVaw3yCyP0jhG4-YzgMmqcmpRA&ust=1561445425820461)

X86 하드웨어 타이머는 그림과 같이 3개의 카운터와 제어 레지스터를 가지고 있다.

| 포트 | 레지스터 | 접근        |
| ---- | -------- | ----------- |
| 040h | Counter0 | 읽기 / 쓰기 |
| 041h | Counter1 | 읽기 / 쓰기 |
| 042h | Counter2 | 일기 / 쓰기 |
| 043h | Counter3 | 쓰기        |



타이머를 초기화하는 코드

- 먼저 타이머 이벤트가 발생 했을 때 이를 다룰 수 있는 인터럽트 핸들러를 등록해야 한다.

  ```c++
  // PIT 초기화
  void InitializePIT()
  {
      setvect(32, InterruptPITHandler);
  }
  ```

  타이머도 인터럽트 형태로 CPU에 신호를 제공하므로 우리는 인터럽트 서비스 루틴을 구현해야 한다.<br>타이머의 인터럽트 번호는 32다. 인터럽트 서비스 프로시저는 InterruptPITHandler에 구현이 되어 있어, 당장은 이 함수는 어떠한 액션도 수행하지 않는다.

- 이제 핸들러를 등록했으면 타이머가 시작하게끔 명령을 내려야 한다.<br>다음 *StartPITCounter* 함수를 통해 우리는 타이머를 동작시킬 수 있다.

  ```c++
  // 타이머를 시작
  void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode)
  {
      if(freq == 0)
          return;
      
      uint16_t divisor = uint16_t(1193181 / (uint16_t)freq);
      
      // 커맨드 전송
      uint8_t ocw = 6;
      ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
      ocw = (ocw & ~I86_PIT_OCW_MASK_RL)   | I86_PIT_OCW_RL_DATA;
      ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
      SendPITCommand(ocw);
      
      // 프리퀀시 비율 설정
      SendPITData(divisor & 0xff, 0);
      SendPITData((divisor >> 8) & 0xff, 0);
      
      // 타이머 틱 카운트 리셋
      _pitTicks = 0;
  }
  ```

  PIT는 1초마다 1193181번의 숫자를 카운팅하고 타이머 인터럽트를 발생시킨다.<br>이 인터럽트를 통해서 우리는 프로세스나 스레드의 스케줄링을 할 수 있으므로 PIT의 역할은 매우 중요하다.

  위 함수는 다음과 같이 호출한다.

  ```C++
  StartPITCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);
  ```

  > 1. 진동수
  >
  >    초당 진동수가 100 이라는 것은 1초당 100번의 타이머 인터럽트가 발생한다는 의미이다.<BR>이 값을 변화시킴에 따라 초당 인터럽트 수나 숫자의 증가가를 조정할 수 있다.
  >
  >    만약 진동수를 1로 하면 인터럽트가 발생할 때 PIT 내부 숫자 카운팅 값은 1193181이 될 것이고며, 진동수가 100이 된다면 인터럽트가 발생했을 때 PIT 내부 숫자 카운팅 값은 11931이 될 것이다.<BR>이 값을 **divisor**라고 한다.
  >
  > 2. 사용할 카운터 레지스터
  >
  >    즉, 0번 레지스터를 사용한다.
  >
  > 3. 제어 레지스터의 1 - 3 비트에 설정하는 부분으로 타이머의 카운팅 방식을 설정하는 데 쓰인다.
  >
  >    제어레지스터의 구성은 다음과 같다.
  >
  >    ![image](https://user-images.githubusercontent.com/34773827/59999283-9166aa80-969c-11e9-8937-c5f0ccde327c.png)
  >
  >    I86_PIT_OCW_MODE_SQUAREWAVEGEN의 값은 0X11이며 MODE 영역의 3비트에 설정된다.

  이제 1바이트 제어 레지스터에 우리가 사용할 카운터 레지스터 (00), 읽기/쓰기, 모드 값 등을 설정해서 명령을 보내고, 0번째 카운팅 레지스터에 Divisor 데이터를 보냄으로써 PIT는 초기화 된다.

  > 이렇게 하면 PIT는 CPU에 우리가 설정한 진동수에 맞게 매번 인터럽트를 보낸다.