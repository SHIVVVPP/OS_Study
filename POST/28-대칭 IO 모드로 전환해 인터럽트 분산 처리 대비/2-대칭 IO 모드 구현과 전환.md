# 대칭 I/O 모드 구현과 전환

## I/O 리다이렉션 테이블 설정 과정

대칭 I/O 모드로 전환하려면 인터럽트를 처리하는 부분이 바뀌기 때문에<BR>관련된 여러 부분을 수정해야 하지만 범위를 최소화 하자.

만일 대칭 I/O 모드를 사용하되 인터럽트 벡터를 PIC 컨트롤러와 같이 설정한다면 IRQ와 벡터 핸들러를 거의 그대로 쓸 수 있어 수정할 부분이 많이 줄어든다.

PIC 컨트롤러는 마스터 PIC 컨트롤러에서 슬레이브 PIC 컨트롤러의 순서로 핀 번호를 할당한다.<BR>그리고 IPC 컨트롤러에 기준 인터럽트 벡터를 지정해주면 기준 인터럽트 벡터와 핀 번호를 더한 값이 인터럽트 벡터로 전달되었다.

하지만 I/O APIC는 다른데,<BR>인터럽트 벡터를 개별적으로 할당 할 수 있으므로, 각 핀에 연결된 IRQ의 순서가 PIC 컨트롤러와 달라도 된다.

그러므로 I/O APIC의 인터럽트 핀에 연결된 IRQ에 대해서 정확하게 파악해야 PIC 컨트롤러와 같은 벡터로 전달 할 수 있다.

<HR>

I/O APIC에 연결된 IRQ 정보를 얻을 수 있는 것은 BIOS이다.<BR>I/O APIC에 연결된 IRQ는 메인 보드 제조사가 결정해서 연결한다.

BIOS는 메인 보드 제조사가 만드는 프로그램이며, 이전에 살펴 보았던 MP 설정 테이블은<BR>프로세서 엔트리, 버스 엔트리, I/O APIC 엔트리, I/O 인터럽트 지정 엔트리, 로컬 인터럽트 지정 엔트리로 구성된다.

그 중에서 I/O 인터럽트 지정 엔트리가 바로 I/O APIC에 연결된 IRQ 정보를 나타낸다.

![image](https://user-images.githubusercontent.com/34773827/62619353-82d50980-b951-11e9-95d3-c84cf8c4a594.png)

I/O 인터럽트 지정 엔트리는 그림의 아래쪽처럼 인터럽트 타입과 플래그, IRQ가 전달되는 버스 ID와 IRQ 번호, 인터럽트가 연결된 I/O APIC의 ID와 핀 번호로 구성된다.

I/O 인터럽트 지정 엔트리는 I/O APIC에 연결된 인터럽트의 정보를 모두 나열하므로,<BR>엔트리를 순차적으로 따라가면 I/O APIC와 IRQ의 관계를 찾을 수 있다.

<HR>

I/O 리다이렉션 테이블을 설정하는 핵심 키워드는 ISA 버스이다.<BR>ISA 버스를 찾아야 I/O 인터럽트 지정 엔트리에서 ISA 버스와 관련된 엔트리를 구할 수 있으며,<BR>이를 통해 관련된 I/O APIC를 찾을 수 있다.

ISA버스와 관련된 I/O APIC까지 모두 찾았다면,<BR>이제 남은 일은 ISA 버스 ID와 일치하는 I/O 인터럽트 엔트리를 따라가면서 IRQ, 극성, 트리거 모드를 참고하여 I/O 리다이렉션 테이블을 설정하는 것이다.

다음 그림은 이러한 과정을 순차적으로 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62619524-f4ad5300-b951-11e9-9c5b-be6e295ea801.png)

<HR>

## 대칭 I/O 모드를 위한 자료구조 설계

#### I/O 리다이렉션 테이블의 자료구조와 매크로

I/O 리다이렉션 테이블은 여러 필드로 구성되므로 비트 오프셋으로 필드에 접근하기보다는 자료구조를 정의해서 사용하는 것이 편리하다.

I/O 리다이렉션 테이블 자료구조는 I/O APIC에서 테이블 정보를 읽거나 쓸 때 사용하는 자료구조이다.

```C
// 인터럽트 마스크(Interrupt Mask) 관련 매크로
#define IOAPIC_INTERRUPT_MASK                           0x01

// 트리거 모드(Trigger Mode) 관련 매크로
#define IOAPIC_TRIGGERMODE_LEVEL                        0x80
#define IOAPIC_TRIGGERMODE_EDGE                         0x00

// 리모트 IRR(Remote IRR) 관련 매크로
#define IOAPIC_REMOTEIRR_EOI                            0x40
#define IOAPIC_REMOTEIRR_ACCEPT                         0x00

// 인터럽트 입력 핀 극성(Interrupt Input Pin Polarity) 관련 매크로
#define IOAPIC_POLARITY_ACTIVELOW                       0x20
#define IOAPIC_POLARITY_ACTIVEHIGH                      0x00

// 전달 상태(Delivery Status) 관련 매크로
#define IOAPIC_DELIFVERYSTATUS_SENDPENDING              0x10
#define IOAPIC_DELIFVERYSTATUS_IDLE                     0x00

// 목적지 모드(Destination Mode) 관련 매크로
#define IOAPIC_DESTINATIONMODE_LOGICALMODE              0x08
#define IOAPIC_DESTINATIONMODE_PHYSICALMODE             0x00

// 전달 모드(Delivery Mode) 관련 매크로
#define IOAPIC_DELIVERYMODE_FIXED                       0x00
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY              0x01
#define IOAPIC_DELIVERYMODE_SMI                         0x02
#define IOAPIC_DELIVERYMODE_NMI                         0x04
#define IOAPIC_DELIVERYMODE_INIT                        0x05
#define IOAPIC_DELIVERYMODE_EXTINT                      0x07

// IO 리다이렉션 테이블의 자료구조
typedef struct kIORedirectionTableStruct
{
    // 인터럽트 벡터
    BYTE bVector;  
    
    // 트리거 모드, 리모트 IRR, 인터럽트 입력 핀 극성, 전달 상태, 목적지 모드, 
    // 전달 모드를 담당하는 필드 
    BYTE bFlagsAndDeliveryMode;
    
    // 인터럽트 마스크
    BYTE bInterruptMask;
    
    // 예약된 영역
    BYTE vbReserved[ 4 ];
    
    // 인터럽트를 전달할 APIC ID
    BYTE bDestination;
} IOREDIRECTIONTABLE;
```

