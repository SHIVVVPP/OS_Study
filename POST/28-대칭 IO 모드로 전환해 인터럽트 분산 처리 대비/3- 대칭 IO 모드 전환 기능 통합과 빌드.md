# 대칭 I/O 모드 전환 기능 통합과 빌드

```
/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
```



## I/O APIC 파일 추가

#### 02.Kernel64/Source/IOAPIC.h

```c
#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
//  매크로
//
////////////////////////////////////////////////////////////////////////////////
// I/O APIC 레지스터 오프셋 관련 매크로
#define IOAPIC_REGISTER_IOREGISTERSELECTOR              0x00
#define IOAPIC_REGISTER_IOWINDOW                        0x10

// 위의 두 레지스터로 접근할 때 사용하는 레지스터의 인덱스
#define IOAPIC_REGISTERINDEX_IOAPICID                   0x00
#define IOAPIC_REGISTERINDEX_IOAPICVERSION              0x01
#define IOAPIC_REGISTERINDEX_IOAPICARBID                0x02
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE      0x10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE     0x11

// IO 리다이렉션 테이블의 최대 개수
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT               24

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

// IRQ를 I/O APIC의 인터럽트 입력 핀(INTIN)으로 대응시키는 테이블의 최대 크기
#define IOAPIC_MAXIRQTOINTINMAPCOUNT                    16

////////////////////////////////////////////////////////////////////////////////
//
//  구조체
//
////////////////////////////////////////////////////////////////////////////////
// 1바이트로 정렬
#pragma pack( push, 1 )

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

#pragma pack( pop )

// I/O APIC를 관리하는 자료구조
typedef struct kIOAPICManagerStruct
{
    // ISA 버스가 연결된 I/O APIC의 메모리 맵 어드레스
    QWORD qwIOAPICBaseAddressOfISA;
    
    // IRQ와 I/O APIC의 인터럽트 입력 핀(INTIN)간의 연결 관계를 저장하는 테이블
    BYTE vbIRQToINTINMap[ IOAPIC_MAXIRQTOINTINMAPCOUNT ];    
} IOAPICMANAGER;


////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
QWORD kGetIOAPICBaseAddressOfISA( void );
void kSetIOAPICRedirectionEntry( IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
        BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector );
void kReadIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry );
void kWriteIOAPICRedirectionTable( int iINTIN, IOREDIRECTIONTABLE* pstEntry );
void kMaskAllInterruptInIOAPIC( void );
void kInitializeIORedirectionTable( void );
void kPrintIRQToINTINMap( void );

#endif /*__IOAPIC_H__*/

```

#### 02.Kernel64/Source/IOAPIC.c

```c
#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "PIC.h"

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

/**
 *  IRQ와 I/O APIC의 인터럽트 핀(INT IN)간의 매핑 관계를 출력
 */
void kPrintIRQToINTINMap( void )
{
    int i;
    
    kPrintf( "=========== IRQ To I/O APIC INT IN Mapping Table ===========\n" );
    
    for( i = 0 ; i < IOAPIC_MAXIRQTOINTINMAPCOUNT ; i++ )
    {
        kPrintf( "IRQ[%d] -> INTIN [%d]\n", i, gs_stIOAPICManager.vbIRQToINTINMap[ i ] );
    }
}

```

<hr>

## 로컬 APIC 파일 수정

#### 02.Kernel64/Source/LocalAPIC.h

```c
#ifndef __LOCALAPIC_H__
#define __LOCALAPIC_H__

#include "Types.h"


////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
// 로컬 APIC 레지스터 오프셋 관련 매크로
#define APIC_REGISTER_EOI                           0x0000B0
#define APIC_REGISTER_SVR                           0x0000F0
#define APIC_REGISTER_APICID                        0x000020
#define APIC_REGISTER_TASKPRIORITY                  0x000080
#define APIC_REGISTER_TIMER                         0x000320
#define APIC_REGISTER_THERMALSENSOR                 0x000330
#define APIC_REGISTER_PERFORMANCEMONITORINGCOUNTER  0x000340
#define APIC_REGISTER_LINT0                         0x000350
#define APIC_REGISTER_LINT1                         0x000360
#define APIC_REGISTER_ERROR                         0x000370
#define APIC_REGISTER_ICR_LOWER                     0x000300
#define APIC_REGISTER_ICR_UPPER                     0x000310

// 전달 모드(Delivery Mode) 관련 매크로
#define APIC_DELIVERYMODE_FIXED                     0x000000
#define APIC_DELIVERYMODE_LOWESTPRIORITY            0x000100
#define APIC_DELIVERYMODE_SMI                       0x000200
#define APIC_DELIVERYMODE_NMI                       0x000400
#define APIC_DELIVERYMODE_INIT                      0x000500
#define APIC_DELIVERYMODE_STARTUP                   0x000600
#define APIC_DELIVERYMODE_EXTINT                    0x000700

// 목적지 모드(Destination Mode) 관련 매크로
#define APIC_DESTINATIONMODE_PHYSICAL               0x000000
#define APIC_DESTINATIONMODE_LOGICAL                0x000800

// 전달 상태(Delivery Status) 관련 매크로
#define APIC_DELIVERYSTATUS_IDLE                    0x000000
#define APIC_DELIVERYSTATUS_PENDING                 0x001000

// 레벨(Level) 관련 매크로
#define APIC_LEVEL_DEASSERT                         0x000000
#define APIC_LEVEL_ASSERT                           0x004000

// 트리거 모드(Trigger Mode) 관련 매크로
#define APIC_TRIGGERMODE_EDGE                       0x000000
#define APIC_TRIGGERMODE_LEVEL                      0x008000

// 목적지 약어(Destination Shorthand) 관련 매크로
#define APIC_DESTINATIONSHORTHAND_NOSHORTHAND       0x000000
#define APIC_DESTIANTIONSHORTHAND_SELF              0x040000
#define APIC_DESTINATIONSHORTHAND_ALLINCLUDINGSELF  0x080000
#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF  0x0C0000

/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
// 인터럽트 마스크(Interrupt Mask) 관련 매크로
#define APIC_INTERRUPT_MASK                         0x010000

// 타이머 모드(Timer Mode) 관련 매크로
#define APIC_TIMERMODE_PERIODIC                     0x020000
#define APIC_TIMERMODE_ONESHOT                      0x000000

// 인터럽트 입력 핀 극성(Interrupt Input Pin Polarity) 관련 매크로
#define APIC_POLARITY_ACTIVELOW                     0x002000
#define APIC_POLARITY_ACTIVEHIGH                    0x000000
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
QWORD kGetLocalAPICBaseAddress( void );
void kEnableSoftwareLocalAPIC( void );
/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
void kSendEOIToLocalAPIC( void );
void kSetTaskPriority( BYTE bPriority );
void kInitializeLocalVectorTable( void );
////////////////////////////////////////////////////////////////////////

#endif /*__LOCALAPIC_H__*/

```

#### 02.Kernel64/Source/LocalAPIC.c

```c
#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

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

/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////
```

<hr>

## MP 설정 테이블 파일과 인터럽트 핸들러 파일 수정

#### 02.Kernel64/Source/MPConfigurationTable.h

```c
#ifndef __MPCONFIGURATIONTABLE__
#define __MPCONFIGURATIONTABLE__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
// MP 플로팅 포인터의 특성 바이트(Feature Byte)
#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE  0x00
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE     0x80

// 엔트리 타입(Entry Type)
#define MP_ENTRYTYPE_PROCESSOR                  0
#define MP_ENTRYTYPE_BUS                        1
#define MP_ENTRYTYPE_IOAPIC                     2
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT      3
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT   4

// 프로세서 CPU 플래그
#define MP_PROCESSOR_CPUFLAGS_ENABLE            0x01
#define MP_PROCESSOR_CPUFLAGS_BSP               0x02

// 버스 타입 스트링(Bus Type String)
#define MP_BUS_TYPESTRING_ISA                   "ISA"
#define MP_BUS_TYPESTRING_PCI                   "PCI"
#define MP_BUS_TYPESTRING_PCMCIA                "PCMCIA"
#define MP_BUS_TYPESTRING_VESALOCALBUS          "VL"

// 인터럽트 타입(Interrupt Type)
#define MP_INTERRUPTTYPE_INT                    0
#define MP_INTERRUPTTYPE_NMI                    1
#define MP_INTERRUPTTYPE_SMI                    2
#define MP_INTERRUPTTYPE_EXTINT                 3

// 인터럽트 플래그(Interrupt Flags)
#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY      0x00
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH           0x01
#define MP_INTERRUPT_FLAGS_ACTIVELOW            0x03
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER       0x00
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED        0x04
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED       0x0C

////////////////////////////////////////////////////////////////////////////////
//
// 구조체
//
////////////////////////////////////////////////////////////////////////////////
// 1바이트로 정렬
#pragma pack( push, 1 )

// MP 플로팅 포인터 자료구조(MP Floating Pointer Data Structure)
typedef struct kMPFloatingPointerStruct
{
    // 시그너처, _MP_
    char vcSignature[ 4 ]; 
    // MP 설정 테이블이 위치하는 어드레스
    DWORD dwMPConfigurationTableAddress;
    // MP 플로팅 포인터 자료구조의 길이, 16 바이트
    BYTE bLength;
    // MultiProcessor Specification의 버전
    BYTE bRevision;
    // 체크섬
    BYTE bCheckSum;
    // MP 특성 바이트 1~5
    BYTE vbMPFeatureByte[ 5 ];
} MPFLOATINGPOINTER;

// MP 설정 테이블 헤더(MP Configuration Table Header) 자료구조
typedef struct kMPConfigurationTableHeaderStruct
{
    // 시그너처, PCMP
    char vcSignature[ 4 ];
    // 기본 테이블 길이
    WORD wBaseTableLength;
    // MultiProcessor Specification의 버전
    BYTE bRevision;
    // 체크섬
    BYTE bCheckSum;
    // OEM ID 문자열
    char vcOEMIDString[ 8 ];
    // PRODUCT ID 문자열
    char vcProductIDString[ 12 ];
    // OEM이 정의한 테이블의 어드레스
    DWORD dwOEMTablePointerAddress;
    // OEM이 정의한 테이블의 크기
    WORD wOEMTableSize;
    // 기본 MP 설정 테이블 엔트리의 개수
    WORD wEntryCount;
    // 로컬 APIC의 메모리 맵 I/O 어드레스
    DWORD dwMemoryMapIOAddressOfLocalAPIC;
    // 확장 테이블의 길이
    WORD wExtendedTableLength;
    // 확장 테이블의 체크섬
    BYTE bExtendedTableChecksum;
    // 예약됨
    BYTE bReserved;
} MPCONFIGURATIONTABLEHEADER;

// 프로세서 엔트리 자료구조(Processor Entry)
typedef struct kProcessorEntryStruct
{
    // 엔트리 타입 코드, 0
    BYTE bEntryType;
    // 프로세서에 포함된 로컬 APIC의 ID
    BYTE bLocalAPICID;
    // 로컬 APIC의 버전
    BYTE bLocalAPICVersion;
    // CPU 플래그
    BYTE bCPUFlags;
    // CPU 시그너처
    BYTE vbCPUSignature[ 4 ];
    // 특성 플래그
    DWORD dwFeatureFlags;
    // 예약됨
    DWORD vdwReserved[ 2 ];
} PROCESSORENTRY;

// 버스 엔트리 자료구조(Bus Entry)
typedef struct kBusEntryStruct
{
    // 엔트리 타입 코드, 1
    BYTE bEntryType;
    // 버스 ID
    BYTE bBusID;
    // 버스 타입 문자열
    char vcBusTypeString[ 6 ];
} BUSENTRY;

// I/O APIC 엔트리 자료구조(I/O APIC Entry)
typedef struct kIOAPICEntryStruct
{
    // 엔트리 타입 코드, 2
    BYTE bEntryType;
    // I/O APIC ID
    BYTE bIOAPICID;
    // I/O APIC 버전
    BYTE bIOAPICVersion;
    // I/O APIC 플래그
    BYTE bIOAPICFlags;
    // 메모리 맵 I/O 어드레스
    DWORD dwMemoryMapAddress;
} IOAPICENTRY;

// I/O 인터럽트 지정 엔트리 자료구조(I/O Interrupt Assignment Entry)
typedef struct kIOInterruptAssignmentEntryStruct
{
    // 엔트리 타입 코드, 3
    BYTE bEntryType;
    // 인터럽트 타입
    BYTE bInterruptType;
    // I/O 인터럽트 플래그
    WORD wInterruptFlags;
    // 발생한 버스 ID
    BYTE bSourceBUSID;
    // 발생한 버스 IRQ
    BYTE bSourceBUSIRQ;
    // 전달할 I/O APIC ID
    BYTE bDestinationIOAPICID;
    // 전달할 I/O APIC INTIN
    BYTE bDestinationIOAPICINTIN;
} IOINTERRUPTASSIGNMENTENTRY;

// 로컬 인터럽트 지정 엔트리 자료구조(Local Interrupt Assignment Entry)
typedef struct kLocalInterruptEntryStruct
{
    // 엔트리 타입 코드, 4
    BYTE bEntryType;
    // 인터럽트 타입
    BYTE bInterruptType;
    // 로컬 인터럽트 플래그
    WORD wInterruptFlags;
    // 발생한 버스 ID
    BYTE bSourceBUSID;
    // 발생한 버스 IRQ
    BYTE bSourceBUSIRQ;
    // 전달할 로컬 APIC ID
    BYTE bDestinationLocalAPICID;
    // 전달할 로컬 APIC INTIN
    BYTE bDestinationLocalAPICLINTIN;
} LOCALINTERRUPTASSIGNMENTENTRY;

#pragma pack( pop)

// MP 설정 테이블을 관리하는 자료구조
typedef struct kMPConfigurationManagerStruct
{
    // MP 플로팅 테이블
    MPFLOATINGPOINTER* pstMPFloatingPointer;
    // MP 설정 테이블 헤더
    MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;
    // 기본 MP 설정 테이블 엔트리의 시작 어드레스
    QWORD qwBaseEntryStartAddress;
    // 프로세서 또는 코어의 수
    int iProcessorCount;
    // PIC 모드 지원 여부
    BOOL bUsePICMode;
    // ISA 버스의 ID
    BYTE bISABusID;
} MPCONFIGRUATIONMANAGER;


////////////////////////////////////////////////////////////////////////////////
//
// 함수
//
////////////////////////////////////////////////////////////////////////////////
BOOL kFindMPFloatingPointerAddress( QWORD* pstAddress );
BOOL kAnalysisMPConfigurationTable( void );
MPCONFIGRUATIONMANAGER* kGetMPConfigurationManager( void );
void kPrintMPConfigurationTable( void );
int kGetProcessorCount( void );
/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
IOAPICENTRY* kFindIOAPICEntryForISA( void );
////////////////////////////////////////////////////////////////////////

#endif /*__MPCONFIGURATIONTABLE__*/

```

