# 타임 스탬프 카운터와 RTC

## 타임 스탬프 카운터와 사용방법

타임 스탬프 카운터(TSC, *Time Stamp Counter*)는 프로세서의 클록에 따라 값이 증가하는 카운터로<br>64비트 MSR(*Model Specific Register*) 레지스터이다.

> 프로세서의 클록은 다른 컨트롤러의 클록에 비해 아주 빠르므로 시간을 정확하게 측정해야 할 때 유용하게 쓸 수 있다.

<hr>

타임 스탬프 카운터는 프로세서 계열에 따라 증가하는 비율이 다르다.

과거 프로세서는 프로세서 클록에 따라 카운터가 1씩 증가했다.<br>따라서 절전 기능으로 인해 버스 클록이 낮아져서 프로세서의 클록이 감소하면 타임 스탬프 카운터의 증가 속도 역시 감소했다.

하지만 멀티코어 프로세서나 최신 프로세서는 매 클록마다 프로세서 최대 클록을 버스 클록으로 나눈 비율만큼 증가한다. 따라서 버스 클록이 낮아지더라도 타임 스탬프는 일정한 속도로 증가한다.

<hr>

타임 스탬프 카운터의 이러한 특징을 이용하면 프로세서의 실제 클록을 간접적으로 측정하는 것도 가능하다.

타임 스탬프 카운터는 RDTSC 명령어로 접근할 수 있으며, RDMSR 명령어를 사용하여 <br>IA32_TIME_STAMP_COUNTER(0x10)통해 접근할 수도 있다.

> RDMSR 명령어보다 RDTSC 명령어를 사용하는 것이 더 간단해 일반적으로 RDTSC를 많이 사용한다.

<hr>

타임 스탬프 카운터는 32비트 프로세서인 펜티엄 계열부터 도입되었는데<br> 그 당시는 64비트 크기의 타임 스탬프 카운터를 하나의 범용 레지스터에 담을 수 없었기 때문에,<br>RDTSC 명령어는 타임 스탬프 카운터의 상위 32비트를 EDX레지스터에 넣고 하위 32비트를 EAX 레지스터에 넣어 전달했다.

이러한 방식은 64비트 프로세서에도 계승되어,<br>64비트 프로세서의 RDTSC 명령어 역시 상위 32비트와 하위 32비트를<br>각각 RDX 레지스터와 RAX 레지스터에 나누어 전달한다.

<hr>

#### 타임 스탬프 카운터를 읽는 어셈블리어 함수와 함수 선언 코드

RDTSC 명령어를 사용하려면 RDX 레지스터와 RAX 레지스터를 직접 제어해야 하므로,<BR>타임 스탬프 카운터를 읽는 어셈블리어 함수를 작성해보자.

다음은 RDTSC 명령어를 사용해서 64비트 타임 스탬프 값을 반환하는 함수의 코드이다.

```assembly
; 타임 스탬프 카운터를 읽어서 반환
;	PARAM: 없음
kReadTSC:
	push rdx		; RDX 레지스터를 스택에 저장
	
	rdtsc			; 타임 스탬프 카운터를 읽어서 RDX:RAX에 저장
	
	shl rdx, 32		; RDX 레지스터에 있는 상위 32비트 TSC 값과 RAX 레지스터에 있는
					; 하위 32비트 TSC 값을 OR 하여 RAX 레지스터에 64비트 TSC 값 저장
					
	pop rdx
	ret
	
// 함수 선언
QWORD	kReadTSC(void);
```

<hr>



## RTC 컨트롤러와 CMOS 메모리

RTC 컨트롤러는 현재 일자와 시간을 담당하는 컨트롤러로서,<br>PC가 꺼진 상태에서도 시간을 기록하기 위해 별도의 전원을 사용한다.

RTC 컨트롤러는 현재 시간을 BIOS와 곤통으로 사용하는 메모리에 기록하는데,<br>이러한 메모리를 CMOS 메모리라고 부른다.

CMOS 메모리는 BIOS에서 사용하는 시스템 설정 값이나 현재 시간 등의 정보를 저장하는 공간으로<br>I/O 포트를 통해 읽고 쓸 수 있다.

CMOS 메모리에 관련된 I/O 포트는 CMOS 메모리 어드레스 포트(0x70)과 CMOS 메모리 데이터 포트(0x71)가 있으며, 각 포트는 1바이트씩 데이터를 읽고 쓸 수 있다.

CMOS 메모리의 데이터를 읽고 쓰려면<br>먼저 어드레스 포트에 관련 레지스터의 어드레스를 설정한 후 데이터 포트에 접근해야 한다.

<hr>

CMOS 메모리 어드레스 포트(0x70)는 NMI 제어에 관련된 비트와 CMOS 메모리 어드레스 필드로 나누어진다.