<HR>

#### I/O APIC를 관리하는 자료구조와 매크로

I/O APIC를 관리하는 자료구조는 I/O APIC의 메모리 맵 I/O 어드레스를 저장하는 필드와 IRQ 인터럽트 입력 핀 매핑 테이블 필드로 구성된다.

```C
// IRQ를 I/O APIC의 인터럽트 입력 핀(INTIN)으로 대응시키는 테이블의 최대 크기
#define IOAPIC_MAXIRQTOINTINMAPCOUNT                    16

// I/O APIC를 관리하는 자료구조
typedef struct kIOAPICManagerStruct
{
    // ISA 버스가 연결된 I/O APIC의 메모리 맵 어드레스
    QWORD qwIOAPICBaseAddressOfISA;
    
    // IRQ와 I/O APIC의 인터럽트 입력 핀(INTIN)간의 연결 관계를 저장하는 테이블
    BYTE vbIRQToINTINMap[ IOAPIC_MAXIRQTOINTINMAPCOUNT ];    
} IOAPICMANAGER;
```

<HR>

## ISA 버스와 관련된 I/O APIC의 메모리 맵 I/O 어드레스 추출

우리가 사용하는 PC에는 인터럽트를 사용하는 디바이스가 그리 많지 않기 때문에 I/O APIC가 하나면 충분하다

그러므로 굳이 I/O APIC를 검색하지 않고,<BR>MP 설정 테이블에 있는 첫 번재 I/O APIC 엔트리르 사용해도 문제가 없지만 일반적인 방법을 살펴보자.

ISA 버스와 관련된 I/O APIC의 메모리 맵 I/O 어드레스를 찾으려면 가장 먼저 해야 할 일은 ISA 버스 엔트리를 찾는 것이다. ISA 버스 엔트리는 MP 설정 테이블의 버스 엔트리에서 찾을 수 있으며,<BR>버스 타입 문자열이 'ISA'인 것을 확인하면 된다.<BR>ISA 버스의 ID는 MP 설정 테이블을 분석하는 단계에서 저장해두므로 *kAnalysisMPConfigurationTable( )* 함수를 참고하면 된다.

ISA버스를 찾았다면 다음은 I/O 인터럽트 지정 엔트리를 검색하여 ISA 버스와 관련된 엔트리를 찾는다.<BR>MP 설정 테이블의 I/O 인터럽트 지정 엔트리는 인터럽트가 전달되는 버스의 ID 와 IRQ,  인터럽트가 연결된 I/O APIC의 ID와 인터럽트 입력 핀을 저장한다.

따라서 I/O 인터럽트 지정 엔트리의 버스 ID가 ISA 버스 ID와 일치하는 것을 찾으면 ISA 버스와 관련된 I/O APIC의 ID를 구할 수 있다.

#### ISA 버스와 관련된 I/O APIC 엔트리를 반환하는 함수<BR>- ISA 버스와 관련된 I/O 인터럽트 지정 엔트리 검색

```C
/**
 *  ISA 버스가 연결된 I/O APIC 엔트리를 검색
 *      kAnalysisMPConfigurationTable() 함수를 먼저 호출한 뒤에 사용해야 함
 */
IOAPICENTRY* kFindIOAPICEntryForISA( void )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    IOAPICENTRY* pstIOAPICEntry;
    QWORD qwEntryAddress;
    BYTE bEntryType;
    BOOL bFind = FALSE;
    int i;
    
    // MP 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
    pstMPHeader = gs_stMPConfigurationManager.pstMPConfigurationTableHeader;
    qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
    
    //==========================================================================
    // ISA 버스와 관련된 I/O 인터럽트 지정 엔트리를 검색
    //==========================================================================
    // 모든 엔트리를 돌면서 ISA 버스와 관련된 I/O 인터럽트 지정 엔트리만 검색
    for( i = 0 ; ( i < pstMPHeader->wEntryCount ) &&
                 ( bFind == FALSE ) ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // 프로세스 엔트리는 무시
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // 버스 엔트리, I/O APIC 엔트리, 로컬 인터럽트 지정 엔트리는 무시
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
            
            // IO 인터럽트 지정 엔트리이면, ISA 버스에 관련된 엔트리인지 확인
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) qwEntryAddress;
            // MP Configuration Manager 자료구조에 저장된 ISA 버스 ID와 비교
            if( pstIOAssignmentEntry->bSourceBUSID == 
                gs_stMPConfigurationManager.bISABusID )
            {
                bFind = TRUE;
            }                    
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        }
    }

    // 여기까지 왔는데 못 찾았다면 NULL을 반환
    if( bFind == FALSE )
    {
        return NULL;
    }
    
    ... 생략 ...
}
```

<hr>

#### ISA 버스와 관련된 I/O APIC 엔트리를 반환하는 함수<BR>- ISA 버스와 관련된 I/O APIC ID와 일치하는 I/O APIC 엔트리 검색

ISA 버스와 관련된 I/O 인터럽트 지정 엔트리를 찾았으니,<BR>I/O APIC ID와 일치하는 I/O APIC엔트리를 찾아야 한다.