#### 02.Kernel64/Source/MPConfigurationTable.c

```c
#include "MPConfigurationTable.h"
#include "Console.h"


// MP 설정 테이블을 관리하는 자료구조
static MPCONFIGRUATIONMANAGER gs_stMPConfigurationManager = { 0, };

/**
 *  BIOS의 영역에서 MP Floating Header를 찾아서 그 주소를 반환
 */
BOOL kFindMPFloatingPointerAddress( QWORD* pstAddress )
{
    char* pcMPFloatingPointer;
    QWORD qwEBDAAddress;
    QWORD qwSystemBaseMemory;
/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
    // 확장 BIOS 데이터 영역과 시스템 기본 메모리를 출력
    //kPrintf( "Extended BIOS Data Area = [0x%X] \n", 
    //        ( DWORD ) ( *( WORD* ) 0x040E ) * 16 );
    //kPrintf( "System Base Address = [0x%X]\n", 
    //        ( DWORD ) ( *( WORD* ) 0x0413 ) * 1024 );
////////////////////////////////////////////////////////////////////////
    
    // 확장 BIOS 데이터 영역을 검색하여 MP 플로팅 포인터를 찾음
    // 확장 BIOS 데이터 영역은 0x040E에서 세그먼트의 시작 어드레스를 찾을 수 있음
    qwEBDAAddress = *( WORD* ) ( 0x040E );
    // 세그먼트의 시작 어드레스이므로 16을 곱하여 실제 물리 어드레스로 변환
    qwEBDAAddress *= 16;
    
    for( pcMPFloatingPointer = ( char* ) qwEBDAAddress ; 
         ( QWORD ) pcMPFloatingPointer <= ( qwEBDAAddress + 1024 ) ; 
         pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            /////////////////////////////////////////////////////////////////////////
            //
            // 대칭 I/O 모드 전환 기능 통합과 빌드
            //
            ////////////////////////////////////////////////////////////////////////
            //kPrintf( "MP Floating Pointer Is In EBDA, [0x%X] Address\n", 
            //         ( QWORD ) pcMPFloatingPointer );
            ////////////////////////////////////////////////////////////////////////
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    // 시스템 기본 메모리의 끝부분에서 1Kbyte 미만인 영역을 검색하여 MP 플로팅 포인터를
    // 찾음
    // 시스템 기본 메모리는 0x0413에서 Kbyte 단위로 정렬된 값을 찾을 수 있음
    qwSystemBaseMemory = *( WORD* ) 0x0413;
    // Kbyte 단위로 저장된 값이므로 1024를 곱해 실제 물리 어드레스로 변환
    qwSystemBaseMemory *= 1024;

    for( pcMPFloatingPointer = ( char* ) ( qwSystemBaseMemory - 1024 ) ; 
        ( QWORD ) pcMPFloatingPointer <= qwSystemBaseMemory ; 
        pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            /////////////////////////////////////////////////////////////////////////
            //
            // 대칭 I/O 모드 전환 기능 통합과 빌드
            //
            ////////////////////////////////////////////////////////////////////////
            // kPrintf( "MP Floating Pointer Is In System Base Memory, [0x%X] Address\n",
            //          ( QWORD ) pcMPFloatingPointer );
            ////////////////////////////////////////////////////////////////////////
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }
    
    // BIOS의 ROM 영역을 검색하여 MP 플로팅 포인터를 찾음
    for( pcMPFloatingPointer = ( char* ) 0x0F0000; 
         ( QWORD) pcMPFloatingPointer < 0x0FFFFF; pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            /////////////////////////////////////////////////////////////////////////
            //
            // 대칭 I/O 모드 전환 기능 통합과 빌드
            //
            ////////////////////////////////////////////////////////////////////////
            // kPrintf( "MP Floating Pointer Is In ROM, [0x%X] Address\n", 
            //          pcMPFloatingPointer );
            ////////////////////////////////////////////////////////////////////////
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 *  MP 설정 테이블을 분석해서 필요한 정보를 저장하는 함수
 */
BOOL kAnalysisMPConfigurationTable( void )
{
    QWORD qwMPFloatingPointerAddress;
    MPFLOATINGPOINTER* pstMPFloatingPointer;
    MPCONFIGURATIONTABLEHEADER* pstMPConfigurationHeader;
    BYTE bEntryType;
    WORD i;
    QWORD qwEntryAddress;
    PROCESSORENTRY* pstProcessorEntry;
    BUSENTRY* pstBusEntry;
    
    // 자료구조 초기화
    kMemSet( &gs_stMPConfigurationManager, 0, sizeof( MPCONFIGRUATIONMANAGER ) );
    gs_stMPConfigurationManager.bISABusID = 0xFF;
    
    // MP 플로팅 포인터의 어드레스를 구함
    if( kFindMPFloatingPointerAddress( &qwMPFloatingPointerAddress ) == FALSE )
    {
        return FALSE;
    }
    
    // MP 플로팅 테이블 설정
    pstMPFloatingPointer = ( MPFLOATINGPOINTER* ) qwMPFloatingPointerAddress;
    gs_stMPConfigurationManager.pstMPFloatingPointer = pstMPFloatingPointer;
    pstMPConfigurationHeader = ( MPCONFIGURATIONTABLEHEADER* ) 
        ( ( QWORD ) pstMPFloatingPointer->dwMPConfigurationTableAddress & 0xFFFFFFFF );
    
    // PIC 모드 지원 여부 저장
    if( pstMPFloatingPointer->vbMPFeatureByte[ 1 ] & 
            MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE )
    {
        gs_stMPConfigurationManager.bUsePICMode = TRUE;
    }    
    
    // MP 설정 테이블 헤더와 기본 MP 설정 테이블 엔트리의 시작 어드레스 설정
    gs_stMPConfigurationManager.pstMPConfigurationTableHeader = 
        pstMPConfigurationHeader;
    gs_stMPConfigurationManager.qwBaseEntryStartAddress = 
        pstMPFloatingPointer->dwMPConfigurationTableAddress + 
        sizeof( MPCONFIGURATIONTABLEHEADER );
    
    // 모든 엔트리를 돌면서 프로세서의 코어 수를 계산하고 ISA 버스를 검색하여 ID를 저장
    qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
    for( i = 0 ; i < pstMPConfigurationHeader->wEntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwEntryAddress;
        switch( bEntryType )
        {
            // 프로세서 엔트리이면 프로세서의 수를 하나 증가시킴
        case MP_ENTRYTYPE_PROCESSOR:
            pstProcessorEntry = ( PROCESSORENTRY* ) qwEntryAddress;
            if( pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE )
            {
                gs_stMPConfigurationManager.iProcessorCount++;
            }
            qwEntryAddress += sizeof( PROCESSORENTRY );
            break;
            
            // 버스 엔트리이면 ISA 버스인지 확인하여 저장
        case MP_ENTRYTYPE_BUS:
            pstBusEntry = ( BUSENTRY* ) qwEntryAddress;
            if( kMemCmp( pstBusEntry->vcBusTypeString, MP_BUS_TYPESTRING_ISA,
                    kStrLen( MP_BUS_TYPESTRING_ISA ) ) == 0 )
            {
                gs_stMPConfigurationManager.bISABusID = pstBusEntry->bBusID;
            }
            qwEntryAddress += sizeof( BUSENTRY );
            break;
            
            // 기타 엔트리는 무시하고 이동
        case MP_ENTRYTYPE_IOAPIC:
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
        default:
            qwEntryAddress += 8;
            break;
        }
    }
    return TRUE;
}

/**
 *  MP 설정 테이블을 관리하는 자료구조를 반환
 */
MPCONFIGRUATIONMANAGER* kGetMPConfigurationManager( void )
{
    return &gs_stMPConfigurationManager;
}

/**
 *  MP 설정 테이블의 정보를 모두 화면에 출력
 */
void kPrintMPConfigurationTable( void )
{
    MPCONFIGRUATIONMANAGER* pstMPConfigurationManager;
    QWORD qwMPFloatingPointerAddress;
    MPFLOATINGPOINTER* pstMPFloatingPointer;
    MPCONFIGURATIONTABLEHEADER* pstMPTableHeader;
    PROCESSORENTRY* pstProcessorEntry;
    BUSENTRY* pstBusEntry;
    IOAPICENTRY* pstIOAPICEntry;
    IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
    LOCALINTERRUPTASSIGNMENTENTRY* pstLocalAssignmentEntry;
    QWORD qwBaseEntryAddress;
    char vcStringBuffer[ 20 ];
    WORD i;
    BYTE bEntryType;
    // 화면에 출력할 문자열
    char* vpcInterruptType[ 4 ] = { "INT", "NMI", "SMI", "ExtINT" };
    char* vpcInterruptFlagsPO[ 4 ] = { "Conform", "Active High", 
            "Reserved", "Active Low" };
    char* vpcInterruptFlagsEL[ 4 ] = { "Conform", "Edge-Trigger", "Reserved", 
            "Level-Trigger"};

    //==========================================================================
    // MP 설정 테이블 처리 함수를 먼저 호출하여 시스템 처리에 필요한 정보를 저장
    //==========================================================================
    kPrintf( "================ MP Configuration Table Summary ================\n" );
    pstMPConfigurationManager = kGetMPConfigurationManager();
    if( ( pstMPConfigurationManager->qwBaseEntryStartAddress == 0 ) &&
        ( kAnalysisMPConfigurationTable() == FALSE ) )
    {
        kPrintf( "MP Configuration Table Analysis Fail\n" );
        return ;
    }
    kPrintf( "MP Configuration Table Analysis Success\n" );

    kPrintf( "MP Floating Pointer Address : 0x%Q\n", 
            pstMPConfigurationManager->pstMPFloatingPointer );
    kPrintf( "PIC Mode Support : %d\n", pstMPConfigurationManager->bUsePICMode );
    kPrintf( "MP Configuration Table Header Address : 0x%Q\n",
            pstMPConfigurationManager->pstMPConfigurationTableHeader );
    kPrintf( "Base MP Configuration Table Entry Start Address : 0x%Q\n",
            pstMPConfigurationManager->qwBaseEntryStartAddress );
    kPrintf( "Processor Count : %d\n", pstMPConfigurationManager->iProcessorCount );
    kPrintf( "ISA Bus ID : %d\n", pstMPConfigurationManager->bISABusID );

    kPrintf( "Press any key to continue... ('q' is exit) : " );
    if( kGetCh() == 'q' )
    {
        kPrintf( "\n" );
        return ;
    }
    kPrintf( "\n" );            
    
    //==========================================================================
    // MP 플로팅 포인터 정보를 출력
    //==========================================================================
    kPrintf( "=================== MP Floating Pointer ===================\n" );
    pstMPFloatingPointer = pstMPConfigurationManager->pstMPFloatingPointer;
    kMemCpy( vcStringBuffer, pstMPFloatingPointer->vcSignature, 4 );
    vcStringBuffer[ 4 ] = '\0';
    kPrintf( "Signature : %s\n", vcStringBuffer );
    kPrintf( "MP Configuration Table Address : 0x%Q\n", 
            pstMPFloatingPointer->dwMPConfigurationTableAddress );
    kPrintf( "Length : %d * 16 Byte\n", pstMPFloatingPointer->bLength );
    kPrintf( "Version : %d\n", pstMPFloatingPointer->bRevision );
    kPrintf( "CheckSum : 0x%X\n", pstMPFloatingPointer->bCheckSum );
    kPrintf( "Feature Byte 1 : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[ 0 ] );
    // MP 설정 테이블 사용 여부 출력
    if( pstMPFloatingPointer->vbMPFeatureByte[ 0 ] == 0 )
    {
        kPrintf( "(Use MP Configuration Table)\n" );
    }
    else
    {
        kPrintf( "(Use Default Configuration)\n" );
    }    
    // PIC 모드 지원 여부 출력
    kPrintf( "Feature Byte 2 : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[ 1 ] );
    if( pstMPFloatingPointer->vbMPFeatureByte[ 2 ] & 
            MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE )
    {
        kPrintf( "(PIC Mode Support)\n" );
    }
    else
    {
        kPrintf( "(Virtual Wire Mode Support)\n" );
    }
    
    //==========================================================================
    // MP 설정 테이블 헤더 정보를 출력
    //==========================================================================
    kPrintf( "\n=============== MP Configuration Table Header ===============\n" );
    pstMPTableHeader = pstMPConfigurationManager->pstMPConfigurationTableHeader;
    kMemCpy( vcStringBuffer, pstMPTableHeader->vcSignature, 4 );
    vcStringBuffer[ 4 ] = '\0';
    kPrintf( "Signature : %s\n", vcStringBuffer );
    kPrintf( "Length : %d Byte\n", pstMPTableHeader->wBaseTableLength );
    kPrintf( "Version : %d\n", pstMPTableHeader->bRevision );
    kPrintf( "CheckSum : 0x%X\n", pstMPTableHeader->bCheckSum );
    kMemCpy( vcStringBuffer, pstMPTableHeader->vcOEMIDString, 8 );
    vcStringBuffer[ 8 ] = '\0';
    kPrintf( "OEM ID String : %s\n", vcStringBuffer );
    kMemCpy( vcStringBuffer, pstMPTableHeader->vcProductIDString, 12 );
    vcStringBuffer[ 12 ] = '\0';
    kPrintf( "Product ID String : %s\n", vcStringBuffer );
    kPrintf( "OEM Table Pointer : 0x%X\n", 
             pstMPTableHeader->dwOEMTablePointerAddress );
    kPrintf( "OEM Table Size : %d Byte\n", pstMPTableHeader->wOEMTableSize );
    kPrintf( "Entry Count : %d\n", pstMPTableHeader->wEntryCount );
    kPrintf( "Memory Mapped I/O Address Of Local APIC : 0x%X\n",
            pstMPTableHeader->dwMemoryMapIOAddressOfLocalAPIC );
    kPrintf( "Extended Table Length : %d Byte\n", 
            pstMPTableHeader->wExtendedTableLength );
    kPrintf( "Extended Table Checksum : 0x%X\n", 
            pstMPTableHeader->bExtendedTableChecksum );
    
    kPrintf( "Press any key to continue... ('q' is exit) : " );
    if( kGetCh() == 'q' )
    {
        kPrintf( "\n" );
        return ;
    }
    kPrintf( "\n" );
    
    //==========================================================================
    // 기본 MP 설정 테이블 엔트리 정보를 출력
    //==========================================================================
    kPrintf( "\n============= Base MP Configuration Table Entry =============\n" );
    qwBaseEntryAddress = pstMPFloatingPointer->dwMPConfigurationTableAddress + 
        sizeof( MPCONFIGURATIONTABLEHEADER );
    for( i = 0 ; i < pstMPTableHeader->wEntryCount ; i++ )
    {
        bEntryType = *( BYTE* ) qwBaseEntryAddress;
        switch( bEntryType )
        {
            // 프로세스 엔트리 정보 출력
        case MP_ENTRYTYPE_PROCESSOR:
            pstProcessorEntry = ( PROCESSORENTRY* ) qwBaseEntryAddress;
            kPrintf( "Entry Type : Processor\n" );
            kPrintf( "Local APIC ID : %d\n", pstProcessorEntry->bLocalAPICID );
            kPrintf( "Local APIC Version : 0x%X\n", pstProcessorEntry->bLocalAPICVersion );
            kPrintf( "CPU Flags : 0x%X ", pstProcessorEntry->bCPUFlags );
            // Enable/Disable 출력
            if( pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE )
            {
                kPrintf( "(Enable, " );
            }
            else
            {
                kPrintf( "(Disable, " );
            }
            // BSP/AP 출력
            if( pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_BSP )
            {
                kPrintf( "BSP)\n" );
            }
            else
            {
                kPrintf( "AP)\n" );
            }            
            kPrintf( "CPU Signature : 0x%X\n", pstProcessorEntry->vbCPUSignature );
            kPrintf( "Feature Flags : 0x%X\n\n", pstProcessorEntry->dwFeatureFlags );

            // 프로세스 엔트리의 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
            qwBaseEntryAddress += sizeof( PROCESSORENTRY );
            break;

            // 버스 엔트리 정보 출력
        case MP_ENTRYTYPE_BUS:
            pstBusEntry = ( BUSENTRY* ) qwBaseEntryAddress;
            kPrintf( "Entry Type : Bus\n" );
            kPrintf( "Bus ID : %d\n", pstBusEntry->bBusID );
            kMemCpy( vcStringBuffer, pstBusEntry->vcBusTypeString, 6 );
            vcStringBuffer[ 6 ] = '\0';
            kPrintf( "Bus Type String : %s\n\n", vcStringBuffer );
            
            // 버스 엔트리의 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
            qwBaseEntryAddress += sizeof( BUSENTRY );
            break;
            
            // I/O APIC 엔트리
        case MP_ENTRYTYPE_IOAPIC:
            pstIOAPICEntry = ( IOAPICENTRY* ) qwBaseEntryAddress;
            kPrintf( "Entry Type : I/O APIC\n" );
            kPrintf( "I/O APIC ID : %d\n", pstIOAPICEntry->bIOAPICID );
            kPrintf( "I/O APIC Version : 0x%X\n", pstIOAPICEntry->bIOAPICVersion );
            kPrintf( "I/O APIC Flags : 0x%X ", pstIOAPICEntry->bIOAPICFlags );
            // Enable/Disable 출력
            if( pstIOAPICEntry->bIOAPICFlags == 1 )
            {
                kPrintf( "(Enable)\n" );
            }
            else
            {
                kPrintf( "(Disable)\n" );
            }
            kPrintf( "Memory Mapped I/O Address : 0x%X\n\n", 
                    pstIOAPICEntry->dwMemoryMapAddress );

            // I/O APIC 엔트리의 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
            qwBaseEntryAddress += sizeof( IOAPICENTRY );
            break;
            
            // I/O 인터럽트 지정 엔트리
        case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
            pstIOAssignmentEntry = ( IOINTERRUPTASSIGNMENTENTRY* ) 
                qwBaseEntryAddress;
            kPrintf( "Entry Type : I/O Interrupt Assignment\n" );
            kPrintf( "Interrupt Type : 0x%X ", pstIOAssignmentEntry->bInterruptType );
            // 인터럽트 타입 출력
            kPrintf( "(%s)\n", vpcInterruptType[ pstIOAssignmentEntry->bInterruptType ] );
            kPrintf( "I/O Interrupt Flags : 0x%X ", pstIOAssignmentEntry->wInterruptFlags );
            // 극성과 트리거 모드 출력
            kPrintf( "(%s, %s)\n", vpcInterruptFlagsPO[ pstIOAssignmentEntry->wInterruptFlags & 0x03 ], 
                    vpcInterruptFlagsEL[ ( pstIOAssignmentEntry->wInterruptFlags >> 2 ) & 0x03 ] );
            kPrintf( "Source BUS ID : %d\n", pstIOAssignmentEntry->bSourceBUSID );
            kPrintf( "Source BUS IRQ : %d\n", pstIOAssignmentEntry->bSourceBUSIRQ );
            kPrintf( "Destination I/O APIC ID : %d\n", 
                     pstIOAssignmentEntry->bDestinationIOAPICID );
            kPrintf( "Destination I/O APIC INTIN : %d\n\n", 
                     pstIOAssignmentEntry->bDestinationIOAPICINTIN );

            // I/O 인터럽트 지정 엔트리의 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
            qwBaseEntryAddress += sizeof( IOINTERRUPTASSIGNMENTENTRY );
            break;
            
            // 로컬 인터럽트 지정 엔트리
        case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
            pstLocalAssignmentEntry = ( LOCALINTERRUPTASSIGNMENTENTRY* )
                qwBaseEntryAddress;
            kPrintf( "Entry Type : Local Interrupt Assignment\n" );
            kPrintf( "Interrupt Type : 0x%X ", pstLocalAssignmentEntry->bInterruptType );
            // 인터럽트 타입 출력
            kPrintf( "(%s)\n", vpcInterruptType[ pstLocalAssignmentEntry->bInterruptType ] );
            kPrintf( "I/O Interrupt Flags : 0x%X ", pstLocalAssignmentEntry->wInterruptFlags );
            // 극성과 트리거 모드 출력
            kPrintf( "(%s, %s)\n", vpcInterruptFlagsPO[ pstLocalAssignmentEntry->wInterruptFlags & 0x03 ], 
                    vpcInterruptFlagsEL[ ( pstLocalAssignmentEntry->wInterruptFlags >> 2 ) & 0x03 ] );
            kPrintf( "Source BUS ID : %d\n", pstLocalAssignmentEntry->bSourceBUSID );
            kPrintf( "Source BUS IRQ : %d\n", pstLocalAssignmentEntry->bSourceBUSIRQ );
            kPrintf( "Destination Local APIC ID : %d\n", 
                     pstLocalAssignmentEntry->bDestinationLocalAPICID );
            kPrintf( "Destination Local APIC LINTIN : %d\n\n", 
                     pstLocalAssignmentEntry->bDestinationLocalAPICLINTIN );
            
            // 로컬 인터럽트 지정 엔트리의 크기만큼 어드레스를 증가시켜 다음 엔트리로 이동
            qwBaseEntryAddress += sizeof( LOCALINTERRUPTASSIGNMENTENTRY );
            break;
            
        default :
            kPrintf( "Unknown Entry Type. %d\n", bEntryType );
            break;
        }

        // 3개를 출력하고 나면 키 입력을 대기
        if( ( i != 0 ) && ( ( ( i + 1 ) % 3 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                return ;
            }
            kPrintf( "\n" );            
        }        
    }
}

/**
 *  프로세서 또는 코어의 개수를 반환
 */
int kGetProcessorCount( void )
{
    // MP 설정 테이블이 없을 수도 있으므로, 0으로 설정된 경우 1을 반환
    if( gs_stMPConfigurationManager.iProcessorCount == 0 )
    {
        return 1;
    }
    return gs_stMPConfigurationManager.iProcessorCount;
}

/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////
```