![image](https://user-images.githubusercontent.com/34773827/61354935-35173500-a8ae-11e9-97f1-20c4d26fe0c2.png)

CMOS 메모리는 시스템 전반에 대한 정보가 포함되어 있으므로<br>포함된 정보의 종류는 수십 가지가 넘는다.

그 중에서 우리가 필요한 것은 RTC에 관련된 데이터이므로,<br>RTC 컨트롤러에 관련된 데이터 레지스터만 추출하면 다음과 같다.

| 어드레스 | 레지스터 이름     | 설명                                                         |
| -------- | ----------------- | ------------------------------------------------------------ |
| 0x00     | Seconds           | 현재 시간 중에서 초를 저장하는 레지스터                      |
| 0x01     | Seoncs Alarm      | 알람을 울릴 초를 저장하는 레지스터                           |
| 0x02     | Minutes           | 현재 시간 중에서 분을 저장하는 레지스터                      |
| 0x03     | Minutes Alarm     | 알람을 울릴 분을 저장하는 레지스터                           |
| 0x04     | Hours             | 현재 시간 중에서 시간을 저장하는 레지스터                    |
| 0x05     | Hours Alarm       | 알람을 울릴 시간을 저장하는 레지스터                         |
| 0x06     | Day of Week       | 현재 일자 중에서 요일에 대한 정보를 저장하는 레지스터<br>일월화수목금토 순으로 (1 ~ 7)의 값을 할당 |
| 0x07     | Day Of The Month  | 현재 일자 중에서 일을 저장하는 레지스터                      |
| 0x08     | Month             | 현재 일자 중에서 달을 저장하는 레지스터                      |
| 0x09     | Year              | 현재 일자 중에서 연을 저장하는 레지스터                      |
| 0x0A     | Status Register A | RTC 컨트롤러의 데이터 업데이트 주기나<br>컨트롤러의 클록에 대한 정보를 설정하는 레지스터 |
| 0x0B     | Status Register B | RTC 인터럽트, 알람, 데이터 표기방식을 설정하는 레지스터<br>알람이 발생했을 때 인터럽트 발생 여부나<br>CMOS 메모리에 저장하는 데이터 포맷(BCD or Binary) 선택 |
| 0x0C     | Status Register C | RTC컨트롤러의 인터럽트 상태 정보를 저장하는 레지스터<br>읽기 전용 레지스터 |
| 0x0D     | Status Register D | RTC의 전원에 관련된 정보를 저장하는 레지스터<br>비트 7의 값이 1로 설정된 경우<br>RTC 컨트롤러에 연결된 전원이 불안정하거나 끊어진 경우이므로<br> 조치가 필요하다.<br>읽기 전용 |

<hr>



### 현재 시간을 읽는 방법

RTC 컨트롤러의 알람 기능이나 인터럽트 발생 기능은 우리 OS에서 사용하지 않으므로<br>현재 시간을 읽는 방법을 중심으로 살펴보자.

현재 시간과 일자를 얻으려면 <br>CMOS 메모리 중에서 Second, Minutes, Hours 레지스터와 Day, Month, Year 레지스터를 읽어야 한다.

CMOS 메모리에서 RTC 관련 정보를 읽는다고 해서 그 값을 바로 쓸 수 있는 것은 아니다.

RTC 컨트롤러는 별다른 설정이 없는 한 BCD 포맷으로 데이터를 저장하므로 읽은 후에 값을 변환하는 작업이 필요하기 때문이다.

CMOS 메모리의 RTC 정보는 1바이트 씩이며, <br>BCD 포맷은 10진수 한 자리를 표현하는데 4비트를 할당한다.

따라서, 1바이트에는 2자리의 정수가 들어 있으므로<br> 상위 4비트를 추출하여 자리수인 10을 곱하고 하위 4비트를 추출하여 더하면 바이너리 포맷으로 변환가능하다.

다음 코드는 CMOS 메모리에서 RTC 정보를 읽어 반환하는 함수와 매크로이다.

*kReadRTCTime( )* 함수는 현재 시간을 읽으며 *kReadRTCData( )* 함수는 현재 일자를 읽는다.

```c
// I/O 포트
#define RTC_COMSADDRESS		0x70
#define RTC_COMSDATA		0x71

// CMOS 메모리 어드레스
#define RTC_ADDRESS_SECOND		0x00
#define RTC_ADDRESS_MINUTE		0x02
#define RTC_ADDRESS_HOUR		0x04
#define RTC_ADDRESS_DAYOFWEEK	0x06
#define RTC_ADDRESS_DAYOFMONTH	0x07
#define RTC_ADDRESS_MONTH		0X08
#define RTC_ADDRESS_YEAR		0x09

// BCD 포맷을 Binary로 변환하는 매크로
#define	RTC_BCDTOBINARY(x) ((((x) >> 4) * 10) + ((x) & 0x0F))

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 시간을 읽는다.
void kReadRTCTime(BYTE* bpHour, BYTE* bpMinute, BYTE* pbSecond)
{
    BYTE bData;
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 시간을 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    // CMOS 데이터 레지스터(포트 0x71)에서 시간을 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 분을 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_MINUTE);
    // CMOS 데이터 레지스터(포트 0x71)에서 분을 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 초를 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_SECOND);
    // CMOS 데이터 레지스터(포트 0x71)에서 초를 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pbSecond = RTC_BCDTOBINARY(bData);
}

// CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 일자를 읽는다.
void kReadRTCData(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth,
                 BYTE* pbDayOfWeek)
{
    BYTE bData;
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 연도를 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    // CMOS 데이터 레지스터(포트 0x71)에서 연도를 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pwYear = RTC_BCDTOBINARY(bData) + 2000;
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 월을 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_MONTH);
    // CMOS 데이터 레지스터(포트 0x71)에서 월을 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pwMonth = RTC_BCDTOBINARY(bData);
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 일을 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS,RTC_ADDRESS_DAYOFMONTH);
    // CMOS 데이터 레지스터(포트 0x71)에서 일을 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pwDayOfMonth = RTC_BCDTOBINARY(bData);
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 요일을 저장하는 레지스터 지정
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    // CMOS 데이터 레지스터(포트 0x71)에서 요일을 읽는다.
    bData = kInPortByte(RTC_CMOSDATA);
    *pwDayOfWeek = RTC_BCDTOBIANRY(bData);
}
```