```C
/**
 *  ISA 버스가 연결된 I/O APIC 엔트리를 검색
 *      kAnalysisMPConfigurationTable() 함수를 먼저 호출한 뒤에 사용해야 함
 */
IOAPICENTRY* kFindIOAPICEntryForISA( void )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    IOAPICENTRY* pstIOAPICEntry;
    QWORD qwEntryAddress;
    BYTE bEntryType;
    BOOL bFind = FALSE;
    int i;
    
    // MP 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
    pstMPHeader = gs_stMPConfigurationManager.pstMPConfigurationTableHeader;
    qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
    
    //==========================================================================
    // ISA 버스와 관련된 I/O 인터럽트 지정 엔트리를 검색
    //==========================================================================
    // 모든 엔트리를 돌면서 ISA 버스와 관련된 I/O 인터럽트 지정 엔트리만 검색
    for( i = 0 ; ( i < pstMPHeader->wEntryCount ) &&
                 ( bFind == FALSE ) ; i++ )
    {
        ... I/O 인터럽트 지정 엔트리를 찾는 코드 ...
    }

    // 여기까지 왔는데 못 찾았다면 NULL을 반환
    if( bFind == FALSE )
    {
        return NULL;
    }
    
    //==========================================================================
    // ISA 버스와 관련된 I/O APIC를 검색하여 I/O APIC의 엔트리를 반환
    //==========================================================================
    // 다시 엔트리를 돌면서 IO 인터럽트 지정 엔트리에 저장된 I/O APIC의 ID와 일치하는
    // 엔트리를 검색
    qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
    for( i = 0 ; i < pstMPHeader->wEntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // 프로세스 엔트리는 무시
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // 버스 엔트리, IO 인터럽트 지정 엔트리, 로컬 인터럽트 지정 엔트리는 무시
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
            
            // I/O APIC 엔트리이면 ISA 버스가 연결된 엔트리인지 확인하여 반환
        case MP_ENTRYTYPE_IOAPIC:
            pstIOAPICEntry = ( IOAPICENTRY* ) qwEntryAddress;
            if( pstIOAPICEntry->bIOAPICID == pstIOAssignmentEntry->bDestinationIOAPICID )
            {
                return pstIOAPICEntry;
            }
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        }
    }
    
    return NULL;
}
```

위의 함수를 보면 기능에 문제가 없지만 호출할 때마다 매번 MP 설정 테이블을 검색해야 하는 단점이 있다.

메모리 맵 I/O 어드레스는 우리가 수정하지 않는다면 변하지 않으므로,<BR>처음만 실제로 엔트리를 검색하여 찾고 그 이후는 자료구조에 저장된 값을 사용하는 것이 효율적이다.<BR>다음은 이러한 방식으로 작성된 함수이다.

I/O APIC 자료구조에 저장된 값이 NULL이면 엔트리를 읽어서 값을 저장한 뒤 반환하는 것을 볼 수 있다.

##### I/O APIC 자료구조에 저장된 값을 우선으로 사용하는 함수

```C
// I/O APIC를 관리하는 자료구조
static IOAPICMANAGER gs_stIOAPICManager;

/**
 *  ISA 버스가 연결된 I/O APIC의 기준 어드레스를 반환
 */
QWORD kGetIOAPICBaseAddressOfISA( void )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    IOAPICENTRY* pstIOAPICEntry;
    
    // I/O APIC의 어드레스가 저장되어 있지 않으면 엔트리를 찾아서 저장
    if( gs_stIOAPICManager.qwIOAPICBaseAddressOfISA == NULL )
    {
        pstIOAPICEntry = kFindIOAPICEntryForISA();
        if( pstIOAPICEntry != NULL )
        {
            gs_stIOAPICManager.qwIOAPICBaseAddressOfISA = 
                pstIOAPICEntry->dwMemoryMapAddress & 0xFFFFFFFF;
        }
    }

    // I/O APIC의 기준 어드레스를 찾아서 저장한 다음 반환
    return gs_stIOAPICManager.qwIOAPICBaseAddressOfISA;
}
```

<HR>

## I/O 리다이렉션 테이블에 인터럽트 마스크 설정

이전에 I/O APIC 엔트리까지 찾았으니 준비 작업이 모두 끝났으므로, 이제 I/O 리다이렉션 테이블을 설정한다

I/O 리다이렉션 테이블을 설정하느 ㄴ작업이 끝나기 전에 인터럽트가 발생한다면 잘못된 로컬 APIC 또는 인터럽트 벡터로 전달될 수 있다.

이러한 문제를 피하려면 I/O 리다이렉션 테이블을 설정하기 전에 24개를 모두 마스크하여 인터럽트가 발생하지 않도록 해야 한다.

I/O 리다이렉션 테이블의 하위 32비트 레지스터에는 인터럽트 마스크 필드가 있으며,<BR>이 비트를 1로 설정하면 인터럽트가 발생하지 않는다.

I/O 리다이렉션 테이블에는 인터럽트 마스크 필드 외에도 많은 필드가 있는데,<BR>인터럽트를 비활성화하려다가 기존 설정 값으 ㄹ지워버리면 나중에 활성화시 문제가 발생하니,<BR>I/O 리다이렉션 테이블을 읽은 후 인터럽트 마스크 필드만 1로 설정하여 다시 쓰는 것이 좋다.

<HR>

#### I/O APIC의 모든 인터럽트를 비활성화하는 함수

```C
// IO 리다이렉션 테이블의 최대 개수
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT               24

/**
 *  I/O APIC에 연결된 모든 인터럽트 핀을 마스크하여 인터럽트가 전달되지 않도록 함
 */
void kMaskAllInterruptInIOAPIC( void )
{
    IOREDIRECTIONTABLE stEntry;
    int i;
    
    // 모든 인터럽트를 비활성화
    for( i = 0 ; i < IOAPIC_MAXIOREDIRECTIONTABLECOUNT ; i++ )
    {
        // I/O 리다이렉션 테이블을 읽어서 인터럽트 마스크 필드(비트 0)를 1로 
        // 설정하여 다시 저장
        kReadIOAPICRedirectionTable( i, &stEntry );
        stEntry.bInterruptMask = IOAPIC_INTERRUPT_MASK;
        kWriteIOAPICRedirectionTable( i, &stEntry );
    }
}
```

위의 함수에서 사용하는 *kReadIOAPICRedirectionTable( )* 함수와 *kWriteIOAPICReadirectionTable( )* 함수는 각각 I/O 리다이렉션 테이블에서 값을 읽고 쓴다.

I/O APIC는 메모리 맵 I/O 방식으로 동작하며 앞서 살펴본 것과 같이 I/O 레지스터 선택 레지스터와 I/O 윈도우 레지스터만 메모리 어드레스를 할당한다.