#### 02.Kernel64/Source/InterruptHandler.h

```c
#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 함수
//
////////////////////////////////////////////////////////////////////////////////
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kCommonInterruptHandler( int iVectorNumber );
void kKeyboardHandler( int iVectorNumber );
void kTimerHandler( int iVectorNumber );
void kDeviceNotAvailableHandler( int iVectorNumber );
////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
void kHDDHandler(int iVectorNumber);
////////////////////////////////////////////////////////////////

#endif /*__INTERRUPTHANDLER_H__*/

```

#### 02.Kernel64/Source/InterruptHandler.c

```c
#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
#include "HardDisk.h"
////////////////////////////////////////////////////////////////

/**
 *  공통으로 사용하는 예외 핸들러
 */
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
    char vcBuffer[ 3 ] = { 0, };

    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 1 ] = '0' + iVectorNumber % 10;
    
    kPrintStringXY( 0, 0, "====================================================" );
    kPrintStringXY( 0, 1, "                 Exception Occur~!!!!               " );
    kPrintStringXY( 0, 2, "                    Vector:                         " );
    kPrintStringXY( 27, 2, vcBuffer );
    kPrintStringXY( 0, 3, "====================================================" );

    while( 1 ) ;
}

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
        
    /////////////////////////////////////////////////////////////////////////
    //
    // 대칭 I/O 모드 전환 기능 통합과 빌드
    //
    ////////////////////////////////////////////////////////////////////////
    // PIC 컨트롤러로 EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

    // 로컬 APIC로 EOI 전송
    kSendEOIToLocalAPIC();
    ////////////////////////////////////////////////////////////////////////
}

/**
 *  키보드 인터럽트의 핸들러
 */
void kKeyboardHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    kPrintStringXY( 0, 0, vcBuffer );
    //=========================================================================

    // 키보드 컨트롤러에서 데이터를 읽어서 ASCII로 변환하여 큐에 삽입
    if( kIsOutputBufferFull() == TRUE )
    {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue( bTemp );
    }

    /////////////////////////////////////////////////////////////////////////
    //
    // 대칭 I/O 모드 전환 기능 통합과 빌드
    //
    ////////////////////////////////////////////////////////////////////////
    // PIC 컨트롤러로 EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

    // 로컬 APIC로 EOI 전송
    kSendEOIToLocalAPIC();
    ////////////////////////////////////////////////////////////////////////
}

/**
 *  타이머 인터럽트의 핸들러
 */
void kTimerHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount = ( g_iTimerInterruptCount + 1 ) % 10;
    kPrintStringXY( 70, 0, vcBuffer );
    //=========================================================================
        
    /////////////////////////////////////////////////////////////////////////
    //
    // 대칭 I/O 모드 전환 기능 통합과 빌드
    //
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////

    // 타이머 발생 횟수를 증가
    g_qwTickCount++;

    // 태스크가 사용한 프로세서의 시간을 줄임
    kDecreaseProcessorTime();
    // 프로세서가 사용할 수 있는 시간을 다 썼다면 태스크 전환 수행
    if( kIsProcessorTimeExpired() == TRUE )
    {
        kScheduleInInterrupt();
    }
}

/**
 *  Device Not Available 예외의 핸들러
 */
void kDeviceNotAvailableHandler( int iVectorNumber )
{
    TCB* pstFPUTask, * pstCurrentTask;
    QWORD qwLastFPUTaskID;

    //=========================================================================
    // FPU 예외가 발생했음을 알리려고 메시지를 출력하는 부분
    char vcBuffer[] = "[EXC:  , ]";
    static int g_iFPUInterruptCount = 0;

    // 예외 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount = ( g_iFPUInterruptCount + 1 ) % 10;
    kPrintStringXY( 0, 0, vcBuffer );    
    //=========================================================================
    
    // CR0 컨트롤 레지스터의 TS 비트를 0으로 설정
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인하여, 있다면 FPU의 상태를 태스크에 저장
    qwLastFPUTaskID = kGetLastFPUUsedTaskID();
    pstCurrentTask = kGetRunningTask();
    
    // 이전에 FPU를 사용한 것이 자신이면 아무것도 안 함
    if( qwLastFPUTaskID == pstCurrentTask->stLink.qwID )
    {
        return ;
    }
    // FPU를 사용한 태스크가 있으면 FPU 상태를 저장
    else if( qwLastFPUTaskID != TASK_INVALIDID )
    {
        pstFPUTask = kGetTCBInTCBPool( GETTCBOFFSET( qwLastFPUTaskID ) );
        if( ( pstFPUTask != NULL ) && ( pstFPUTask->stLink.qwID == qwLastFPUTaskID ) )
        {
            kSaveFPUContext( pstFPUTask->vqwFPUContext );
        }
    }
    
    // 현재 태스크가 FPU를 사용한 적이 있는 지 확인하여 FPU를 사용한 적이 없다면 
    // 초기화하고, 사용한적이 있다면 저장된 FPU 콘텍스트를 복원
    if( pstCurrentTask->bFPUUsed == FALSE )
    {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    }
    else
    {
        kLoadFPUContext( pstCurrentTask->vqwFPUContext );
    }
    
    // FPU를 사용한 태스크 ID를 현재 태스크로 변경
    kSetLastFPUUsedTaskID( pstCurrentTask->stLink.qwID );
}

////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
/**
 *  하드 디스크에서 발생하는 인터럽트의 핸들러
 */
void kHDDHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iHDDInterruptCount = 0;
    BYTE bTemp;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount = ( g_iHDDInterruptCount + 1 ) % 10;
    // 왼쪽 위에 있는 메시지와 겹치지 않도록 (10, 0)에 출력
    kPrintStringXY( 10, 0, vcBuffer );
    //=========================================================================

    // 첫 번째 PATA 포트의 인터럽트 벡터(IRQ 14) 처리
    if( iVectorNumber - PIC_IRQSTARTVECTOR == 14 )
    {
        // 첫 번째 PATA 포트의 인터럽트 발생 여부를 TRUE로 설정
        kSetHDDInterruptFlag( TRUE, TRUE );
    }
    // 두 번째 PATA 포트의 인터럽트 벡터(IRQ 15) 처리
    else
    {
        // 두 번째 PATA 포트의 인터럽트 발생 여부를 TRUE로 설정
        kSetHDDInterruptFlag( FALSE, TRUE );
    }
        
    /////////////////////////////////////////////////////////////////////////
    //
    // 대칭 I/O 모드 전환 기능 통합과 빌드
    //
    ////////////////////////////////////////////////////////////////////////
    // PIC 컨트롤러로 EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );

    // 로컬 APIC로 EOI 전송
    kSendEOIToLocalAPIC();
    ////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////
```

