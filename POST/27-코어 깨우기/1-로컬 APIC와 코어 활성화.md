# 로컬 APIC와 코어 활성화

## 로컬 APIC 레지스터의 구조와 레지스터

로컬 APIC는 외부 디바이스에서 발생한 인터럽트나 I/O APIC에서 전달된 인터럽트를 CPU에 전달한다.<br>그리고 내부 타이머나 성능 모니터링 카운터, 온도 센서 같은 프로세서 내부에서 발생한 인터럽트를 전달하거나 직접 인터럽트를 생성하기도 한다.

![image](https://user-images.githubusercontent.com/34773827/62596431-42f42f00-b91d-11e9-915c-82d4f57dbede.png)

그림으로 외부 인터럽트를 CPU로 전달하는 방법과 내부 인터럽트를 처리하는 방법에 대해 몇가지 얻을 수 있다.

#### 외부 인터럽트

먼저 외부 인터럽트로 표시된 영역을 보면 APIC 버스를 통해 전달된 인터럽트와 LINT 0/1 핀에서 전달된 인터럽트가 있다. APIC 버스를 통해 전달된 인터럽트와 LINT 0/1 핀에서 전달된 인터럽트 중에서 INIT, NMI, SMI 타입의 인터럽트는 내부 처리 로직을 거치지 않고 바로 CPU로 전달된다. <BR>이를 제외한 나머지 타입은 인터럽트에 해당 벡터를 가지고 있다. <BR>인터럽트 벡터는 프로세서 우선순위와 관계가 있어서 벡터와 프로세서의 우선순위를 비교하여  CPU로 전달할지를 결정한다. 

#### 내부 인터럽트

이러한 처리 방식은 내부 인터럽트도 마찬가지이며 내부 인터럽트 역시 로컬 벡터 테이블에 할당된 인터럽트 벡터와 프로세서의 우선순위를 비교해서 CPU로 전달할지 여부를 결정한다.

내부 인터럽트에는 타이머와 성능 모니터링 카운터, 온도 센서가 전달하는 인터럽트가 있다.<br>이러한 내부 인터럽트는 LINT0 핀과 LINT1 핀에서 전달된 인터럽트와 함께 로컬 벡터 테이블에 저장된 설정에 따라 처리한다. 로컬 벡터 테이블은 인터럽트가 발생했을 때 CPU로 전달할 인터럽트 벡터와 마스크 여부, 인터럽트 전달 모드 등이 저장되어 있으며, 필요하다면 목적에 맞게 설정을 변경할 수 있다.

<HR>

로컬 APIC는 메모리 맵 I/O 방식을 사용하므로 밑의 표에 표시된 어드레스에 값을 읽고 쓰는 방식으로 제어된다.

로컬 APIC 레지스터에 접근할 수 있는 기준 어드레스는 프로세서나 코어가 리셋 되었을 때 0xFEE00000로 설정되며, 0xFEE00000 ~ 0xFEE003F0 범위 내에 수십 개의 레지스터가 들어있다.

다음은 로컬 APIC의 레지스터 어드레스 맵을 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62596981-66b87480-b91f-11e9-846a-452fa913a61a.png)

<HR>

## 로컬 APIC 활성화

AP를 깨우려면 가장 먼저 해야 할 작업이 로컬 APIC를 활성화하는 것이다.

로컬 APIC를 활성화 하려면 두 가지 레지스터에 접근해야 하는데,<BR>하나는 IA32_APIC_BASE MSR 레지스터이며, <BR>다른 하나는 의사 인터럽트 벡터 레지스터(Spurious Interrupt Vector Register) 이다.

### IA32_APIC_BASE MSR

IA32_APIC_BASE MSR 레지스터는 이름의 분위기에서 알 수 있듯이 MSR(Model Specific Register)로 프로세서 모델 별로 정의된 특수 목적 레지스터이다.<br>IA32_APIC_BASE MSR 레지스터는 APIC 레지스터의 기준 어드레스와 APIC 활성화 여부, BSP 여부를 담당한다.