따라서 I/O 리다이렉션 테이블 레지스터에 접근하려면 I/O 레지스터 선택 레지스터에 접근할 레지스터의 인덱스를 지정하고, I/O 윈도우 레지스터를 통해 값을 읽거나 써야 한다.

<HR>

#### I/O 리다이렉션 테이블을 읽는 함수

```C
// I/O APIC 레지스터 오프셋 관련 매크로
#define IOAPIC_REGISTER_IOREGISTERSELECTOR              0x00
#define IOAPIC_REGISTER_IOWINDOW                        0x10

// 위의 두 레지스터로 접근할 때 사용하는 레지스터의 인덱스
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE      0x10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE     0x11

/**
 *  인터럽트 입력 핀(INTIN)에 해당하는 I/O 리다이렉션 테이블에서 값을 읽음
 */
void kReadIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry )
{
    QWORD* pqwData;
    QWORD qwIOAPICBaseAddress;
    
    // ISA 버스가 연결된 I/O APIC의 메모리 맵 I/O 어드레스
    qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();
    
    // I/O 리다이렉션 테이블은 8바이트이므로, 8바이트 정수로 변환해서 처리
    pqwData = ( QWORD* ) pstEntry;
    
    //--------------------------------------------------------------------------
    // I/O 리다이렉션 테이블의 상위 4바이트를 읽음
    // I/O 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한 쌍이므로, INTIN에
    // 2를 곱하여 해당 I/O 리다이렉션 테이블 레지스터의 인덱스를 계산
    //--------------------------------------------------------------------------
    // I/O 레지스터 선택 레지스터(0xFEC00000)에 상위 I/O 리다이렉션 테이블 레지스터의
    // 인덱스를 전송
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) = 
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    // I/O 윈도우 레지스터(0xFEC00010)에서 상위 I/O 리다이렉션 테이블 레지스터의
    // 값을 읽음
    *pqwData = *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW );
    *pqwData = *pqwData << 32;
    
    //--------------------------------------------------------------------------
    // I/O 리다이렉션 테이블의 하위 4바이트를 읽음
    // I/O 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한 쌍이므로, INTIN에
    // 2를 곱하여 해당 I/O 리다이렉션 테이블 레지스터의 인덱스를 계산
    //--------------------------------------------------------------------------
    // I/O 레지스터 선택 레지스터(0xFEC00000)에 하위 I/O 리다이렉션 테이블 레지스터의
    // 인덱스를 전송
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2 ;
    // I/O 윈도우 레지스터(0xFEC00010)에서 하위 I/O 리다이렉션 테이블 레지스터의
    // 값을 읽음
    *pqwData |= *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW );
}
```

위의 코드를 보면 가장 먼저 하는 일이 ISA 버스가 연결된 I/O APIC의 메모리 맵 I/O 어드레스를 *kGetIOAPICBaseAddressOfISA( )* 함수로 구하는 것이다.<br>이 후 코드는 찾은 어드레스와 메모리 오프셋을 이용하여 I/O 레지스터 선택 레지스터와 I/O 윈도우 레지스터에 접근하고 있다.

I/O 리다이렉션 테이블을 읽으려면 먼저 I/O 레지스터 선택 레지스터에 I/O 리다이렉션 테이블 레지스터의 인덱스를 지정해야 한다. I/O 리다이렉션 테이블 레지스터는 I/O APIC의 인터럽트 입력 핀 수 만큼 존재하며, 인터럽트 입력 핀의 인덱스가 바로 I/O 리다이렉션 테이블의 인덱스가 된다.

<HR>

#### I/O 리다이렉션 테이블에 쓰는 함수

```C
/**
 *  인터럽트 입력 핀(INTIN)에 해당하는 I/O 리다이렉션 테이블에 값을 씀
 */
void kWriteIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry )
{
    QWORD* pqwData;
    QWORD qwIOAPICBaseAddress;
    
    // ISA 버스가 연결된 I/O APIC의 메모리 맵 I/O 어드레스
    qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();

    // I/O 리다이렉션 테이블은 8바이트이므로, 8바이트 정수로 변환해서 처리
    pqwData = ( QWORD* ) pstEntry;
    
    //--------------------------------------------------------------------------
    // I/O 리다이렉션 테이블에 상위 4바이트를 씀
    // I/O 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한 쌍이므로, INTIN에
    // 2를 곱하여 해당 I/O 리다이렉션 테이블 레지스터의 인덱스를 계산
    //--------------------------------------------------------------------------
    // I/O 레지스터 선택 레지스터(0xFEC00000)에 상위 I/O 리다이렉션 테이블 레지스터의
    // 인덱스를 전송
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) = 
        IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
    // I/O 윈도우 레지스터(0xFEC00010)에 상위 I/O 리다이렉션 테이블 레지스터의
    // 값을 씀
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW ) = *pqwData >> 32;
    
    //--------------------------------------------------------------------------
    // I/O 리다이렉션 테이블에 하위 4바이트를 씀
    // I/O 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한 쌍이므로, INTIN에
    // 2를 곱하여 해당 I/O 리다이렉션 테이블 레지스터의 인덱스를 계산
    //--------------------------------------------------------------------------
    // I/O 레지스터 선택 레지스터(0xFEC00000)에 하위 I/O 리다이렉션 테이블 레지스터의
    // 인덱스를 전송
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR ) =
        IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2 ;
    // I/O 윈도우 레지스터(0xFEC00010)에 하위 I/O 리다이렉션 테이블 레지스터의
    // 값을 씀
    *( DWORD* ) ( qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW ) = *pqwData;
}
```

<HR>

#### I/O 리다이렉션 테이블 자료구조에 필드를 설정하는 함수

I/O 리다이렉션 테이블 자료구조에 데이터를 삽입하는 함수이다.

각 필드에 일일이 값을 대입하는 번거로움을 피하려고 작성한 함수이며 코드는 다음과 같다.