<hr>

## 콘솔 셸 파일 수정

#### 02.Kernel64/Source/ConsoleShell.h

```c
#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT  300
#define CONSOLESHELL_PROMPTMESSAGE          "CYNOS64! >"

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void ( * CommandFunction ) ( const char* pcParameter );


////////////////////////////////////////////////////////////////////////////////
//
// 구조체
//
////////////////////////////////////////////////////////////////////////////////
// 1바이트로 정렬
#pragma pack( push, 1 )

// 셸의 커맨드를 저장하는 자료구조
typedef struct kShellCommandEntryStruct
{
    // 커맨드 문자열
    char* pcCommand;
    // 커맨드의 도움말
    char* pcHelp;
    // 커맨드를 수행하는 함수의 포인터
    CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

// 파라미터를 처리하기위해 정보를 저장하는 자료구조
typedef struct kParameterListStruct
{
    // 파라미터 버퍼의 어드레스
    const char* pcBuffer;
    // 파라미터의 길이
    int iLength;
    // 현재 처리할 파라미터가 시작하는 위치
    int iCurrentPosition;
} PARAMETERLIST;

#pragma pack( pop )

////////////////////////////////////////////////////////////////////////////////
//
// 함수
//
////////////////////////////////////////////////////////////////////////////////
// 실제 셸 코드
void kStartConsoleShell( void );
void kExecuteCommand( const char* pcCommandBuffer );
void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter );
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter );

// 커맨드를 처리하는 함수
static void kHelp( const char* pcParameterBuffer );
static void kCls( const char* pcParameterBuffer );
static void kShowTotalRAMSize( const char* pcParameterBuffer );
static void kStringToDecimalHexTest( const char* pcParameterBuffer );
static void kShutdown( const char* pcParameterBuffer );

static void kConsoleBackGround(const char* pcParameterBuffer);
static void kConsoleForeGround(const char* pcParameterBuffer);

////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
static void kSetTimer(const char* pcParameterBuffer);
static void kWaitUsingPIT(const char* pcParameterBuffer);
static void kReadTimeStampCounter(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 태스크 개념을 추가해 멀티태스킹을 구현하자
//
////////////////////////////////////////////////////////////////////////////////
static void kCreateTestTask(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
static void kChangeTaskPriority( const char* pcParameterBuffer );
static void kShowTaskList( const char* pcParameterBuffer );
static void kKillTask( const char* pcParameterBuffer );
static void kCPULoad( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 멀티 스레딩 기능을 추가하자.
//
////////////////////////////////////////////////////////////////////////////////
static void kTestMutex( const char* pcParameterBuffer );
static void kCreateThreadTask( void );
static void kTestThread( const char* pcParameterBuffer );
static void kShowMatrix( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 실수 연산 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////////////
static void kTestPIE(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 동적 메모리 할당
//
//////////////////////////////////////////////////////////////////////////////// 
static void kShowDyanmicMemoryInformation( const char* pcParameterBuffer );
static void kTestSequentialAllocation(const char* pcParameterBuffer);
static void kTestRandomAllocation(const char* pcParameterBuffer);
static void kRandomAllocationTask(void);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
static void kShowHDDInformation( const char* pcParameterBuffer );
static void kReadSector( const char* pcParameterBuffer );
static void kWriteSector( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 간단한 파일 시스템 구현
//
////////////////////////////////////////////////////////////////
static void kMountHDD(const char* pcParameterBuffer);
static void kFormatHDD(const char* pcParameterBuffer);
static void kShowFileSystemInformation(const char* pcParameterBuffer);
static void kCreateFileInRootDirectory(const char* pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer);
static void kShowRootDirectory(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// C 표준 입출력 함수 추가
//
////////////////////////////////////////////////////////////////////////////////
static void kWriteDataToFile( const char* pcParameterBuffer );
static void kReadDataFromFile( const char* pcParameterBuffer );
static void kTestFileIO( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
// 파일 시스템 캐시와 램 디스크 추가
//
///////////////////////////////////////////////////////////////////
static void kFlushCache( const char* pcParameterBuffer );
static void kTestPerformance( const char* pcParameterBuffer );
///////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 시리얼 포트 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
static void kDownloadFile(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// MP 설정 테이블 분석
//
////////////////////////////////////////////////////////////////////////////////
static void kShowMPConfigurationTable(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 잠자는 코어 깨우기
//
////////////////////////////////////////////////////////////////
static void kStartApplicationProcessor( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
static void kStartSymmetricIOMode( const char* pcParameterBuffer );
static void kShowIRQINTINMappingTable( const char* pcParameterBuffer );
////////////////////////////////////////////////////////////////////////
#endif /*__CONSOLESHELL_H__*/

```

#### 02.Kernel64/Source/ConsoleShell.c

