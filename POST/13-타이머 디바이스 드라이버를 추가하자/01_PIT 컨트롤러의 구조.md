# PIT 컨트롤러의 구조와 기능

## PIT 컨트롤러, I/O 포트, 레지스터

PIT 컨트롤러는 1개의 컨트롤 레지스터와 3개의 카운터로 구성되며,<br>I/O 포트 0x43, 0x40, 0x41, 0x42에 연결되어 있다.

이 중에서 컨트롤 레지스터는 쓰기만 가능하고, 컨트롤 레지스터를 제외한 카운터는 읽기 쓰기 모두 가능하다.

< PIC 컨트롤러와 PC 내부 버스의 관계 >

![image](https://user-images.githubusercontent.com/34773827/61349405-ffb71b00-a89e-11e9-8290-7bfdd6dc2eb5.png)

< I/O 포트와 PIT 컨트롤러 레지스터의 관계 >

![image](https://user-images.githubusercontent.com/34773827/61349609-a8fe1100-a89f-11e9-8021-e1078bb2ab4a.png)

<hr>

PIT 컨트롤러의 컨트롤 레지스터의 크기는 1바이트고 카운터 레지스터의 크기는 2바이트이다.

I/O 포트는 PIT 컨트롤러와 1바이트 단위로 전송하므로,<br>컨트롤 레지스터에 전달하는 커맨드에 따라 카운터 I/O 포트로 읽고 쓰는 바이트 수가 결졍된다.

컨트롤 레지스터는 4개의 필드로 구성되어 있으며, 각 필드의 오프셋과 역할은 다음과 같다.

![image](https://user-images.githubusercontent.com/34773827/61349985-0181de00-a8a1-11e9-890c-a6e03de58fe8.png)

<hr>

### PIT 모드와 카운터

PIT 컨트롤러의 내부 클록은 1.193182Mhz으로 동작하며,<br>매회마다 각 카운터의 값을 1씩 감소시켜 0이 되었을 때 신호를 발생시킨다.

신호를 발생 시킨 이후에 PIT 컨트롤러는 설정된 모드에 따라 다르게 동작한다.

#### 모드

PIT 컨트롤러는 총 6개의 모드를 지원한다.

- 모드 0

  카운터에 입력된 값을 매회마다 1씩 감소시켜 카운터가 0이 되었을 때 외부로 신호를 발생시키는 모드

  카운터에 새로운 값을 설정하지 않는 한 자동으로 반복하지 않기 때문에 일정 시간이 지났음을 확인하는데 사용한다.

- 모드 2

  모드 2는 모드 0과 기능이 거의 같지만, 차이점은 카운터에 새로운 값을 사용하지 않아도 자동으로 반복한다는 것이다.

  따라서 모드 2를 사용하면 일정한 주기로 신호를 발생시킬 수 있기 때문에 시간 측정을 반복하거나 특정 작업을 반복해서 수행하는데 사용한다.

모드 0 과 2를 제외한 나머지 모드는 PC에서 잘 사용하지 않는다.

#### 카운터

PIT 컨트롤러에는 카운터가 3개 있으며, 이중에서 카운터 1과 카운터 2는 예전부터 메인 메모리와 PC 스피커를 위해 사용했다.

카운터 0이 다른 카운터와 다른 점은<br>완료 신호를 출력하는 OUT 핀이 PIC 컨트롤러의 IRQ 0 에 연결되어있다는 것이다.<br>이는 카운터 0를 사용하면 일정한 주기로 인터럽트를 발생시킬 수 있다는 것을 의미한다.

<hr>



## PIT 컨트롤러 초기화

PIT 컨트롤러를 초기화하는 데 가장 중요한 요소는 바로 시간이다.

이것은 PIT 컨트롤러의 본래 목적이자 우리가 PIT 컨트롤러를 사용하는 근본적인 이유이다.

PIT 컨트롤러를 이용해서 시간을 측정하려면 시간을 PIT 컨트롤러의 카운터 값으로 변환해야한다.

내부 클록이 1.193182Mhz로 동작하므로 한 클록은 1초를 내부 클록으로 나눈 약 838.095ns(10의 -9승)이 된다.

따라서 측정할 시간을 이 값으로 나누면 카운터에 설정해야 할 값을 구할 수 있다.

카운터 값을 계산하는 또 다른 방법은 1초에 1193182번 카운터가 증가하므로 이 값에 시간을 곱하는 것이다.

> 예를 들어 현재 시점에서 100us(microsecond, 10의 -6승)를 기다린다면 1193182에 약 0.0001초를 곱한 약 23864를 설정한다.

PIT 컨트롤러의 카운터에 설정할 수 있는 최대 값이 0x100000(66536)임을 감안하면 하나의 카운터로 측정할 수 있는 최대 시간은 약 54.93ms가 된다.

> 2바이트로 표현할 수 있는 범위는 0x00 ~ 0xFFFF까지 이다.
>
> PIT 컨트롤러는 0을 카운터에 전송하면 다음 클록에서 최댓값인 0xFFFF로 변경한 뒤 카운트를 시작한다.
>
> 따라서 카운터 하나로 계산할 수 있는 범위는 2바이트 범위에서 1을 더한 0x00 ~ 0x10000이 된다.

PIT 컨트롤러를 초기화하는 데 필요한 또 다른 요소는 반복 여부이다.

만약 PIT 컨트롤러를 사용하는 목적이 현재 시점에서 일정 시간이 지났음을 확인하는 것이라면<br>한 번만 확인하면 되므로 모드 0을 사용하면 된다.

만약 일정한 주기로 반복하는 것이 목적이라면 카운터의 재설정 없이 자동으로 반복해야 하므로<br>모드 2를 사용해야 한다.

<hr>

#### PIT 컨트롤러 초기화에 관련된 매크로와 함수

이러한 이유로 PIT 컨트롤러 초기화 함수는 시간(카운터 값)과 반복 유무를 설정할 수 있게 전달받도록 한다.

다음은 PIC 컨트롤러를 초기화 하는 함수와 관련된 매크로를 나타낸 것이다.

```c
#define PIT_FREQUENCY 1193180
#define MSTOCOUNT( x )	(PIT_FREQUENCY * (x) / 1000)
#define USTOCOUNT( x )	(PIT_FREQUENCY * (x) / 1000000)

// I/O 포트
#define PIT_PORT_CONTROL	0x43
#define PIT_PORT_COUNTER0	0x40

// 모드
#define PIT_CONTROL_COUNTER0		0x00
#define PIT_CONTROL_LSBMSBRW		0x30
#define PIT_CONTROL_LATCH			0x00
#define PIT_CONTROL_MODE0			0x00
#define PIT_CONTROL_MODE2			0x04
#define PIT_CONTROL_BINARYCOUNTER	0x00
#define PIT_CONTROL_BDCCOUNTER		0x01

#define PIT_COUNTER0_ONCE	(PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW |\
							PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER)\
#define PIT_COUNTER0_PREIODIC (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | \
							PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)\
// 초기화 함수
void kInitializePIT(WORD wCount, BOOL bPeriodic)
{
    // PIT 컨트롤 레지스터(포트 0x43)에 값을 초기화하여 카운트를 멈춘 뒤에
    // 모드 0에 바이너리 카운터로 지정
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
    
    // 만약 일정한 주기로 반복하는 타이머라면 모드 2로 설정한다.
    if(bPeriodic == TRUE)
    {
        // PIT 컨트롤 레지스터(포트 0x43)에 모드 2에 바이너리 카운터로 설정
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    }
    
    // 카운터 0(포트 0x40)에 LSB -> MSB 순으로 카운터 초기 값을 설정
    kOutPortByte(PIT_PORT_COUNTER0, wCount);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}
```

*kInitializePIT( )* 함수를 사용할 때 주의할 점은 파라미터로 전달하는 값이 시간이 아니라 카운터에 설정할 값이라는 점이다.

만약 시간을 기준으로 전달하고 싶다면 MSTOCOUNT 매크로와 USTOCOUNT를 사용해야 한다.

두 매크로는 밀리세컨드와 마이크로세컨드 시간을 카운터로 변환하는 매크로이며, 사용방법은 다음과 같다.

```C
// 1ms 마다 주기적으로 인터럽트가 발생하도록 설정하는 예
kInitializePIT(MSTOCOUNT(1), TRUE);

// 100us 후에 인터럽트가 발생하도록 설정하는 예
kInitializePIT(USTOCOUNT(100), FALSE);
```

<hr>



## 카운터를 읽어 직접 시간 계산하기

PIT 컨트롤러를 초기화한 후 타이머가 완료되었음은 인터럽트, 즉 IRQ 0을 통해 확인할 수 있다.

하지만 인터럽트를 처리하는 PIC 컨트롤러가 초기화되지 않았거나 인터럽트가 부분적으로 비활성화된 상황이라면 인터럽트로 확인할 수 없다.

따라서 이러한 환경에서도 일정 시간이 경과했는지 확인할 방법이 필요하다.

<hr>

PIT 컨트롤러는 카운터가 초기화되면 내부 클록에 따라 카운터의 값을 1씩 감소시킨다.

> 만약 우리가 PIC 컨트롤러의 카운터 값을 직접 읽을 수 있다면,<br>특정 두 시점에서 읽은 카운터의 차를 이용해 일정 시간이 경과했는지 여부를 알 수 있다.

PIT 컨트롤러의 카운터를 직접 읽으려면, 먼저 래치(*Latch*) 커맨드를 전송해야 한다.

래치 커맨드를 전송하면 <br>PIT 컨트롤러는 래치 커맨드가 지정한 카운터 레지스터에 현재 감소 중인 카운터 값을 저장한다.<br>커맨드를 전송한 후 카운터의 I/O 포트를 하위 바이트와 상위 바이트 순으로 두 번에 걸쳐 읽으면 된다.

카운터의 값을 직접 읽어 시간의 경과를 측정하려면 특정 시점에서 카운터를 읽어 이전 값과 비교해야 한다.

카운터의 값은 아주 빠르게 변하므로 계산을 편리하게 하려면 카운터가 0 ~ 0xFFFF 범위에서 변하도록 설정하는 것이 좋은데 0 ~ 0xFFFF이 WORD(2바이트 부호 없는 정수)의 표현 범위와 같으므로 쉽게 계산 할 수 있기 때문이다

<hr>

#### PIT 컨트롤러를 통해 시간을 측정하는 코드

```c
// 카운터 0의 현재 값을 반환
WORD kReadCounter0(void)
{
    BYTE bHighByte, bLowByte;
    WORD wTemp = 0;
    
    // PIT 컨트롤 레지스터(포트 0x43)에 래치 커맨드를 전송하여 카운터 0에 있는 현재 값을 읽는다.
    kOutPortByte(PIT_PORT_CONTROL,PIT_COUNTER0_LATCH);
    
    // 카운터 0(포트 0x40)에서 LSB->MSB 순으로 카운터 값을 읽는다.
    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);
    
    // 읽은 값을 16비트로 합하여 변환
    wTemp = bHighByte;
    wTemp = (wTemp << 8)|bLowByte;
    return wTemp;
}

// 카운터 0을 직접 설정하여 일정 시간 이상 대기
//		함수를 호출하면 PIT 컨트롤러의 설정이 바뀌므로, 이후에 PIT 컨트롤러를 재설정해야 한다.
//		정확하게 측정하려면 함수 사용전에 인터럽트를 비활성화 하는 것이 좋다.
//		약 50ms까지 측정가능하다.
void kWaitUsingDirectPIT(WORD wCount);
{
    WORD wLastCounter0;
    WORD wCurrentCounter0;
    
    // PIT 컨트롤러를 0 ~ 0xFFFF 까지 반복해서 카운터하도록 설정
    kInitializePIT(0, TRUE);
    
    // 지금부터 wCount 이상 증가할 때 까지 대기
    wLastCounter0 = kReadCounter0();
    while(1)
    {
        // 현재 카운터 0의 값을 반환
        wCurrentCounter0 = kReadCounter0();
        if(((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount)
        {
            break;
        }
    }
}
```

<hr>

#### *kWaitUsingDirectPIT( )* 함수의 사용 예

다음은 *kWaitUsingDirectPIT( )* 함수를 사용해서 파라미터로 넘어온 millisecond 동안 대기하는 코드이다.

```c
void kWaitms(long lMillisecond)
{
    int i;
    
    // 30ms 단위로 나누어서 수행
    for(i = 0; i< lMillisecond / 30 ; i++)
    {
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
}
```