![image](https://user-images.githubusercontent.com/34773827/62597141-00802180-b920-11e9-9442-8ca4c5199c6a.png)

#### 기준 어드레스

이전 장에서 MP 설정 테이블에 저장된 로컬 APIC 메모리 맵 I/O 어드레스를 커널 내부의 자료구조에 저장했다.

이 어드레스에 저장 하는 이유는 IA32_APIC_BASE MSR 레지스터의 APIC 기준 어드레스 필드로 로컬 APIC 레지스터가 시작하는 어드레스를 변경할 수 있기 때문이다.

프로세서가 리셋되면 이 값은 0xFEE00000이지만, 부팅 과정에서 다른 값으로 변경될 수 있다.

따라서 로컬 APIC에 접근하기 전에 MP 설정 테이블을 읽거나 IA32_APIC_BASE MSR 레지스터를 읽어서 로컬 APIC 기준 어드레스를 확인해야 한다.

#### APIC 전역 활성/ 비활성

APIC 전역 활성/비활성 필드는 모든 로컬 APIC에 적용되며, 이 필드가 1로 설정되어야만 로컬 APIC가 동작한다.

이 필드가 0으로 설정되면 인터럽트 라인이 CPU에 직접 연결된 것처럼 동작하므로 로컬 APIC의 기능을 사용할 수 없다. 따라서 모든 코어를 활성화하려면 반드시 1로 설정해야 한다.

<HR>

#### IA32_APIC_BASE MSR을 설정해서 로컬 APIC를 활성화 하는 코드

```assembly
; IA32_APIC_BASE MSR의 APIC 전역 활성화 필드(비트 11)를 1로 설정하여 APIC를 활성화함
;   PARAM: 없음
kEnableGlobalLocalAPIC:
    push rax            ; RDMSR과 WRMSR에서 사용하는 레지스터를 모두 스택에 저장
    push rcx
    push rdx
    
    ; IA32_APIC BASE MSR에 설정된 기존 값을 읽어서 전역 APIC 비트를 활성화
    mov rcx, 27         ; IA32_APIC_BASE MSR은 레지스터 어드레스 27에 위치하며, 
    rdmsr               ; MSR의 상위 32비트와 하위 32비트는 각각 EDX 레지스터와 
                        ; EAX 레지스터를 사용함
    
    or eax, 0x0800      ; APIC 전역 활성/비활성 필드는 비트 11에 위치하므로 하위
    wrmsr               ;  32비트를 담당하는 EAX 레지스터의 비트 11을 1로 설정한 뒤
                        ;  MSR 레지스터에 값을 덮어씀
        
    pop rdx             ; 사용이 끝난 레지스터를 스택에서 복원
    pop rcx
    pop rax
    ret
```

<HR>

### 의사 인터럽트 벡터 레지스터 설정

IA32_APIC_BASE MSR 레지스터를 이용해서 로컬 APIC를 활성화했다면, 이제 로컬 APIC 레지스터 중 하나인 의사 인터럽트 벡터 레지스터(어드레스 0xFEE000F0)를 설정할 차례이다.

의사 인터럽트 벡터 레지스터는 포커스 프로세서 검사 기능을 사용할지 여부와 APIC 활성화 여부, 의사 벡터를 저장한다.

![image](https://user-images.githubusercontent.com/34773827/62597586-7a64da80-b921-11e9-8247-1dbaa609fb94.png)

#### 의사 인터럽트

의사 인터럽트는 하드웨어와 소프트웨어의 시간 차 때문에 발생하는 인터럽트로 일반적인 인터럽트는 아니다.

인터럽트 요청이 로컬 APIC로 전달되었을 때 컨트롤러는 IRR 레지스터와 TMR 레지스터 등을 변경하여 인터럽트를 대기시킨다. 그러나 태스크 우선순위 레지스터를 변경하는 작업은 OS와 같은 시스템 소프트웨어가 하기 때문에 둘 사이에 시간적 틈이 생기게 된다.

> 로컬 APIC에 특정 인터럽트가 쵸엉(INTR)되어 대기하고 있고, OS는 먼저 전달된 인터럽트를 처리하고 있다고 가정할 때
>
> OS는 현재 수행 중인 인터럽트 처리가 끝나면 인터럽트 완료(INTA)를 전송한다.
>
> 로컬 APIC는 인터럽트 완료를 수신하면 대기하고 있는 인터럽트 중에서 가장 우선순위가 높은 인터럽트를 꺼내서 CPU에 전달한다.
>
> 그런데 OS가 처리한 인터럽트의 응답을 전송하기 전에 태스크 우선순위를 대기 중인 인터럽트보다 높거나 같게 설정한 후 인터럽트 응답을 전달하면 태스크 우선순위보다 우선순위가 높은 인터럽트만 처리할 수 있으므로 대기 중인 인터럽트를 CPU에게 전달할 수 없기 때문에 문제가 생긴다.
>
> 이때 발생하는 것이 바로 의사 인터럽트이다.

의사 인터럽트는 실제 인터럽트가 아니므로 EOI 처리를 하지 않고 완료해야 한다.

하지만 우리 OS는 모든 인터럽트를 수신할 수 있도록 우선순위를 0으로 사용하므로,<BR>실제 의사 인터럽트가 발생할 일은 거의 없다.

<HR>

따라서 우리는 의사 인터럽트 벡터 레지스터를 로컬 APIC를 임시로 활성화 시키거나 비활성화 시키는 용도로 사용한다.

IA32_APIC_BASE MSR 레지스터가 모든 로컬 APIC에 영향을 끼친다면, 의사 인터럽트 벡터 레지스터는 해당 코어의 로컬 APIC에만 적용된다.

따라서 IA32_APIC_BASE MSR에서 활성화한 뒤에 BSP와 AP 모두 의사 인터럽트 벡터 레지스터를 사용하여 활성화 해야 한다.

#### 의사 인터럽트 벡터 레지스터를 설정하여 로컬 APIC를 활성화 하는 코드

```C
#define APIC_REGISTER_SVR                           0x0000F0

/**
 *  로컬 APIC의 메모리 맵 I/O 어드레스를 반환
 */
QWORD kGetLocalAPICBaseAddress( void )
{
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    
    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
    return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}

/**
 *  의사 인터럽트 벡터 레지스터(Spurious Interrupt Vector Register)에 있는 
 *  APIC 소프트웨어 활성/비활성 필드를 1로 설정하여 로컬 APIC를 활성화함
 */
void kEnableSoftwareLocalAPIC( void )
{
    QWORD qwLocalAPICBaseAddress;
    
    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    
    // 의사 인터럽트 벡터 레지스터(Spurious Interrupt Vector Register, 0xFEE000F0)의 
    // APIC 소프트웨어 활성/비활성 필드(비트 8)를 1로 설정해서 로컬 APIC를 활성화
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_SVR ) |= 0x100;
}
```

지금까지는 로컬 APIC를 활성화하는 방법을 살펴보았는데,<BR>사실 로컬 APIC만 활성화되면 코어를 깨울 준비가 절반은 끝났다고 할 수 있다.

남은 작업은 IPI 메시지를 이용하여 코어를 깨우는 것이다.

<HR>

## 코어 활성화와 인터럽트 커맨드 레지스터

코어, 즉 애플리케이션 프로세서(AP, Application Processor)를 활성화하는 작업 자체는 크게 어렵지 않다.

MultiProcessor Specification 문서의 "B Operating System Programming Guidelines" 장을 보면 AP를 활성화하는 방법이 정리되어 있으며, 각 과정을 진행하면 코어를 깨울 수 있다.

![image](https://user-images.githubusercontent.com/34773827/62598159-67530a00-b923-11e9-83f8-68bd43918698.png)

그림에 표시된 IPI는 Interprocessor Interrupts의 약자로 프로세서 간에 전달하는 인터럽트를 의미한다.

IPI는 로컬 APIC의 하위 인터럽트 커맨드 레지스터(어드레스 0xFEE00300)와 상위 인터럽트 커맨드 레지스터(어드레스 0xFEE00310)로 생성할 수 있다.

![image](https://user-images.githubusercontent.com/34773827/62598286-dfb9cb00-b923-11e9-9b4b-0885cdd21c67.png)

![image](https://user-images.githubusercontent.com/34773827/62598407-3a532700-b924-11e9-8440-30b12c8405f5.png)

#### 코어(AP)를 활성화 하는 코드

```c
// 로컬 APIC 레지스터 오프셋 관련 매크로
#define APIC_REGISTER_APICID                        0x000020
#define APIC_REGISTER_ICR_LOWER                     0x000300

// 전달 모드(Delivery Mode) 관련 매크로
#define APIC_DELIVERYMODE_INIT                      0x000500
#define APIC_DELIVERYMODE_STARTUP                   0x000600

// 목적지 모드(Destination Mode) 관련 매크로
#define APIC_DESTINATIONMODE_PHYSICAL               0x000000

// 전달 상태(Delivery Status) 관련 매크로
#define APIC_DELIVERYSTATUS_PENDING                 0x001000

// 레벨(Level) 관련 매크로
#define APIC_LEVEL_ASSERT                           0x004000

// 트리거 모드(Trigger Mode) 관련 매크로
#define APIC_TRIGGERMODE_LEVEL                      0x008000

// 목적지 약어(Destination Shorthand) 관련 매크로
#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF  0x0C0000

// 활성화된 Application Processor의 개수
volatile int g_iWakeUpApplicationProcessorCount = 0;
// APIC ID 레지스터의 어드레스
volatile QWORD g_qwAPICIDAddress = 0;

/**
 *  AP(Application Processor)를 활성화
 */
static BOOL kWakeUpApplicationProcessor( void )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    QWORD qwLocalAPICBaseAddress;
    BOOL bInterruptFlag;
    int i;

    // 인터럽트를 불가로 설정
    bInterruptFlag = kSetInterruptFlag( FALSE );

    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    pstMPManager = kGetMPConfigurationManager(); 
    pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
    qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

    // APIC ID 레지스터의 어드레스(0xFEE00020)를 저장하여, Application Processor가
    // 자신의 APIC ID를 읽을 수 있게 함
    g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
    
    //--------------------------------------------------------------------------
    // 하위 인터럽트 커맨드 레지스터(Lower Interrupt Command Register, 0xFEE00300)에
    // 초기화(INIT) IPI와 시작(Start Up) IPI를 전송하여 AP를 깨움
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    // 초기화(INIT) IPI 전송
    //--------------------------------------------------------------------------
    // 하위 인터럽트 커맨드 레지스터(0xFEE00300)을 사용해서 BSP(Bootstrap Processor)를
    // 제외한 나머지 코어에 INIT IPI를 전송
    // AP(Application Processor)는 보호 모드 커널(0x10000)에서 시작
    // All Excluding Self, Edge Trigger, Assert, Physical Destination, INIT
    *( DWORD* )( qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER ) = 
        APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
        APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;
    
    // PIT를 직접 제어하여 10ms 동안 대기
    kWaitUsingDirectPIT( MSTOCOUNT( 10 ) );
        
    // 하위 인터럽트 커맨드 레지스터(0xFEE00300)에서 전달 상태 비트(비트 12)를 
    // 확인하여 성공 여부 판별
    if( *( DWORD* )( qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER ) &
            APIC_DELIVERYSTATUS_PENDING )
    {
        // 타이머 인터럽트가 1초에 1000번 발생하도록 재설정
        kInitializePIT( MSTOCOUNT( 1 ), TRUE );
        
        // 인터럽트 플래그를 복원
        kSetInterruptFlag( bInterruptFlag );
        return FALSE;
    }
    
    //--------------------------------------------------------------------------
    // 시작(Start Up) IPI 전송, 2회 반복 전송함
    //--------------------------------------------------------------------------
    for( i = 0 ; i < 2 ; i++ )
    {
        // 하위 인터럽트 커맨드 레지스터(0xFEE00300)을 사용해서 BSP를 제외한 
        // 나머지 코어에 SIPI 메시지를 전송
        // 보호 모드 커널이 시작하는 0x10000에서 실행시키려고 0x10(0x10000 / 4Kbyte)를
        // 인터럽트 벡터로 설정
        // All Excluding Self, Edge Trigger, Assert, Physical Destination, Start Up
        *( DWORD* )( qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER ) = 
            APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
            APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | 
            APIC_DELIVERYMODE_STARTUP | 0x10;
        
        // PIT를 직접 제어하여 200us 동안 대기
        kWaitUsingDirectPIT( USTOCOUNT( 200 ) );
            
        // 하위 인터럽트 커맨드 레지스터(0xFEE00300)에서 전달 상태 비트(비트 12)를 
        // 확인하여 성공 여부 판별
        if( *( DWORD* )( qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER ) &
                APIC_DELIVERYSTATUS_PENDING )
        {
            // 타이머 인터럽트가 1초에 1000번 발생하도록 재설정
            kInitializePIT( MSTOCOUNT( 1 ), TRUE );
            
            // 인터럽트 플래그를 복원
            kSetInterruptFlag( bInterruptFlag );
            return FALSE;
        }
    }
    
    // 타이머 인터럽트가 1초에 1000번 발생하도록 재설정
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );
    
    // 인터럽트 플래그를 복원
    kSetInterruptFlag( bInterruptFlag );
    
    // Application Processor가 모두 깨어날 때까지 대기
    while( g_iWakeUpApplicationProcessorCount < 
            ( pstMPManager->iProcessorCount - 1 ) )
    {
        kSleep( 50 );
    }    
    return TRUE;
}
```



#### 로컬 APIC 활성화와 AP 활성화를 순차적으로 수행하는 코드

```C
/**
 *  로컬 APIC를 활성화하고 AP(Application Processor)를 활성화
 */
BOOL kStartUpApplicationProcessor( void )
{
    // MP 설정 테이블 분석
    if( kAnalysisMPConfigurationTable() == FALSE )
    {
        return FALSE;
    }
    
    // 모든 프로세서에서 로컬 APIC를 사용하도록 활성화
    kEnableGlobalLocalAPIC();
    
    // BSP(Bootstrap Processor)의 로컬 APIC를 활성화
    kEnableSoftwareLocalAPIC();    
    
    // AP를 깨움
    if( kWakeUpApplicationProcessor() == FALSE )
    {
        return FALSE;
    }
    
    return TRUE;
}
```