```c
#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
#include "Task.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// 멀티 스레딩 기능을 추가하자.
//
////////////////////////////////////////////////////////////////////////////////
#include "Synchronization.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// 동적 메모리 할당
//
//////////////////////////////////////////////////////////////////////////////// 
#include "DynamicMemory.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
#include "HardDisk.h"
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
// 간단한 파일 시스템 구현
//
////////////////////////////////////////////////////////////////
#include "FileSystem.h"
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// 시리얼 포트 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
#include "SerialPort.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MP 설정 테이블 분석
//
////////////////////////////////////////////////////////////////////////////////
#include "MPConfigurationTable.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
// 잠자는 코어 깨우기
//
////////////////////////////////////////////////////////////////
#include "LocalAPIC.h"
#include "MultiProcessor.h"
////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
#include "IOAPIC.h"
////////////////////////////////////////////////////////////////////////

// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
        { "help", "Show Help", kHelp },
        { "cls", "Clear Screen", kCls },
        { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
        { "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
        { "shutdown", "Shutdown And Reboot OS", kShutdown },
		{ "changeBackColor", "Change Console Background Color", kConsoleBackGround},
		{ "changeForeColor", "Change Console Foreground Color", kConsoleForeGround},
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 타이머 디바이스 드라이버 추가
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", 
                kSetTimer },
        { "wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT },
        { "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
        { "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
        { "date", "Show Date And Time", kShowDateAndTime },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 라운드 로빈 스케줄러를 추가하자
        //
        ////////////////////////////////////////////////////////////////////////////////
        {"createtask","Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask},
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",
                kChangeTaskPriority },
        { "tasklist", "Show Task List", kShowTaskList },
        { "killtask", "End Task, ex)killtask 1(ID)", kKillTask },
        { "cpuload", "Show Processor Load", kCPULoad },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 멀티 스레딩 기능을 추가하자.
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "testmutex", "Test Mutex Function", kTestMutex },
        { "testthread", "Test Thread And Process Function", kTestThread },
        { "showmatrix", "Show Matrix Screen", kShowMatrix },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 실수 연산 기능 통합과 빌드
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "testpie", "Test PIE Calculation", kTestPIE },      
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 동적 메모리 할당
        //
        //////////////////////////////////////////////////////////////////////////////// 
        { "dynamicmeminfo", "Show Dyanmic Memory Information", kShowDyanmicMemoryInformation },
        { "testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation },
        { "testranalloc", "Test Random Allocation & Free", kTestRandomAllocation },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////
        //
        // 하드디스크 디바이스 드라이버 추가 
        //
        ////////////////////////////////////////////////////////////////
        { "hddinfo", "Show HDD Information", kShowHDDInformation },
        { "readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", 
                kReadSector },
        { "writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", 
                kWriteSector },
        ////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////
        //
        // 간단한 파일 시스템 구현
        //
        ////////////////////////////////////////////////////////////////
        { "mounthdd", "Mount HDD", kMountHDD },
        { "formathdd", "Format HDD", kFormatHDD },
        { "filesysteminfo", "Show File System Information", kShowFileSystemInformation },
        { "createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory },
        { "deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory },
        { "dir", "Show Directory", kShowRootDirectory },
        ////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // C 표준 입출력 함수 추가
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile },
        { "readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile },
        { "testfileio", "Test File I/O Function", kTestFileIO },
        ////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////
        //
        // 파일 시스템 캐시와 램 디스크 추가
        //
        ///////////////////////////////////////////////////////////////////
        { "testperformance", "Test File Read/WritePerformance", kTestPerformance },
        { "flush", "Flush File System Cache", kFlushCache },
        ///////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 시리얼 포트 디바이스 드라이버 추가
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "download", "Download Data From Serial, ex) download a.txt", kDownloadFile },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////
        //
        // MP 설정 테이블 분석
        //
        ////////////////////////////////////////////////////////////////////////////////
        { "showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable },
        ////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////
        //
        // 잠자는 코어 깨우기
        //
        ////////////////////////////////////////////////////////////////
        { "startap", "Start Application Processor", kStartApplicationProcessor },
        ////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////
        //
        // 대칭 I/O 모드 전환 기능 통합과 빌드
        //
        ////////////////////////////////////////////////////////////////////////
        { "startsymmetricio", "Start Symmetric I/O Mode", kStartSymmetricIOMode },
        { "showirqintinmap", "Show IRQ->INITIN Mapping Table", kShowIRQINTINMappingTable },
        ////////////////////////////////////////////////////////////////////////
};                                     

//==============================================================================
//  실제 셸을 구성하는 코드
//==============================================================================
/**
 *  셸의 메인 루프
 */
void kStartConsoleShell( void )
{
    char vcCommandBuffer[ CONSOLESHELL_MAXCOMMANDBUFFERCOUNT ];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;
    
    // 프롬프트 출력
    kPrintf( CONSOLESHELL_PROMPTMESSAGE );
    
    while( 1 )
    {
        // 키가 수신될 때까지 대기
        bKey = kGetCh();
        // Backspace 키 처리
        if( bKey == KEY_BACKSPACE )
        {
            if( iCommandBufferIndex > 0 )
            {
                // 현재 커서 위치를 얻어서 한 문자 앞으로 이동한 다음 공백을 출력하고 
                // 커맨드 버퍼에서 마지막 문자 삭제
                kGetCursor( &iCursorX, &iCursorY );
                kPrintStringXY( iCursorX - 1, iCursorY, " " );
                kSetCursor( iCursorX - 1, iCursorY );
                iCommandBufferIndex--;
            }
        }
        // 엔터 키 처리
        else if( bKey == KEY_ENTER )
        {
            kPrintf( "\n" );
            
            if( iCommandBufferIndex > 0 )
            {
                // 커맨드 버퍼에 있는 명령을 실행
                vcCommandBuffer[ iCommandBufferIndex ] = '\0';
                kExecuteCommand( vcCommandBuffer );
            }
            
            // 프롬프트 출력 및 커맨드 버퍼 초기화
            kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );            
            kMemSet( vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT );
            iCommandBufferIndex = 0;
        }
        // 시프트 키, CAPS Lock, NUM Lock, Scroll Lock은 무시
        else if( ( bKey == KEY_LSHIFT ) || ( bKey == KEY_RSHIFT ) ||
                 ( bKey == KEY_CAPSLOCK ) || ( bKey == KEY_NUMLOCK ) ||
                 ( bKey == KEY_SCROLLLOCK ) )
        {
            ;
        }
        else
        {
            // TAB은 공백으로 전환
            if( bKey == KEY_TAB )
            {
                bKey = ' ';
            }
            
            // 버퍼에 공간이 남아있을 때만 가능
            if( iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT )
            {
                if(bKey > 31 && bKey <127)
                {
                    vcCommandBuffer[ iCommandBufferIndex++ ] = bKey;
                    kPrintf( "%c", bKey );
                }
            }
        }
    }
}

/*
 *  커맨드 버퍼에 있는 커맨드를 비교하여 해당 커맨드를 처리하는 함수를 수행
 */
void kExecuteCommand( const char* pcCommandBuffer )
{
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;
    
    // 공백으로 구분된 커맨드를 추출
    iCommandBufferLength = kStrLen( pcCommandBuffer );
    for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
    {
        if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
        {
            break;
        }
    }
    
    // 커맨드 테이블을 검사해서 동일한 이름의 커맨드가 있는지 확인
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );
    for( i = 0 ; i < iCount ; i++ )
    {
        iCommandLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
        // 커맨드의 길이와 내용이 완전히 일치하는지 검사
        if( ( iCommandLength == iSpaceIndex ) &&
            ( kMemCmp( gs_vstCommandTable[ i ].pcCommand, pcCommandBuffer,
                       iSpaceIndex ) == 0 ) )
        {
            gs_vstCommandTable[ i ].pfFunction( pcCommandBuffer + iSpaceIndex + 1 );
            break;
        }
    }

    // 리스트에서 찾을 수 없다면 에러 출력
    if( i >= iCount )
    {
        kPrintf( "'%s' is not found.\n", pcCommandBuffer );
    }
}

/**
 *  파라미터 자료구조를 초기화
 */
void kInitializeParameter( PARAMETERLIST* pstList, const char* pcParameter )
{
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen( pcParameter );
    pstList->iCurrentPosition = 0;
}

/**
 *  공백으로 구분된 파라미터의 내용과 길이를 반환
 */
int kGetNextParameter( PARAMETERLIST* pstList, char* pcParameter )
{
    int i;
    int iLength;

    // 더 이상 파라미터가 없으면 나감
    if( pstList->iLength <= pstList->iCurrentPosition )
    {
        return 0;
    }
    
    // 버퍼의 길이만큼 이동하면서 공백을 검색
    for( i = pstList->iCurrentPosition ; i < pstList->iLength ; i++ )
    {
        if( pstList->pcBuffer[ i ] == ' ' )
        {
            break;
        }
    }
    
    // 파라미터를 복사하고 길이를 반환
    kMemCpy( pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i );
    iLength = i - pstList->iCurrentPosition;
    pcParameter[ iLength ] = '\0';

    // 파라미터의 위치 업데이트
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}
    
//==============================================================================
//  커맨드를 처리하는 코드
//==============================================================================
/**
 *  셸 도움말을 출력
 */
static void kHelp( const char* pcCommandBuffer )
{
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;
    
    
    kPrintf( "=========================================================\n" );
    kPrintf( "                    MINT64 Shell Help                    \n" );
    kPrintf( "=========================================================\n" );
    
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );

    // 가장 긴 커맨드의 길이를 계산
    for( i = 0 ; i < iCount ; i++ )
    {
        iLength = kStrLen( gs_vstCommandTable[ i ].pcCommand );
        if( iLength > iMaxCommandLength )
        {
            iMaxCommandLength = iLength;
        }
    }
    
    // 도움말 출력
    for( i = 0 ; i < iCount ; i++ )
    {
        kPrintf( "%s", gs_vstCommandTable[ i ].pcCommand );
        kGetCursor( &iCursorX, &iCursorY );
        kSetCursor( iMaxCommandLength, iCursorY );
        kPrintf( "  - %s\n", gs_vstCommandTable[ i ].pcHelp );

////////////////////////////////////////////////////////////////////////////////
//
// 동적 메모리 할당
//
//////////////////////////////////////////////////////////////////////////////// 
// 목록이 많을 경우 나눠서 보여줌
        if( ( i != 0 ) && ( ( i % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
            kPrintf( "\n" );
        }
////////////////////////////////////////////////////////////////////////////////
    }
}

/**
 *  화면을 지움 
 */
static void kCls( const char* pcParameterBuffer )
{
    // 맨 윗줄은 디버깅 용으로 사용하므로 화면을 지운 후, 라인 1로 커서 이동
    kClearScreen();
    kSetCursor( 0, 1 );
}

/**
 *  총 메모리 크기를 출력
 */
static void kShowTotalRAMSize( const char* pcParameterBuffer )
{
    kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize() );
}

/**
 *  문자열로 된 숫자를 숫자로 변환하여 화면에 출력
 */
static void kStringToDecimalHexTest( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;
    
    // 파라미터 초기화
    kInitializeParameter( &stList, pcParameterBuffer );
    
    while( 1 )
    {
        // 다음 파라미터를 구함, 파라미터의 길이가 0이면 파라미터가 없는 것이므로
        // 종료
        iLength = kGetNextParameter( &stList, vcParameter );
        if( iLength == 0 )
        {
            break;
        }

        // 파라미터에 대한 정보를 출력하고 16진수인지 10진수인지 판단하여 변환한 후
        // 결과를 printf로 출력
        kPrintf( "Param %d = '%s', Length = %d, ", iCount + 1, 
                 vcParameter, iLength );

        // 0x로 시작하면 16진수, 그외는 10진수로 판단
        if( kMemCmp( vcParameter, "0x", 2 ) == 0 )
        {
            lValue = kAToI( vcParameter + 2, 16 );
            kPrintf( "HEX Value = %q\n", lValue );
        }
        else
        {
            lValue = kAToI( vcParameter, 10 );
            kPrintf( "Decimal Value = %d\n", lValue );
        }
        
        iCount++;
    }
}

/**
 *  PC를 재시작(Reboot)
 */
static void kShutdown( const char* pcParamegerBuffer )
{
    kPrintf( "System Shutdown Start...\n" );
    
///////////////////////////////////////////////////////////////////
//
// 파일 시스템 캐시와 램 디스크 추가
//
///////////////////////////////////////////////////////////////////
    // 파일 시스템 캐시에 들어있는 내용을 하드 디스크로 옮김
    kPrintf( "Cache Flush... ");
    if( kFlushFileSystemCache() == TRUE )
    {
        kPrintf( "Pass\n" );
    }
    else
    {
        kPrintf( "Fail\n" );
    }
///////////////////////////////////////////////////////////////////

    // 키보드 컨트롤러를 통해 PC를 재시작
    kPrintf( "Press Any Key To Reboot PC..." );
    kGetCh();
    kReboot();
}

////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  PIT 컨트롤러의 카운터 0 설정
 */
static void kSetTimer( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    // 파라미터 초기화
    kInitializeParameter( &stList, pcParameterBuffer );
    
    // milisecond 추출
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }
    lValue = kAToI( vcParameter, 10 );

    // Periodic 추출
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)settimer 10(ms) 1(periodic)\n" );
        return ;
    }    
    bPeriodic = kAToI( vcParameter, 10 );
    
    kInitializePIT( MSTOCOUNT( lValue ), bPeriodic );
    kPrintf( "Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic );
}

/**
 *  PIT 컨트롤러를 직접 사용하여 ms 동안 대기  
 */
static void kWaitUsingPIT( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;
    
    // 파라미터 초기화
    kInitializeParameter( &stList, pcParameterBuffer );
    if( kGetNextParameter( &stList, vcParameter ) == 0 )
    {
        kPrintf( "ex)wait 100(ms)\n" );
        return ;
    }
    
    lMillisecond = kAToI( pcParameterBuffer, 10 );
    kPrintf( "%d ms Sleep Start...\n", lMillisecond );
    
    // 인터럽트를 비활성화하고 PIT 컨트롤러를 통해 직접 시간을 측정
    kDisableInterrupt();
    for( i = 0 ; i < lMillisecond / 30 ; i++ )
    {
        kWaitUsingDirectPIT( MSTOCOUNT( 30 ) );
    }
    kWaitUsingDirectPIT( MSTOCOUNT( lMillisecond % 30 ) );   
    kEnableInterrupt();
    kPrintf( "%d ms Sleep Complete\n", lMillisecond );
    
    // 타이머 복원
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );
}

/**
 *  타임 스탬프 카운터를 읽음  
 */
static void kReadTimeStampCounter( const char* pcParameterBuffer )
{
    QWORD qwTSC;
    
    qwTSC = kReadTSC();
    kPrintf( "Time Stamp Counter = %q\n", qwTSC );
}

/**
 *  프로세서의 속도를 측정
 */
static void kMeasureProcessorSpeed( const char* pcParameterBuffer )
{
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;
        
    kPrintf( "Now Measuring." );
    
    // 10초 동안 변화한 타임 스탬프 카운터를 이용하여 프로세서의 속도를 간접적으로 측정
    kDisableInterrupt();    
    for( i = 0 ; i < 200 ; i++ )
    {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT( MSTOCOUNT( 50 ) );
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf( "." );
    }
    // 타이머 복원
    kInitializePIT( MSTOCOUNT( 1 ), TRUE );    
    kEnableInterrupt();
    
    kPrintf( "\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000 );
}

/**
 *  RTC 컨트롤러에 저장된 일자 및 시간 정보를 표시
 */
static void kShowDateAndTime( const char* pcParameterBuffer )
{
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime( &bHour, &bMinute, &bSecond );
    kReadRTCDate( &wYear, &bMonth, &bDayOfMonth, &bDayOfWeek );
    
    kPrintf( "Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth,
             kConvertDayOfWeekToString( bDayOfWeek ) );
    kPrintf( "Time: %d:%d:%d\n", bHour, bMinute, bSecond );
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가 ( kHelp까지 static 함수로 바꾸기)
//
////////////////////////////////////////////////////////////////////////////////
// 태스크 1
//  화면 테두리를 돌면서 문자를 출력
static void kTestTask1(void)
////////////////////////////////////////////////////////////////////////////////
{
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
    int j;
////////////////////////////////////////////////////////////////////////////////
    CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF)%10;

    // 화면 네 귀퉁이를 돌면서 문자 출력
////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
    for(j = 0; j< 20000; j++)
    {
        switch(i)
        {
            case 0:
                iX++;
                if(iX >= (CONSOLE_WIDTH - iMargin))
                {
                    i = 1;
                }
                break;
            case 1:
                iY++;
                if(iY >= (CONSOLE_HEIGHT - iMargin))
                {
                    i = 2;
                }
                break;
            case 2:
                iX--;
                if(iX < iMargin)
                {
                    i = 3;
                }
                break;
            case 3:
                iY--;
                if(iY < iMargin)
                {
                    i = 0;
                }
                break;
        }

        // 문자 및 색깔 지정
        pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
        pstScreen[iY*CONSOLE_WIDTH + iX].bAttribute = 0x70 | bData & 0x0F;
        bData++;

        // 다른 태스크로 전환
        //kSchedule();
    }
    kExitTask();
////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
//태스크 2
//     자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
static void kTestTask2( void )
////////////////////////////////////////////////////////////////////////////////
{
    int i = 0, iOffset;
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };
    
    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - 
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    while( 1 )
    {
        // 회전하는 바람개비를 표시
        pstScreen[ iOffset ].bCharactor = vcData[ i % 4 ];
        // 색깔 지정
        pstScreen[ iOffset ].bAttribute = 0x70 |( iOffset % 15 ) + 1;
        i++;
        
////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
        // 다른 태스크로 전환
        //kSchedule();
////////////////////////////////////////////////////////////////////////////////
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
// 태스크를 생성해서 멀티 태스킹 수행
static void kCreateTestTask( const char* pcParameterBuffer )
////////////////////////////////////////////////////////////////////////////////
{
    PARAMETERLIST stList;
    char vcType[ 30 ];
    char vcCount[ 30 ];
    int i;
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcType );
    kGetNextParameter( &stList, vcCount );

    switch( kAToI( vcType, 10 ) )
    {
    // 타입 1 태스크 생성
    case 1:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            ////////////////////////////////////////////////////////////////////////////////
            //
            // 멀티 스레딩 기능을 추가하자.
            //
            ////////////////////////////////////////////////////////////////////////////////
            if( kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask1 ) == NULL )
            {
                break;
            }
            ////////////////////////////////////////////////////////////////////////////////
        }
        
        kPrintf( "Task1 %d Created\n", i );
        break;
        
    // 타입 2 태스크 생성
    case 2:
    default:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        {    
            ////////////////////////////////////////////////////////////////////////////////
            //
            // 멀티 스레딩 기능을 추가하자.
            //
            ////////////////////////////////////////////////////////////////////////////////
            if( kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL )
            {
                break;
            }
            ////////////////////////////////////////////////////////////////////////////////
        }
        kPrintf( "Task2 %d Created\n", i );
        break;
    }    
}   

/**
 *  태스크의 우선 순위를 변경
 */
static void kChangeTaskPriority( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    char vcPriority[ 30 ];
    QWORD qwID;
    BYTE bPriority;
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    kGetNextParameter( &stList, vcPriority );
    
    // 태스크의 우선 순위를 변경
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    bPriority = kAToI( vcPriority, 10 );
    
    kPrintf( "Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority );
    if( kChangePriority( qwID, bPriority ) == TRUE )
    {
        kPrintf( "Success\n" );
    }
    else
    {
        kPrintf( "Fail\n" );
    }
}

/**
 *  현재 생성된 모든 태스크의 정보를 출력
 */
static void kShowTaskList( const char* pcParameterBuffer )
{
    int i;
    TCB* pstTCB;
    int iCount = 0;
    
    kPrintf( "=========== Task Total Count [%d] ===========\n", kGetTaskCount() );
    for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
    {
        // TCB를 구해서 TCB가 사용 중이면 ID를 출력
        pstTCB = kGetTCBInTCBPool( i );
        if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
        {
            // 태스크가 10개 출력될 때마다, 계속 태스크 정보를 표시할지 여부를 확인
            if( ( iCount != 0 ) && ( ( iCount % 10 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
            }
            
            ////////////////////////////////////////////////////////////////////////////////
            //
            // 멀티 스레딩 기능을 추가하자.
            //
            ////////////////////////////////////////////////////////////////////////////////
            kPrintf( "[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
                     pstTCB->stLink.qwID, GETPRIORITY( pstTCB->qwFlags ), 
                     pstTCB->qwFlags, kGetListCount( &( pstTCB->stChildThreadList ) ) );
            kPrintf( "    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
                    pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize );
            ////////////////////////////////////////////////////////////////////////////////
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 멀티 스레딩 기능을 추가하자.
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  태스크를 종료
 */
static void kKillTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    QWORD qwID;
    TCB* pstTCB;
    int i;
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    
    // 태스크를 종료
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    // 특정 ID만 종료하는 경우
    if( qwID != 0xFFFFFFFF )
    {
        pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );
        qwID = pstTCB->stLink.qwID;

        // 시스템 테스트는 제외
        if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
        {
            kPrintf( "Kill Task ID [0x%q] ", qwID );
            if( kEndTask( qwID ) == TRUE )
            {
                kPrintf( "Success\n" );
            }
            else
            {
                kPrintf( "Fail\n" );
            }
        }
        else
        {
            kPrintf( "Task does not exist or task is system task\n" );
        }
    }
    // 콘솔 셸과 유휴 태스크를 제외하고 모든 태스크 종료
    else
    {
        for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
        {
            pstTCB = kGetTCBInTCBPool( i );
            qwID = pstTCB->stLink.qwID;

            // 시스템 테스트는 삭제 목록에서 제외
            if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
            {
                kPrintf( "Kill Task ID [0x%q] ", qwID );
                if( kEndTask( qwID ) == TRUE )
                {
                    kPrintf( "Success\n" );
                }
                else
                {
                    kPrintf( "Fail\n" );
                }
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////

/**
 *  프로세서의 사용률을 표시
 */
static void kCPULoad( const char* pcParameterBuffer )
{
    kPrintf( "Processor Load : %d%%\n", kGetProcessorLoad() );
}

////////////////////////////////////////////////////////////////////////////////
//
// 멀티 스레딩 기능을 추가하자.
//
////////////////////////////////////////////////////////////////////////////////
// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

/**
 *  뮤텍스를 테스트하는 태스크
 */
static void kPrintNumberTask( void )
{
    int i;
    int j;
    QWORD qwTickCount;

    // 50ms 정도 대기하여 콘솔 셸이 출력하는 메시지와 겹치지 않도록 함
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 50 )
    {
        kSchedule();
    }    
    
    // 루프를 돌면서 숫자를 출력
    for( i = 0 ; i < 5 ; i++ )
    {
        kLock( &( gs_stMutex ) );
        kPrintf( "Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
                gs_qwAdder );
        
        gs_qwAdder += 1;
        kUnlock( & ( gs_stMutex ) );
    
        // 프로세서 소모를 늘리려고 추가한 코드
        for( j = 0 ; j < 30000 ; j++ ) ;
    }
    
    // 모든 태스크가 종료할 때까지 1초(100ms) 정도 대기
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 1000 )
    {
        kSchedule();
    }    
    
    // 태스크 종료
    //kExitTask();
}

/**
 *  뮤텍스를 테스트하는 태스크 생성
 */
static void kTestMutex( const char* pcParameterBuffer )
{
    int i;
    
    gs_qwAdder = 1;
    
    // 뮤텍스 초기화
    kInitializeMutex( &gs_stMutex );
    
    for( i = 0 ; i < 3 ; i++ )
    {
        // 뮤텍스를 테스트하는 태스크를 3개 생성
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kPrintNumberTask );
    }    
    kPrintf( "Wait Util %d Task End...\n", i );
    kGetCh();
}

/**
 *  태스크 2를 자신의 스레드로 생성하는 태스크
 */
static void kCreateThreadTask( void )
{
    int i;
    
    for( i = 0 ; i < 3 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 );
    }
    
    while( 1 )
    {
        kSleep( 1 );
    }
}

/**
 *  스레드를 테스트하는 태스크 생성
 */
static void kTestThread( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, ( void * )0xEEEEEEEE, 0x1000, 
                              ( QWORD ) kCreateThreadTask );
    if( pstProcess != NULL )
    {
        kPrintf( "Process [0x%Q] Create Success\n", pstProcess->stLink.qwID ); 
    }
    else
    {
        kPrintf( "Process Create Fail\n" );
    }
}

// 난수를 발생시키기 위한 변수
static volatile QWORD gs_qwRandomValue = 0;

/**
 *  임의의 난수를 반환
 */
QWORD kRandom( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

/**
 *  철자를 흘러내리게 하는 스레드
 */
static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;
    
    while( 1 )
    {
        // 잠시 대기함
        kSleep( kRandom() % 20 );
        
        if( ( kRandom() % 20 ) < 16 )
        {
            vcText[ 0 ] = ' ';
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                kPrintStringXY( iX, i , vcText );
                kSleep( 50 );
            }
        }        
        else
        {
            for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
            {
                vcText[ 0 ] = i + kRandom();
                kPrintStringXY( iX, i, vcText );
                kSleep( 50 );
            }
        }
    }
}

/**
 *  스레드를 생성하여 매트릭스 화면처럼 보여주는 프로세스
 */
static void kMatrixProcess( void )
{
    int i;
    
    for( i = 0 ; i < 300 ; i++ )
    {
        if( kCreateTask( TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, 
                         ( QWORD ) kDropCharactorThread ) == NULL )
        {
            break;
        }
        
        kSleep( kRandom() % 5 + 5 );
    }
    
    kPrintf( "%d Thread is created\n", i );

    // 키가 입력되면 프로세스 종료
    kGetCh();
}

/**
 *  매트릭스 화면을 보여줌
 */
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, ( void* ) 0xE00000, 0xE00000, 
                              ( QWORD ) kMatrixProcess );
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

        // 태스크가 종료 될 때까지 대기
        while( ( pstProcess->stLink.qwID >> 32 ) != 0 )
        {
            kSleep( 100 );
        }
    }
    else
    {
        kPrintf( "Matrix Process Create Fail\n" );
    }
} 
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 실수 연산 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  FPU를 테스트하는 태스크
 */
static void kFPUTestTask( void )
{
    double dValue1;
    double dValue2;
    TCB* pstRunningTask;
    QWORD qwCount = 0;
    QWORD qwRandomValue;
    int i;
    int iOffset;
    char vcData[ 4 ] = { '-', '\\', '|', '/' };
    CHARACTER* pstScreen = ( CHARACTER* ) CONSOLE_VIDEOMEMORYADDRESS;

    pstRunningTask = kGetRunningTask();

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    iOffset = ( pstRunningTask->stLink.qwID & 0xFFFFFFFF ) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - 
        ( iOffset % ( CONSOLE_WIDTH * CONSOLE_HEIGHT ) );

    // 루프를 무한히 반복하면서 동일한 계산을 수행
    while( 1 )
    {
        dValue1 = 1;
        dValue2 = 1;
        
        // 테스트를 위해 동일한 계산을 2번 반복해서 실행
        for( i = 0 ; i < 10 ; i++ )
        {
            qwRandomValue = kRandom();
            dValue1 *= ( double ) qwRandomValue;
            dValue2 *= ( double ) qwRandomValue;

            kSleep( 1 );
            
            qwRandomValue = kRandom();
            dValue1 /= ( double ) qwRandomValue;
            dValue2 /= ( double ) qwRandomValue;
        }
        
        if( dValue1 != dValue2 )
        {
            kPrintf( "Value Is Not Same~!!! [%f] != [%f]\n", dValue1, dValue2 );
            break;
        }
        qwCount++;

        // 회전하는 바람개비를 표시
        pstScreen[ iOffset ].bCharactor = vcData[ qwCount % 4 ];

        // 색깔 지정
        pstScreen[ iOffset ].bAttribute = 0x70 |( iOffset % 15 ) + 1;
    }
}

/**
 *  원주율(PIE)를 계산
 */
static void kTestPIE( const char* pcParameterBuffer )
{
    double dResult;
    int i;
    
    kPrintf( "PIE Cacluation Test\n" );
    kPrintf( "Result: 355 / 113 = " );
    dResult = ( double ) 355 / 113;
    kPrintf( "%d.%d%d\n", ( QWORD ) dResult, ( ( QWORD ) ( dResult * 10 ) % 10 ),
             ( ( QWORD ) ( dResult * 100 ) % 10 ) );
    
    // 실수를 계산하는 태스크를 생성
    for( i = 0 ; i < 100 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kFPUTestTask );
    }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 동적 메모리 할당
//
//////////////////////////////////////////////////////////////////////////////// 
/**
 *  동적 메모리 정보를 표시
 */
static void kShowDyanmicMemoryInformation( const char* pcParameterBuffer )
{
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
    
    kGetDynamicMemoryInformation( &qwStartAddress, &qwTotalSize, &qwMetaSize, 
            &qwUsedSize );

    kPrintf( "============ Dynamic Memory Information ============\n" );
    kPrintf( "Start Address: [0x%Q]\n", qwStartAddress );
    kPrintf( "Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize, 
            qwTotalSize / 1024 / 1024 );
    kPrintf( "Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize, 
            qwMetaSize / 1024 );
    kPrintf( "Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024 );
}

/**
 *  모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
 */
static void kTestSequentialAllocation( const char* pcParameterBuffer )
{
    DYNAMICMEMORY* pstMemory;
    long i, j, k;
    QWORD* pqwBuffer;
    
    kPrintf( "============ Dynamic Memory Test ============\n" );
    pstMemory = kGetDynamicMemoryManager();
    
    for( i = 0 ; i < pstMemory->iMaxLevelCount ; i++ )
    {
        kPrintf( "Block List [%d] Test Start\n", i );
        kPrintf( "Allocation And Compare: ");
        
        // 모든 블록을 할당 받아서 값을 채운 후 검사
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            pqwBuffer = kAllocateMemory( DYNAMICMEMORY_MIN_SIZE << i );
            if( pqwBuffer == NULL )
            {
                kPrintf( "\nAllocation Fail\n" );
                return ;
            }

            // 값을 채운 후 다시 검사
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                pqwBuffer[ k ] = k;
            }
            
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                if( pqwBuffer[ k ] != k )
                {
                    kPrintf( "\nCompare Fail\n" );
                    return ;
                }
            }
            // 진행 과정을 . 으로 표시
            kPrintf( "." );
        }
        
        kPrintf( "\nFree: ");
        // 할당 받은 블록을 모두 반환
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            if( kFreeMemory( ( void * ) ( pstMemory->qwStartAddress + 
                         ( DYNAMICMEMORY_MIN_SIZE << i ) * j ) ) == FALSE )
            {
                kPrintf( "\nFree Fail\n" );
                return ;
            }
            // 진행 과정을 . 으로 표시
            kPrintf( "." );
        }
        kPrintf( "\n" );
    }
    kPrintf( "Test Complete~!!!\n" );
}

/**
 *  임의로 메모리를 할당하고 해제하는 것을 반복하는 태스크
 */
static void kRandomAllocationTask( void )
{
    TCB* pstTask;
    QWORD qwMemorySize;
    char vcBuffer[ 200 ];
    BYTE* pbAllocationBuffer;
    int i, j;
    int iY;
    
    pstTask = kGetRunningTask();
    iY = ( pstTask->stLink.qwID ) % 15 + 9;

    for( j = 0 ; j < 10 ; j++ )
    {
        // 1KB ~ 32M까지 할당하도록 함
        do
        {
            qwMemorySize = ( ( kRandom() % ( 32 * 1024 ) ) + 1 ) * 1024;
            pbAllocationBuffer = kAllocateMemory( qwMemorySize );

            // 만일 버퍼를 할당 받지 못하면 다른 태스크가 메모리를 사용하고 
            // 있을 수 있으므로 잠시 대기한 후 다시 시도
            if( pbAllocationBuffer == 0 )
            {
                kSleep( 1 );
            }
        } while( pbAllocationBuffer == 0 );
            
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", 
                  pbAllocationBuffer, qwMemorySize );
        // 자신의 ID를 Y 좌표로 하여 데이터를 출력
        kPrintStringXY( 20, iY, vcBuffer );
        kSleep( 200 );
        
        // 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움 
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            pbAllocationBuffer[ i ] = kRandom() & 0xFF;
            pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] = pbAllocationBuffer[ i ];
        }
        kSleep( 200 );
        
        // 채운 데이터가 정상적인지 다시 확인
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            if( pbAllocationBuffer[ i ] != pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] )
            {
                kPrintf( "Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID );
                kExitTask();
            }
        }
        kFreeMemory( pbAllocationBuffer );
        kSleep( 200 );
    }
    
    kExitTask();
}

/**
 *  태스크를 여러 개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
 */
static void kTestRandomAllocation( const char* pcParameterBuffer )
{
    int i;
    
    for( i = 0 ; i < 1000 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kRandomAllocationTask );
    }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
/**
 *  하드 디스크의 정보를 표시
 */
static void kShowHDDInformation( const char* pcParameterBuffer )
{
    HDDINFORMATION stHDD;
    char vcBuffer[ 100 ];
    
    // 하드 디스크의 정보를 읽음
    if( kGetHDDInformation(&stHDD)== FALSE )
    {
        kPrintf( "HDD Information Read Fail\n" );
        return ;
    }        
    
    kPrintf( "============ Primary Master HDD Information ============\n" );
    
    // 모델 번호 출력
    kMemCpy( vcBuffer, stHDD.vwModelNumber, sizeof( stHDD.vwModelNumber ) );
    vcBuffer[ sizeof( stHDD.vwModelNumber ) - 1 ] = '\0';
    kPrintf( "Model Number:\t %s\n", vcBuffer );
    
    // 시리얼 번호 출력
    kMemCpy( vcBuffer, stHDD.vwSerialNumber, sizeof( stHDD.vwSerialNumber ) );
    vcBuffer[ sizeof( stHDD.vwSerialNumber ) - 1 ] = '\0';
    kPrintf( "Serial Number:\t %s\n", vcBuffer );

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf( "Head Count:\t %d\n", stHDD.wNumberOfHead );
    kPrintf( "Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder );
    kPrintf( "Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder );
    
    // 총 섹터 수 출력
    kPrintf( "Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, 
            stHDD.dwTotalSectors / 2 / 1024 );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 읽음
 */
static void kReadSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;
    
    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) readsector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );
    
    // 섹터 수만큼 메모리를 할당 받아 읽기 수행
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    if( kReadHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) == iSectorCount )
    {
        kPrintf( "LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount );
        // 데이터 버퍼의 내용을 출력
        for( j = 0 ; j < iSectorCount ; j++ )
        {
            for( i = 0 ; i < 512 ; i++ )
            {
                if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
                {
                    kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                    if( kGetCh() == 'q' )
                    {
                        bExit = TRUE;
                        break;
                    }
                }                

                if( ( i % 16 ) == 0 )
                {
                    kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
                bData = pcBuffer[ j * 512 + i ] & 0xFF;
                if( bData < 16 )
                {
                    kPrintf( "0" );
                }
                kPrintf( "%X ", bData );
            }
            
            if( bExit == TRUE )
            {
                break;
            }
        }
        kPrintf( "\n" );
    }
    else
    {
        kPrintf( "Read Fail\n" );
    }
    
    kFreeMemory( pcBuffer );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 씀
 */
static void kWriteSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) writesector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    s_dwWriteCount++;
    
    // 버퍼를 할당 받아 데이터를 채움. 
    // 패턴은 4 바이트의 LBA 어드레스와 4 바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i += 8 )
        {
            *( DWORD* ) &( pcBuffer[ j * 512 + i ] ) = dwLBA + j;
            *( DWORD* ) &( pcBuffer[ j * 512 + i + 4 ] ) = s_dwWriteCount;            
        }
    }
    
    // 쓰기 수행
    if( kWriteHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) != iSectorCount )
    {
        kPrintf( "Write Fail\n" );
        return ;
    }
    kPrintf( "LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount );

    // 데이터 버퍼의 내용을 출력
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i++ )
        {
            if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
            {
                kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    bExit = TRUE;
                    break;
                }
            }                

            if( ( i % 16 ) == 0 )
            {
                kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
            }

            // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
            bData = pcBuffer[ j * 512 + i ] & 0xFF;
            if( bData < 16 )
            {
                kPrintf( "0" );
            }
            kPrintf( "%X ", bData );
        }
        
        if( bExit == TRUE )
        {
            break;
        }
    }
    kPrintf( "\n" );    
    kFreeMemory( pcBuffer );    
}
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 간단한 파일 시스템 구현
//
////////////////////////////////////////////////////////////////
/**
 *  하드 디스크를 연결
 */
static void kMountHDD( const char* pcParameterBuffer )
{
    if( kMount() == FALSE )
    {
        kPrintf( "HDD Mount Fail\n" );
        return ;
    }
    kPrintf( "HDD Mount Success\n" );
}

/**
 *  하드 디스크에 파일 시스템을 생성(포맷)
 */
static void kFormatHDD( const char* pcParameterBuffer )
{
    if( kFormat() == FALSE )
    {
        kPrintf( "HDD Format Fail\n" );
        return ;
    }
    kPrintf( "HDD Format Success\n" );
}

/**
 *  파일 시스템 정보를 표시
 */
static void kShowFileSystemInformation( const char* pcParameterBuffer )
{
    FILESYSTEMMANAGER stManager;
    
    kGetFileSystemInformation( &stManager );
    
    kPrintf( "================== File System Information ==================\n" );
    kPrintf( "Mouted:\t\t\t\t\t %d\n", stManager.bMounted );
    kPrintf( "Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount );
    kPrintf( "Cluster Link Table Start Address:\t %d Sector\n", 
            stManager.dwClusterLinkAreaStartAddress );
    kPrintf( "Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize );
    kPrintf( "Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress );
    kPrintf( "Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount );
}


////////////////////////////////////////////////////////////////////////////////
//
// C 표준 입출력 함수 추가
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  루트 디렉터리에 빈 파일을 생성
 */
static void kCreateFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE* pstFile;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    pstFile = fopen( vcFileName, "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Create Fail\n" );
        return;
    }
    fclose( pstFile );
    kPrintf( "File Create Success\n" );
}

/**
 *  루트 디렉터리에서 파일을 삭제
 */
static void kDeleteFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    if( remove( vcFileName ) != 0 )
    {
        kPrintf( "File Not Found or File Opened\n" );
        return ;
    }
    
    kPrintf( "File Delete Success\n" );
}

/**
 *  루트 디렉터리의 파일 목록을 표시
 */
static void kShowRootDirectory( const char* pcParameterBuffer )
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[ 400 ];
    char vcTempValue[ 50 ];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;
    
    // 파일 시스템 정보를 얻음
    kGetFileSystemInformation( &stManager );
     
    // 루트 디렉터리를 엶
    pstDirectory = opendir( "/" );
    if( pstDirectory == NULL )
    {
        kPrintf( "Root Directory Open Fail\n" );
        return ;
    }
    
    // 먼저 루프를 돌면서 디렉터리에 있는 파일의 개수와 전체 파일이 사용한 크기를 계산
    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;
    while( 1 )
    {
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == NULL )
        {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // 실제로 사용된 클러스터의 개수를 계산
        if( pstEntry->dwFileSize == 0 )
        {
            // 크기가 0이라도 클러스터 1개는 할당되어 있음
            dwUsedClusterCount++;
        }
        else
        {
            // 클러스터 개수를 올림하여 더함
            dwUsedClusterCount += ( pstEntry->dwFileSize + 
                ( FILESYSTEM_CLUSTERSIZE - 1 ) ) / FILESYSTEM_CLUSTERSIZE;
        }
    }
    
    // 실제 파일의 내용을 표시하는 루프
    rewinddir( pstDirectory );
    iCount = 0;
    while( 1 )
    {
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == NULL )
        {
            break;
        }
        
        // 전부 공백으로 초기화 한 후 각 위치에 값을 대입
        kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
        vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';
        
        // 파일 이름 삽입
        kMemCpy( vcBuffer, pstEntry->d_name, 
                 kStrLen( pstEntry->d_name ) );

        // 파일 길이 삽입
        kSPrintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
        kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue ) );

        // 파일의 시작 클러스터 삽입
        kSPrintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
        kMemCpy( vcBuffer + 55, vcTempValue, kStrLen( vcTempValue ) + 1 );
        kPrintf( "    %s\n", vcBuffer );

        if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
        }
        iCount++;
    }
    
    // 총 파일의 개수와 파일의 총 크기를 출력
    kPrintf( "\t\tTotal File Count: %d\n", iTotalCount );
    kPrintf( "\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte, 
             dwUsedClusterCount );
    
    // 남은 클러스터 수를 이용해서 여유 공간을 출력
    kPrintf( "\t\tFree Space: %d KByte (%d Cluster)\n", 
             ( stManager.dwTotalClusterCount - dwUsedClusterCount ) * 
             FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - 
             dwUsedClusterCount );
    
    // 디렉터리를 닫음
    closedir( pstDirectory );
}

/**
 *  파일을 생성하여 키보드로 입력된 데이터를 씀
 */
static void kWriteDataToFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "w" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 엔터 키가 연속으로 3번 눌러질 때까지 내용을 파일에 씀
    iEnterCount = 0;
    while( 1 )
    {
        bKey = kGetCh();
        // 엔터 키이면 연속 3번 눌러졌는가 확인하여 루프를 빠져 나감
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            if( iEnterCount >= 3 )
            {
                break;
            }
        }
        // 엔터 키가 아니라면 엔터 키 입력 횟수를 초기화
        else
        {
            iEnterCount = 0;
        }
        
        kPrintf( "%c", bKey );
        if( fwrite( &bKey, 1, 1, fp ) != 1 )
        {
            kPrintf( "File Wirte Fail\n" );
            break;
        }
    }
    
    kPrintf( "File Create Success\n" );
    fclose( fp );
}

/**
 *  파일을 열어서 데이터를 읽음
 */
static void kReadDataFromFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "r" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 파일의 끝까지 출력하는 것을 반복
    iEnterCount = 0;
    while( 1 )
    {
        if( fread( &bKey, 1, 1, fp ) != 1 )
        {
            break;
        }
        kPrintf( "%c", bKey );
        
        // 만약 엔터 키이면 엔터 키 횟수를 증가시키고 20라인까지 출력했다면 
        // 더 출력할지 여부를 물어봄
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            
            if( ( iEnterCount != 0 ) && ( ( iEnterCount % 20 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
                iEnterCount = 0;
            }
        }
    }
    fclose( fp );
}

/**
 *  파일 I/O에 관련된 기능을 테스트
 */
static void kTestFileIO( const char* pcParameterBuffer )
{
    FILE* pstFile;
    BYTE* pbBuffer;
    int i;
    int j;
    DWORD dwRandomOffset;
    DWORD dwByteCount;
    BYTE vbTempBuffer[ 1024 ];
    DWORD dwMaxFileSize;
    
    kPrintf( "================== File I/O Function Test ==================\n" );
    
    // 4Mbyte의 버퍼 할당
    dwMaxFileSize = 4 * 1024 * 1024;
    pbBuffer = kAllocateMemory( dwMaxFileSize );
    if( pbBuffer == NULL )
    {
        kPrintf( "Memory Allocation Fail\n" );
        return ;
    }
    // 테스트용 파일을 삭제
    remove( "testfileio.bin" );

    //==========================================================================
    // 파일 열기 테스트
    //==========================================================================
    kPrintf( "1. File Open Fail Test..." );
    // r 옵션은 파일을 생성하지 않으므로, 테스트 파일이 없는 경우 NULL이 되어야 함
    pstFile = fopen( "testfileio.bin", "r" );
    if( pstFile == NULL )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
        fclose( pstFile );
    }
    
    //==========================================================================
    // 파일 생성 테스트
    //==========================================================================
    kPrintf( "2. File Create Test..." );
    // w 옵션은 파일을 생성하므로, 정상적으로 핸들이 반환되어야함
    pstFile = fopen( "testfileio.bin", "w" );
    if( pstFile != NULL )
    {
        kPrintf( "[Pass]\n" );
        kPrintf( "    File Handle [0x%Q]\n", pstFile );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // 순차적인 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "3. Sequential Write Test(Cluster Size)..." );
    // 열린 핸들을 가지고 쓰기 수행
    for( i = 0 ; i < 100 ; i++ )
    {
        kMemSet( pbBuffer, i, FILESYSTEM_CLUSTERSIZE );
        if( fwrite( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    %d Cluster Error\n", i );
            break;
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "4. Sequential Read And Verify Test(Cluster Size)..." );
    // 파일의 처음으로 이동
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END );
    
    // 열린 핸들을 가지고 읽기 수행 후, 데이터 검증
    for( i = 0 ; i < 100 ; i++ )
    {
        // 파일을 읽음
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) !=
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        // 데이터 검사
        for( j = 0 ; j < FILESYSTEM_CLUSTERSIZE ; j++ )
        {
            if( pbBuffer[ j ] != ( BYTE ) i )
            {
                kPrintf( "[Fail]\n" );
                kPrintf( "    %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[ j ], 
                         ( BYTE ) i );
                break;
            }
        }
    }
    if( i >= 100 )
    {
        kPrintf( "[Pass]\n" );
    }

    //==========================================================================
    // 임의의 영역 쓰기 테스트
    //==========================================================================
    kPrintf( "5. Random Write Test...\n" );
    
    // 버퍼를 모두 0으로 채움
    kMemSet( pbBuffer, 0, dwMaxFileSize );
    // 여기 저기에 옮겨다니면서 데이터를 쓰고 검증
    // 파일의 내용을 읽어서 버퍼로 복사
    fseek( pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR );
    fread( pbBuffer, 1, dwMaxFileSize, pstFile );
    
    // 임의의 위치로 옮기면서 데이터를 파일과 버퍼에 동시에 씀
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( dwMaxFileSize - dwByteCount );
        
        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );

        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        kMemSet( vbTempBuffer, i, dwByteCount );
              
        // 데이터를 씀
        if( fwrite( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
        
        kMemSet( pbBuffer + dwRandomOffset, i, dwByteCount );
    }
    
    // 맨 마지막으로 이동하여 1바이트를 써서 파일의 크기를 4Mbyte로 만듦
    fseek( pstFile, dwMaxFileSize - 1, SEEK_SET );
    fwrite( &i, 1, 1, pstFile );
    pbBuffer[ dwMaxFileSize - 1 ] = ( BYTE ) i;

    //==========================================================================
    // 임의의 영역 읽기 테스트
    //==========================================================================
    kPrintf( "6. Random Read And Verify Test...\n" );
    // 임의의 위치로 옮기면서 파일에서 데이터를 읽어 버퍼의 내용과 비교
    for( i = 0 ; i < 100 ; i++ )
    {
        dwByteCount = ( kRandom() % ( sizeof( vbTempBuffer ) - 1 ) ) + 1;
        dwRandomOffset = kRandom() % ( ( dwMaxFileSize ) - dwByteCount );

        kPrintf( "    [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, 
                dwByteCount );
        
        // 파일 포인터를 이동
        fseek( pstFile, dwRandomOffset, SEEK_SET );
        
        // 데이터 읽음
        if( fread( vbTempBuffer, 1, dwByteCount, pstFile ) != dwByteCount )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Read Fail\n", dwRandomOffset ); 
            break;
        }
        
        // 버퍼와 비교
        if( kMemCmp( pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount ) 
                != 0 )
        {
            kPrintf( "[Fail]\n" );
            kPrintf( "    Compare Fail\n", dwRandomOffset ); 
            break;
        }
        
        kPrintf( "[Pass]\n" );
    }
    
    //==========================================================================
    // 다시 순차적인 영역 읽기 테스트
    //==========================================================================
    kPrintf( "7. Sequential Write, Read And Verify Test(1024 Byte)...\n" );
    // 파일의 처음으로 이동
    fseek( pstFile, -dwMaxFileSize, SEEK_CUR );
    
    // 열린 핸들을 가지고 쓰기 수행. 앞부분에서 2Mbyte만 씀
    for( i = 0 ; i < ( 2 * 1024 * 1024 / 1024 ) ; i++ )
    {
        kPrintf( "    [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024 );

        // 1024 바이트씩 파일을 씀
        if( fwrite( pbBuffer + ( i * 1024 ), 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }

    // 파일의 처음으로 이동
    fseek( pstFile, -dwMaxFileSize, SEEK_SET );
    
    // 열린 핸들을 가지고 읽기 수행 후 데이터 검증. Random Write로 데이터가 잘못 
    // 저장될 수 있으므로 검증은 4Mbyte 전체를 대상으로 함
    for( i = 0 ; i < ( dwMaxFileSize / 1024 )  ; i++ )
    {
        // 데이터 검사
        kPrintf( "    [%d] Offset [%d] Byte [%d] Read And Verify...", i, 
                i * 1024, 1024 );
        
        // 1024 바이트씩 파일을 읽음
        if( fread( vbTempBuffer, 1, 1024, pstFile ) != 1024 )
        {
            kPrintf( "[Fail]\n" );
            return ;
        }
        
        if( kMemCmp( pbBuffer + ( i * 1024 ), vbTempBuffer, 1024 ) != 0 )
        {
            kPrintf( "[Fail]\n" );
            break;
        }
        else
        {
            kPrintf( "[Pass]\n" );
        }
    }
        
    //==========================================================================
    // 파일 삭제 실패 테스트
    //==========================================================================
    kPrintf( "8. File Delete Fail Test..." );
    // 파일이 열려있는 상태이므로 파일을 지우려고 하면 실패해야 함
    if( remove( "testfileio.bin" ) != 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    //==========================================================================
    // 파일 닫기 테스트
    //==========================================================================
    kPrintf( "9. File Close Test..." );
    // 파일이 정상적으로 닫혀야 함
    if( fclose( pstFile ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }

    //==========================================================================
    // 파일 삭제 테스트
    //==========================================================================
    kPrintf( "10. File Delete Test..." );
    // 파일이 닫혔으므로 정상적으로 지워져야 함 
    if( remove( "testfileio.bin" ) == 0 )
    {
        kPrintf( "[Pass]\n" );
    }
    else
    {
        kPrintf( "[Fail]\n" );
    }
    
    // 메모리를 해제
    kFreeMemory( pbBuffer );    
}

////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
// 파일 시스템 캐시와 램 디스크 추가
//
///////////////////////////////////////////////////////////////////
/**
 *  파일을 읽고 쓰는 속도를 측정
 */
static void kTestPerformance( const char* pcParameterBuffer )
{
    FILE* pstFile;
    DWORD dwClusterTestFileSize;
    DWORD dwOneByteTestFileSize;
    QWORD qwLastTickCount;
    DWORD i;
    BYTE* pbBuffer;
    
    // 클러스터는 1Mbyte까지 파일을 테스트
    dwClusterTestFileSize = 1024 * 1024;
    // 1바이트씩 읽고 쓰는 테스트는 시간이 많이 걸리므로 16Kbyte만 테스트
    dwOneByteTestFileSize = 16 * 1024;
    
    // 테스트용 버퍼 메모리 할당
    pbBuffer = kAllocateMemory( dwClusterTestFileSize );
    if( pbBuffer == NULL )
    {
        kPrintf( "Memory Allocate Fail\n" );
        return ;
    }
    
    // 버퍼를 초기화
    kMemSet( pbBuffer, 0, FILESYSTEM_CLUSTERSIZE );
    
    kPrintf( "================== File I/O Performance Test ==================\n" );

    //==========================================================================
    // 클러스터 단위로 파일을 순차적으로 쓰는 테스트
    //==========================================================================
    kPrintf( "1.Sequential Read/Write Test(Cluster Size)\n" );

    // 기존의 테스트 파일을 제거하고 새로 만듦
    remove( "performance.txt" );
    pstFile = fopen( "performance.txt", "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Open Fail\n" );
        kFreeMemory( pbBuffer );
        return ;
    }
    
    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 쓰는 테스트
    for( i = 0 ; i < ( dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE ) ; i++ )
    {
        if( fwrite( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) != 
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "Write Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Write(Cluster Size): %d ms\n", kGetTickCount() - 
             qwLastTickCount );

    //==========================================================================
    // 클러스터 단위로 파일을 순차적으로 읽는 테스트
    //==========================================================================
    // 파일의 처음으로 이동
    fseek( pstFile, 0, SEEK_SET );
    
    qwLastTickCount = kGetTickCount();
    // 클러스터 단위로 읽는 테스트
    for( i = 0 ; i < ( dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE ) ; i++ )
    {
        if( fread( pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile ) != 
            FILESYSTEM_CLUSTERSIZE )
        {
            kPrintf( "Read Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Read(Cluster Size): %d ms\n", kGetTickCount() - 
             qwLastTickCount );
    
    //==========================================================================
    // 1 바이트 단위로 파일을 순차적으로 쓰는 테스트
    //==========================================================================
    kPrintf( "2.Sequential Read/Write Test(1 Byte)\n" );
    
    // 기존의 테스트 파일을 제거하고 새로 만듦
    remove( "performance.txt" );
    pstFile = fopen( "performance.txt", "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Open Fail\n" );
        kFreeMemory( pbBuffer );
        return ;
    }
    
    qwLastTickCount = kGetTickCount();
    // 1 바이트 단위로 쓰는 테스트
    for( i = 0 ; i < dwOneByteTestFileSize ; i++ )
    {
        if( fwrite( pbBuffer, 1, 1, pstFile ) != 1 )
        {
            kPrintf( "Write Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Write(1 Byte): %d ms\n", kGetTickCount() - 
             qwLastTickCount );

    //==========================================================================
    // 1 바이트 단위로 파일을 순차적으로 읽는 테스트
    //==========================================================================
    // 파일의 처음으로 이동
    fseek( pstFile, 0, SEEK_SET );
    
    qwLastTickCount = kGetTickCount();
    // 1 바이트 단위로 읽는 테스트
    for( i = 0 ; i < dwOneByteTestFileSize ; i++ )
    {
        if( fread( pbBuffer, 1, 1, pstFile ) != 1 )
        {
            kPrintf( "Read Fail\n" );
            // 파일을 닫고 메모리를 해제함
            fclose( pstFile );
            kFreeMemory( pbBuffer );
            return ;
        }
    }
    // 시간 출력
    kPrintf( "   Sequential Read(1 Byte): %d ms\n", kGetTickCount() - 
             qwLastTickCount );
    
    // 파일을 닫고 메모리를 해제함
    fclose( pstFile );
    kFreeMemory( pbBuffer );
}

/**
 *  파일 시스템의 캐시 버퍼에 있는 데이터를 모두 하드 디스크에 씀 
 */
static void kFlushCache( const char* pcParameterBuffer )
{
    QWORD qwTickCount;
    
    qwTickCount = kGetTickCount();
    kPrintf( "Cache Flush... ");
    if( kFlushFileSystemCache() == TRUE )
    {
        kPrintf( "Pass\n" );
    }
    else
    {
        kPrintf( "Fail\n" );
    }
    kPrintf( "Total Time = %d ms\n", kGetTickCount() - qwTickCount );
}
///////////////////////////////////////////////////////////////////


/*
static void kStringToDecimalHexTest( const char* pcParameterBuffer )
{
    char vcParameter[ 100 ];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;
    
    // 파라미터 초기화
    kInitializeParameter( &stList, pcParameterBuffer );
    
    while( 1 )
    {
        // 다음 파라미터를 구함, 파라미터의 길이가 0이면 파라미터가 없는 것이므로
        // 종료
        iLength = kGetNextParameter( &stList, vcParameter );
        if( iLength == 0 )
        {
            break;
        }

        // 파라미터에 대한 정보를 출력하고 16진수인지 10진수인지 판단하여 변환한 후
        // 결과를 printf로 출력
        kPrintf( "Param %d = '%s', Length = %d, ", iCount + 1, 
                 vcParameter, iLength );

        // 0x로 시작하면 16진수, 그외는 10진수로 판단
        if( kMemCmp( vcParameter, "0x", 2 ) == 0 )
        {
            lValue = kAToI( vcParameter + 2, 16 );
            kPrintf( "HEX Value = %q\n", lValue );
        }
        else
        {
            lValue = kAToI( vcParameter, 10 );
            kPrintf( "Decimal Value = %d\n", lValue );
        }
        
        iCount++;
    }
}
*/

////////////////////////////////////////////////////////////////////////////////
//
// 콘솔 색상 수정
//
////////////////////////////////////////////////////////////////////////////////
static void kConsoleBackGround(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	
	kInitializeParameter(&stList,pcParameterBuffer);
	
	while(1)
	{
		iLength = kGetNextParameter(&stList,vcParameter);
		if(iLength == 0)
		{
			break;
		}
		
		long lValue;
		lValue = kAToI(vcParameter,10);
		BYTE color;
		switch(lValue)
		{
			case 0:
			color = 0x00;
			break;
			case 1:
			color = 0x10;
			break;
			case 2:
			color = 0x20;
			break;
			case 3:
			color = 0x30;
			break;
			case 4:
			color = 0x40;
			break;
			case 5:
			color = 0x50;
			break;
			case 6:
			color = 0x60;
			break;
			case 7:
			color = 0x70;
			break;
			case 8:
			color = 0x80;
			break;
			case 9:
			color = 0x90;
			break;
			case 10:
			color = 0xA0;
			break;
			case 11:
			color = 0xB0;
			break;
			case 12:
			color = 0xC0;
			break;
			case 13:
			color = 0xD0;
			break;
			case 14:
			color = 0xE0;
			break;
			case 15:
			color = 0xF0;
			break;
		}
		kSetConsoleBackAttr(color);
		kClearScreenOnlyAttr();
	}
}
static void kConsoleForeGround(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	
	kInitializeParameter(&stList,pcParameterBuffer);
	
	while(1)
	{
		iLength = kGetNextParameter(&stList,vcParameter);
		if(iLength == 0)
		{
			break;
		}
		
		long lValue;
		lValue = kAToI(vcParameter,10);
		BYTE color;
		switch(lValue)
		{
			case 0:
			color = 0x00;
			break;
			case 1:
			color = 0x01;
			break;
			case 2:
			color = 0x02;
			break;
			case 3:
			color = 0x03;
			break;
			case 4:
			color = 0x04;
			break;
			case 5:
			color = 0x05;
			break;
			case 6:
			color = 0x06;
			break;
			case 7:
			color = 0x07;
			break;
			case 8:
			color = 0x08;
			break;
			case 9:
			color = 0x09;
			break;
			case 10:
			color = 0x0A;
			break;
			case 11:
			color = 0x0B;
			break;
			case 12:
			color = 0x0C;
			break;
			case 13:
			color = 0x0D;
			break;
			case 14:
			color = 0x0E;
			break;
			case 15:
			color = 0x0F;
			break;
		}
		kSetConsoleForeAttr(color);
		kClearScreenOnlyAttr();
	}
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 시리얼 포트 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  시리얼 포트로부터 데이터를 수신하여 파일로 저장
 */
static void kDownloadFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iFileNameLength;
    DWORD dwDataLength;
    FILE* fp;
    DWORD dwReceivedSize;
    DWORD dwTempSize;
    BYTE vbDataBuffer[ SERIAL_FIFOMAXSIZE ];
    QWORD qwLastReceivedTickCount;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iFileNameLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iFileNameLength ] = '\0';
    if( ( iFileNameLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || 
        ( iFileNameLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        kPrintf( "ex)download a.txt\n" );
        return ;
    }
    
    // 시리얼 포트의 FIFO를 모두 비움
    kClearSerialFIFO();
    
    //==========================================================================
    // 데이터 길이가 수신될 때까지 기다린다는 메시지를 출력하고, 4 바이트를 수신한 뒤
    // Ack를 전송
    //==========================================================================
    kPrintf( "Waiting For Data Length....." );
    dwReceivedSize = 0;
    qwLastReceivedTickCount = kGetTickCount();
    while( dwReceivedSize < 4 )
    {
        // 남은 수만큼 데이터 수신
        dwTempSize = kReceiveSerialData( ( ( BYTE* ) &dwDataLength ) +
            dwReceivedSize, 4 - dwReceivedSize );
        dwReceivedSize += dwTempSize;
        
        // 수신된 데이터가 없다면 잠시 대기
        if( dwTempSize == 0 )
        {
            kSleep( 0 );
            
            // 대기한 시간이 30초 이상이라면 Time Out으로 중지
            if( ( kGetTickCount() - qwLastReceivedTickCount ) > 30000 )
            {
                kPrintf( "Time Out Occur~!!\n" );
                return ;
            }
        }
        else
        {
            // 마지막으로 데이터를 수신한 시간을 갱신
            qwLastReceivedTickCount = kGetTickCount();
        }
    }
    kPrintf( "[%d] Byte\n", dwDataLength );

    // 정상적으로 데이터 길이를 수신했으므로, Ack를 송신
    kSendSerialData( "A", 1 );

    //==========================================================================
    // 파일을 생성하고 시리얼로부터 데이터를 수신하여 파일에 저장
    //==========================================================================
    // 파일 생성
    fp = fopen( vcFileName, "w" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 데이터 수신
    kPrintf( "Data Receive Start: " );
    dwReceivedSize = 0;
    qwLastReceivedTickCount = kGetTickCount();
    while( dwReceivedSize < dwDataLength )
    {
        // 버퍼에 담아서 데이터를 씀
        dwTempSize = kReceiveSerialData( vbDataBuffer, SERIAL_FIFOMAXSIZE );
        dwReceivedSize += dwTempSize;

        // 이번에 데이터가 수신된 것이 있다면 ACK 또는 파일 쓰기 수행
        if( dwTempSize != 0 ) 
        {
            // 수신하는 쪽은 데이터의 마지막까지 수신했거나 FIFO의 크기인 
            // 16 바이트마다 한번씩 Ack를 전송
            if( ( ( dwReceivedSize % SERIAL_FIFOMAXSIZE ) == 0 ) ||
                ( ( dwReceivedSize == dwDataLength ) ) )
            {
                kSendSerialData( "A", 1 );
                kPrintf( "#" );
            }
            
            // 쓰기 중에 문제가 생기면 바로 종료
            if( fwrite( vbDataBuffer, 1, dwTempSize, fp ) != dwTempSize )
            {
                kPrintf( "File Write Error Occur\n" );
                break;
            }
            
            // 마지막으로 데이터를 수신한 시간을 갱신
            qwLastReceivedTickCount = kGetTickCount();
        }
        // 이번에 수신된 데이터가 없다면 잠시 대기
        else
        {
            kSleep( 0 );
            
            // 대기한 시간이 10초 이상이라면 Time Out으로 중지
            if( ( kGetTickCount() - qwLastReceivedTickCount ) > 10000 )
            {
                kPrintf( "Time Out Occur~!!\n" );
                break;
            }
        }
    }   

    //==========================================================================
    // 전체 데이터의 크기와 실제로 수신 받은 데이터의 크기를 비교하여 성공 여부를
    // 출력한 뒤, 파일을 닫고 파일 시스템 캐시를 모두 비움
    //==========================================================================
    // 수신된 길이를 비교해서 문제가 발생했는지를 표시
    if( dwReceivedSize != dwDataLength )
    {
        kPrintf( "\nError Occur. Total Size [%d] Received Size [%d]\n", 
                 dwReceivedSize, dwDataLength );
    }
    else
    {
        kPrintf( "\nReceive Complete. Total Size [%d] Byte\n", dwReceivedSize );
    }
    
    // 파일을 닫고 파일 시스템 캐시를 내보냄
    fclose( fp );
    kFlushFileSystemCache();
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// MP 설정 테이블 분석
//
////////////////////////////////////////////////////////////////////////////////
/**
 *  MP 설정 테이블 정보를 출력
 */
static void kShowMPConfigurationTable( const char* pcParameterBuffer )
{
    kPrintMPConfigurationTable();
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 잠자는 코어 깨우기
//
////////////////////////////////////////////////////////////////
/**
 *  AP(Application Processor)를 시작
 */
static void kStartApplicationProcessor( const char* pcParameterBuffer )
{
    // AP(Application Processor)를 깨움
    if( kStartUpApplicationProcessor() == FALSE )
    {
        kPrintf( "Application Processor Start Fail\n" );
        return ;
    }
    kPrintf( "Application Processor Start Success\n" );
    
    // BSP(Bootstrap Processor)의 APIC ID 출력
    kPrintf( "Bootstrap Processor[APIC ID: %d] Start Application Processor\n",
             kGetAPICID() );
}
////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//
// 대칭 I/O 모드 전환 기능 통합과 빌드
//
////////////////////////////////////////////////////////////////////////
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

/**
 *  IRQ와 I/O APIC의 인터럽트 입력 핀(INTIN)의 관계를 저장한 테이블을 표시
 */
static void kShowIRQINTINMappingTable( const char* pcParameterBuffer )
{
    // I/O APIC를 관리하는 자료구조에 있는 출력 함수를 호출
    kPrintIRQToINTINMap();
}
////////////////////////////////////////////////////////////////////////
```