```C
/**
 *  I/O 리다이렉션 테이블 자료구조에 값을 설정
 */
void kSetIOAPICRedirectionEntry( IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
        BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector )
{
    kMemSet( pstEntry, 0, sizeof( IOREDIRECTIONTABLE ) );
    
    pstEntry->bDestination = bAPICID;
    pstEntry->bFlagsAndDeliveryMode = bFlagsAndDeliveryMode;
    pstEntry->bInterruptMask = bInterruptMask;
    pstEntry->bVector = bVector;
}
```

<HR>

## I/O 리다이렉션 테이블 초기화

앞에서 살펴 본 과정은 I/O 리다이렉션 테이블을 초기화하는 데 필요한 정보를 모으거나 준비하는 단계이다.

이번 절 부터는 앞서 작성한 함수를 이용하여 본격적으로 I/O 리다이렉션 테이블을 설정한다.

I/O 리다이렉션 테이블을 설정하는 데 가장 중요한 점은 I/O APIC에 연결된 IRQ의 상태를 PIC 컨트롤러와 같은 상태로 만드는 것이다.

다음은 PIC 컨트롤러의 구성과 IRQ의 관계를 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62636048-d1939b00-b973-11e9-9b89-2b593c790a88.png)

<HR>

이제 I/O 리다이렉션 테이블을 설정하여 위 그림과 똑같이 만들어보자.

#### 인터럽트 벡터

인터럽트 벡터는 IRQ 0 ~ 15를 벡터 32 ~ 47 매핑하면 되므로, I/O 리다이렉션 테이블의 인터럽트 입력 핀에 전달되는 IRQ에 32를 더해서 인터럽트 벡터로 설정하면 된다.

그리고 인터럽트 입력 핀에 전달되는 IRQ에 대한 정보는 MP 설정 테이블의 I/O 인터럽트 지정 엔트리에 나와 있으므로, ISA 버스에 관계된 것만 찾아서 설정하면 된다.

#### 트리거 모드와 인터럽트 입력 핀 극성 필드

트리거 모드와 인터럽트 입력 핀 극성 필드는 처리할 때 주의가 필요하다.

버스에 연결된 디바이스에 따라 트리거 방식과 극성이 다르기 때문이다.

대표적인 버스 ISA와 PCI를 예로 들면,<BR>ISA 버스는 엣지 트리거와 1일 때 활성화(Active High)를 사용하는 반면,<br>PCI 버스는 레벨 트리거와 0일 때 활성화(Active Low)를 사용한다.

버스 타입 혹은 디바이스 타입에 따라 조합은 더 다양할 수 있지만  트리거 모드와 극성 정보 또한 MP 설정 테이블에 있는 I/O 인터럽트 지정 엔트리에 저장되어 있기 때문에 걱정할 필요가 없다.<BR>I/O 인터럽트 지정 엔트리의 I/O 인터럽트 플래그가 바로 그것이다.

I/O 인터럽트 플래그 필드는 트리거(비트 2 ~ 4)와 극성(비트 0 ~ 1)으로 나누어지며, 해당 필드를 참고해서 설정하면 된다.

#### 목적지 필드와 목적지 모드 필드

목적지 필드와 목적지 모드 필드는 인터럽트 입력 핀에 인터럽트가 발생했을 때,<BR>이를 전달할 APIC의 ID를 설정한다. 

별다른 설정이 필요 없는 물리 목적지 모드의 경우 0~0xFE를 설정하여 특정 로컬 APIC를 지정할 수도 있으며, 0xFF를 설정하여 모든 로컬 APIC로 전달할 수도 있다.

I/O 리다이렉션 테이블을 초기화하는 단계에서는 IRQ 0, 즉 PIT 인터럽트는 0xFF로 지정하여 모든 로컬 APIC로 전달하고 나머지는 0x00으로 지정하여 BSP로만 전달한다.

#### 전달 모드

전달 모드는 인터럽트를 전달하는 방식을 설정하는 필드이며,<BR>ISA 인터럽트만 처리하므로 고정 (FIXED) 모드로 설정하면 된다.

고정 모드는 목적지 필드에 저장된 로컬 APIC로 인터럽트 벡터 필드에 저장된 인터럽트를 전달하는 모드이다.

고정 모드를 사용할 때는 주의할 점이 있는데<BR>I/O 인터럽트 지정 엔트리에 저장된 인터럽트 타입 필드의 값이 인터럽트 타입(INT, 0)이어야 한다.

인터럽트 타입이 외부 인터럽트 또는 NMI 등의 타입이면 PIC 컨트롤러나 시스템에서 전달되는 인터럽트이므로, 해당 타입과 일치하는 전달 모드를 사용해야 한다.

인터럽트 타입이 아닌 인터럽트는 디바이스에서 직접 연결된 ISA 인터럽트가 아니므로,<BR>인터럽트를 비활성화해도 우리 OS가 동작하는 데 문제가 없다.

#### I/O 리다이렉션 테이블에 설정된 인터럽트 마스크 필드

I/O리다이렉션 테이블에 설정된 인터럽트 마스크 필드는 설정을 완료한 뒤에 해제하면 된다.

<HR>

### I/O 리다이렉션 테이블 초기화

I/O 리다이렉션 테이블을 초기화하는 데 필요한 필드를 모두 살펴보았으니,<BR>이제 초기화하는 코드를 살펴보자.

I/O 리다이렉션 테이블 초기화 함수는 크게 두 부분으로 나눌 수 있다.

첫 번째는 자료구조를 초기화하고 I/O APIC의 인터럽트를 모두 마스크하는 부분이며,

두 번째 I/O 인터럽트 지정 엔트리에 따라 I/O 리다이렉션 테이블을 설정하고 IRQ > INTIN 매핑 테이블을 구성하는 부분이다.

#### I/O 리다이렉션 테이블을 초기화하는 함수 <BR>- 자료구조를 초기화하고 모든 인터럽트를 발생 불가로 설정

```C
/**
 *  I/O APIC의 I/O 리다이렉션 테이블을 초기화
 */
void kInitializeIORedirectionTable( void )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    IOREDIRECTIONTABLE stIORedirectionEntry;
    QWORD qwEntryAddress;
    BYTE bEntryType;
    BYTE bDestination;
    int i;

    //==========================================================================
    // I/O APIC를 관리하는 자료구조를 초기화
    //==========================================================================
    kMemSet( &gs_stIOAPICManager, 0, sizeof( gs_stIOAPICManager ) );
    
    // I/O APIC의 메모리 맵 I/O 어드레스 저장, 아래 함수에서 내부적으로 처리함
    kGetIOAPICBaseAddressOfISA();
    
    // IRQ를 I/O APIC의 INTIN 핀과 연결한 테이블(IRQ->INTIN 매핑 테이블)을 초기화
    for( i = 0 ; i < IOAPIC_MAXIRQTOINTINMAPCOUNT ; i++ )
    {
        gs_stIOAPICManager.vbIRQToINTINMap[ i ] = 0xFF;
    }
    
    //==========================================================================
    // I/O APIC를 마스크하여 인터럽트가 발생하지 않도록 하고 I/O 리다이렉션 테이블 초기화
    //==========================================================================
    // 먼저 I/O APIC의 인터럽트를 마스크하여 인터럽트가 발생하지 않도록 함
    kMaskAllInterruptInIOAPIC();
    
    ... 생략 ...
}
```



#### I/O 리다이렉션 테이블 초기화 함수 <BR>- I/O 리다이렉션 테이블을 초기화 하고 IRQ->INTIN 매핑 테이블 구성

```C
/**
 *  I/O APIC의 I/O 리다이렉션 테이블을 초기화
 */
void kInitializeIORedirectionTable( void )
{
    ... 자료구조를 초기화하고 인터럽트를 불가능 하게 설정하는 코드 ...
        
    // IO 인터럽트 지정 엔트리 중에서 ISA 버스와 관련된 인터럽트만 추려서 I/O 리다이렉션
    // 테이블에 설정
    // MP 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
    pstMPManager = kGetMPConfigurationManager();
    pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
    qwEntryAddress = pstMPManager->qwBaseEntryStartAddress;
    
    // 모든 엔트리를 확인하여 ISA 버스와 관련된 I/O 인터럽트 지정 엔트리를 검색
    for( i = 0 ; i < pstMPHeader->wEntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // IO 인터럽트 지정 엔트리이면, ISA 버스인지 확인하여 I/O 리다이렉션
            // 테이블에 설정하고 IRQ->INITIN 매핑 테이블을 구성
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) qwEntryAddress;

            // 인터럽트 타입이 인터럽트(INT)인 것만 처리
            if( ( pstIOAssignmentEntry->bSourceBUSID == pstMPManager->bISABusID ) &&
                ( pstIOAssignmentEntry->bInterruptType == MP_INTERRUPTTYPE_INT ) )                        
            {
                // 목적지 필드는 IRQ 0를 제외하고 0x00으로 설정하여 Bootstrap Processor만 전달
                // IRQ 0는 스케줄러에 사용해야 하므로 0xFF로 설정하여 모든 코어로 전달
                if( pstIOAssignmentEntry->bSourceBUSIRQ == 0 )
                {
                    bDestination = 0xFF;
                }
                else
                {
                    bDestination = 0x00;
                }
                
                // ISA 버스는 엣지 트리거(Edge Trigger)와 1일 때 활성화(Active High)를
                // 사용
                // 목적지 모드는 물리 모드, 전달 모드는 고정(Fixed)으로 할당
                // 인터럽트 벡터는 PIC 컨트롤러의 벡터와 같이 0x20 + IRQ로 설정
                kSetIOAPICRedirectionEntry( &stIORedirectionEntry, bDestination, 
                    0x00, IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH |
                    IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED, 
                    PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBUSIRQ );
                
                // ISA 버스에서 전달된 IRQ는 I/O APIC의 INTIN 핀에 있으므로, INTIN 값을
                // 이용하여 처리
                kWriteIOAPICRedirectionTable( pstIOAssignmentEntry->bDestinationIOAPICINTIN, 
                        &stIORedirectionEntry );
                
                // IRQ와 인터럽트 입력 핀(INTIN)의 관계를 저장(IRQ->INTIN 매핑 테이블 구성)
                gs_stIOAPICManager.vbIRQToINTINMap[ pstIOAssignmentEntry->bSourceBUSIRQ ] =
                    pstIOAssignmentEntry->bDestinationIOAPICINTIN;                
            }                    
            qwEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
        
            // 프로세스 엔트리는 무시
        case MP_ENTRYTYPE_PROCESSOR:
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // 버스 엔트리, I/O APIC 엔트리, 로컬 인터럽트 지정 엔트리는 무시
        case MP_ENTRYTYPE_BUS:
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            qwEntryAddress += 8;
            break;
        }
    }  
}
```

<HR>

## 대칭 I/O 모드로 전환 문제점과 해결 방법

이제 앞서 작성한 I/O 리다이렉션 테이블 초기화 함수를 사용하여 대칭 I/O 모드로 전환해보자.

대칭 I/O 모드로 전환하는 과정은 <BR>PIC 모드 비활성화 > PIC 컨트롤러를 비활성화 > 로컬 APIC 활성화 > I/O 리다이렉션 테이블 초기화<BR>의 순서로 진행된다.

그리고 진행하는 과정에서 프로세서의 인터럽트 플래그를 비활성화하거나 활성화하여 전환 중에 발생하는 인터럽트를 무시하는 부분도 있다.

#### PIC 컨트롤러에서 I/O APIC로 전환하여 대칭 I/O 모드로 전환하는 코드

```C
/**
 *  대칭 I/O 모드(Symmetric I/O Mode)로 전환
 */
static void kStartSymmetricIOMode( const char* pcParameterBuffer )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    BOOL bInterruptFlag;

    // MP 설정 테이블을 분석
    if( kAnalysisMPConfigurationTable() == FALSE )
    {
        kPrintf( "Analyze MP Configuration Table Fail\n" );
        return ;
    }

    // MP 설정 매니저를 찾아서 PIC 모드인가 확인
    pstMPManager = kGetMPConfigurationManager();
    if( pstMPManager->bUsePICMode == TRUE )
    {
        // PIC 모드이면 I/O 포트 어드레스 0x22에 0x70을 먼저 전송하고 
        // I/O 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근하여
        // PIC 모드 비활성화
        kOutPortByte( 0x22, 0x70 );
        kOutPortByte( 0x23, 0x01 );
    }

    // PIC 컨트롤러의 인터럽트를 모두 마스크하여 인터럽트가 발생할 수 없도록 함
    kPrintf( "Mask All PIC Controller Interrupt\n" );
    kMaskPICInterrupt( 0xFFFF );

    // 프로세서 전체의 로컬 APIC를 활성화
    kPrintf( "Enable Global Local APIC\n" );
    kEnableGlobalLocalAPIC();
    
    // 현재 코어의 로컬 APIC를 활성화
    kPrintf( "Enable Software Local APIC\n" );
    kEnableSoftwareLocalAPIC();

    // 인터럽트를 불가로 설정
    kPrintf( "Disable CPU Interrupt Flag\n" );
    bInterruptFlag = kSetInterruptFlag( FALSE );
    
    // 모든 인터럽트를 수신할 수 있도록 태스크 우선 순위 레지스터를 0으로 설정
    kSetTaskPriority( 0 );

    // 로컬 APIC의 로컬 벡터 테이블을 초기화
    kInitializeLocalVectorTable();
    
    // I/O APIC 초기화
    kPrintf( "Initialize IO Redirection Table\n" );
    kInitializeIORedirectionTable();
        
    // 이전 인터럽트 플래그를 복원
    kPrintf( "Restore CPU Interrupt Flag\n" );
    kSetInterruptFlag( bInterruptFlag );
        
    kPrintf( "Change Symmetric I/O Mode Complete\n" );
}
```

<hr>

### 문제점

I/O 리다이렉션 테이블만 설정해서는 인터럽트가 제대로 처리되지 않는다.

사실 I/O 리다이렉션 테이블을 설정하는 부분은 아무런 문제가 없다.<BR>문제가 발생한 근본적인 요인은 EOI(End of Interrupt) 처리 때문이며,<BR>부가적인 요인으로는 로컬 APIC의 태스크 우선순위 레지스터가 있다.

그리고 로컬 APIC에 한 가지 더 덧붙이면 초기화 되지 않은 로컬 벡터 테이블도 문제가 될 수 있다.

![image](https://user-images.githubusercontent.com/34773827/62637918-6a77e580-b977-11e9-9dd6-d70ca7b012e7.png)

EOI 처리와 태스크 우선순위 레지스터, 로컬 벡터 테이블이 문제가 되는 이유는 위 그림을 보면 알 수 있다.

그림은 PC에서 인터럽트 요청이 발생할 수 있는 경로와 그 응답이 전달되는 경로를 간략히 나타낸 것이다.

인터럽트는 I/O APIC와 로컬 APIC에서 각각 발생할 수 있으며,<BR>벡터를 사용하는 인터럽트의 경우 태스크 우선순위 레지스터와 비교하여 높은 우선순위만 CPU로 전달된다.

인터럽트 응답은 EOI 레지스터를 통해서 전달되며 인터럽트가 발생한 위치에 따라서 로컬 APIC 내부에서 처리하거나 I/O APIC와 같은 외부 컨트롤러로 전달한다.

이 과정에서 어느 한 부분이라도 문제가 발생하면 인터럽트 처리에 문제가 생긴다.

<HR>

### EOI 처리

#### 공통 인터럽트 컨트롤러의 코드

인터럽트 문제의 근본적인 원인인 EOI 문제는 인터럽트를 처리하는 컨트롤러가 PIC 컨트롤러에서 로컬 APIC와 I/O APIC로 전환되었는데 여전히 EOI를 PIC 컨트롤러로 전송해서 생긴 것이다.

다음 코드에서 가장 마지막에 PIC 컨트롤러로 EOI를 전송하는 것을 볼 수 있다.

```C
/**
 *  공통으로 사용하는 인터럽트 핸들러 
 */
void kCommonInterruptHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = ( g_iCommonInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
    //=========================================================================
    
    // PIC 컨트롤러로 EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}
```

대칭 I/O 모드를 사용하면 PIC 컨트롤러가 아닌 I/O APIC와 로컬 APIC를 통해 인터럽트가 전달되므로,<BR>EOI 역시 I/O APIC와 로컬 APIC로 전달해야 한다.

로컬 APIC에게 EOI를 전송시키는 방법은 간단하다.

로컬 APIC에 있는 EOI 레지스터(0xFEE000B0)에 0x00을 전송하면 된다.

EOI레지스터에 값이 써지면 로컬 APIC는 지금 처리 중인 인터럽트에 대해서 인터럽트 응답을 I/O APIC에 전달한다. 그리고 로컬 APIC는 대기 중인 다른 인터럽트를 코어에 전달하며 모든 인터럽트가 처리 될 때 까지 이 과정을 반복하면서 인터럽트 처리가 끝난다.

<HR>

#### 로컬 APIC로 EOI를 전송하는 함수

다음은 로컬 APIC에 EOI를 전송하는 함수이다.

EOI 레지슽터는 로컬 APIC의 메모리 맵 I/O 기준 어드레스에서 0xB0만큼 떨어진 어드레스에 있으므로, <br>기준 어드레스를 이용하여 EOI 레지스터의 어드레스를 계산한뒤 0x00을 저장하도록 한다.

```C
/**
 *  로컬 APIC에 EOI(End of Interrupt)를 전송
 */
void kSendEOIToLocalAPIC( void )
{
    QWORD qwLocalAPICBaseAddress;
    
    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    
    // EOI 레지스터(0xFEE000B0)에 0x00을 출력하여 EOI를 전송
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_EOI ) = 0;
}
```

<hr>

#### 수정된 공통 인터럽트 핸들러의 코드<br>- PIC 컨트롤러와 로컬 APIC로 EOI를 전송

```C
/**
 *  공통으로 사용하는 인터럽트 핸들러 
 */
void kCommonInterruptHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = ( g_iCommonInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
    //=========================================================================
    
    // PIC 컨트롤러로 EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

    // 로컬 APIC로 EOI 전송
    kSendEOIToLocalAPIC();
}
```

<HR>

### 태스크 우선순위 레지스터 처리

로컬 APIC에 있는 태스크 우선순위 레지스터(TPR, Task Priority Register)는 CPU로 전달할 인터럽트의 우선순위를 저장하는 레지스터이다.

인터럽트의 우선순위는 벡터를 16으로 나눈 값이며,<BR>인터럽트 벡터가 0 ~ 0xFF 범위이므로 유효한 우선순위는 0 ~ 15 사이이다.

I/O 리다이렉션 테이블에 인터럽트 벡터를 지정할 때 IRQ+32를 지정했으므로,<BR>인터럽트를 수신하려면 적어도 우선순위를 0 또는 1로 설정해야 한다.

우리 OS에서는 인터럽트를 모두 수신하여 처리하므로 0으로 설정하면 된다.

#### 로컬 APIC에 태스크 우선순위를 저장하는 함수

태스크 우선순위 레지스터는 로컬 APIC의 메모리 맵 I/O 시작 어드레스에서 0x80 만큼 떨어진 위치에 있다.

태스크 우선순위 레지스터에 값을 설정하려면 EOI 레지스터와 같이 레지스터가 매핑된 메모리 어드레스에 값을 저장한다.

```C
/*
 *  태스크 우선 순위 레지스터(Task Priority Register) 설정
 */
void kSetTaskPriority( BYTE bPriority )
{
    QWORD qwLocalAPICBaseAddress;
    
    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
    
    // 태스크 우선 순위 레지스터(0xFEE00080)에 우선 순위 값을 전송
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_TASKPRIORITY ) = bPriority;
}
```

<hr>

### 로컬 인터럽트 벡터 처리

![image](https://user-images.githubusercontent.com/34773827/62639133-d9563e00-b979-11e9-9530-3f54627c991a.png)

#### 로컬 벡터 테이블 초기화 함수

다음은 로컬 벡터 테이블을 초기화 하는 함수이다.

```c
/**
 *  로컬 벡터 테이블 초기화
 */
void kInitializeLocalVectorTable( void )
{
    QWORD qwLocalAPICBaseAddress;
    DWORD dwTempValue;
    
    // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
    qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

    // 타이머 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해서 
    // LVT 타이머 레지스터(0xFEE00320)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_TIMER ) |= APIC_INTERRUPT_MASK;
    
    // LINT0 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해서
    // LVT LINT0 레지스터(0xFEE00350)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_LINT0 ) |= APIC_INTERRUPT_MASK;

    // LINT1 인터럽트는 NMI가 발생하도록 NMI로 설정하여 LVT LINT1 
    // 레지스터(0xFEE00360)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_LINT1 ) = APIC_TRIGGERMODE_EDGE | 
        APIC_POLARITY_ACTIVEHIGH | APIC_DELIVERYMODE_NMI;

    // 에러 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해서
    // LVT 에러 레지스터(0xFEE00370)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_ERROR ) |= APIC_INTERRUPT_MASK;

    // 성능 모니터링 카운터 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해서
    // LVT 성능 모니터링 카운터 레지스터(0xFEE00340)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + 
            APIC_REGISTER_PERFORMANCEMONITORINGCOUNTER ) |= APIC_INTERRUPT_MASK;

    // 온도 센서 인터럽트가 발생하지 않도록 기존 값에 마스크 값을 더해서
    // LVT 온도 센서 레지스터(0xFEE00330)에 저장
    *( DWORD* ) ( qwLocalAPICBaseAddress + APIC_REGISTER_THERMALSENSOR ) |= 
        APIC_INTERRUPT_MASK;
}
```

#### 수정된 대칭 I/O 모드로 전환하는 코드

```C
/**
 *  대칭 I/O 모드(Symmetric I/O Mode)로 전환
 */
static void kStartSymmetricIOMode( const char* pcParameterBuffer )
{
    MPCONFIGRUATIONMANAGER* pstMPManager;
    BOOL bInterruptFlag;

    // MP 설정 테이블을 분석
    if( kAnalysisMPConfigurationTable() == FALSE )
    {
        kPrintf( "Analyze MP Configuration Table Fail\n" );
        return ;
    }

    // MP 설정 매니저를 찾아서 PIC 모드인가 확인
    pstMPManager = kGetMPConfigurationManager();
    if( pstMPManager->bUsePICMode == TRUE )
    {
        // PIC 모드이면 I/O 포트 어드레스 0x22에 0x70을 먼저 전송하고 
        // I/O 포트 어드레스 0x23에 0x01을 전송하는 방법으로 IMCR 레지스터에 접근하여
        // PIC 모드 비활성화
        kOutPortByte( 0x22, 0x70 );
        kOutPortByte( 0x23, 0x01 );
    }

    // PIC 컨트롤러의 인터럽트를 모두 마스크하여 인터럽트가 발생할 수 없도록 함
    kPrintf( "Mask All PIC Controller Interrupt\n" );
    kMaskPICInterrupt( 0xFFFF );

    // 프로세서 전체의 로컬 APIC를 활성화
    kPrintf( "Enable Global Local APIC\n" );
    kEnableGlobalLocalAPIC();
    
    // 현재 코어의 로컬 APIC를 활성화
    kPrintf( "Enable Software Local APIC\n" );
    kEnableSoftwareLocalAPIC();

    // 인터럽트를 불가로 설정
    kPrintf( "Disable CPU Interrupt Flag\n" );
    bInterruptFlag = kSetInterruptFlag( FALSE );
    
    // 모든 인터럽트를 수신할 수 있도록 태스크 우선 순위 레지스터를 0으로 설정
    kSetTaskPriority( 0 );

    // 로컬 APIC의 로컬 벡터 테이블을 초기화
    kInitializeLocalVectorTable();
    
    // I/O APIC 초기화
    kPrintf( "Initialize IO Redirection Table\n" );
    kInitializeIORedirectionTable();
        
    // 이전 인터럽트 플래그를 복원
    kPrintf( "Restore CPU Interrupt Flag\n" );
    kSetInterruptFlag( bInterruptFlag );
        
    kPrintf( "Change Symmetric I/O Mode Complete\n" );
}
```

