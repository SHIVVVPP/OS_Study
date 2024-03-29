# MP 설정 테이블 분석 기능 설계와 구현

이전까지 MP 설정 테이블을 찾고 각 정보를 분석하는 방법을 살펴보았다.

이제 이 내용을 바탕으로 자료구조를 설계하고 실제로 MP 설정 테이블을 찾아 분석하는 코드를 작성한다.

<hr>

## MP 설정 테이블 자료구조 설계

#### Multiprocessor Specification에 정의된 MP 설정 테이블 관련 자료구조와 매크로

```c
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
```

<hr>

#### 커널에서 MP 설정 테이블을 관리하는 자료구조

Multiprocessor Specification 문서에 정의된 자료구조와 매크로를 살펴보았으니<br>이제 우리 OS에서 사용할 자료구조를 살펴보자.

MP 설정 테이블에 있는 자료구조는 이미 BIOS의 데이터 영역에 존재하기 때문에 커널 영역에서 별도의 공간을 할당하여 중복으로 관리할 필요가 없다.

다만, 원하는 정보를 빨리 검색하려면 몇 가지 정보가 필요하므로 MP 플로팅 포인터의 시작 어드레스와 MP설정 테이블 헤더의 시작 어드레스, 기본 MP 설정 테이블 엔트리의 시작 어드레스를 저장한다.

또한 코어를 활성화하는 과정에서 코어의 개수가 이미 파악되어 있어야 하며,<br>I/O APIC를 설정할 때 ISA 버스로부터 전달된 인터럽트를 추출해야 하므로 두 가지 정보를 추가하면 더 편리하다.

```c
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
```

<hr>

## MP 플로팅 포인터 검색

MP 플로팅 포인터는 BIOS의 데이터 영역에 있으며 확장 BIOS 데이터 영역(EBDA, Extended BIOS Data Area)이나 시스템의 기본 메모리의 끝부분, 또는 BIOS의 롬 영역 중 어느 한 곳에 위치한다.

따라서 MP 플로팅 포인터를 찾으려면 이를 순차적으로 검색하여 MP 플로팅 포인터의 시그니처인 "_ MP _"가 존재하는지 확인한다.

<hr>

확장 BIOS 데이터 영역의 시작 어드레스는 BIOS의 데이터 영역인 0x040E에서 찾을 수 있다.<br>0x040E의 2바이트는 실제 물리주소가 아니라 확장 BIOS 데이터 영역의 세그먼트 어드레스가 들어있으므로 이를 변환해야 한다.<br>세그먼트 어드레스를 물리 어드레스로 변환하려면 16비트 환경의 세그먼트:오프셋 계산법에 따라 세그먼트 어드레스에 16을 곱하면 된다.<br>확장 BIOS 데이터 영역의 시작 어드레스를 구했다면 그로부터 1KB 범위 내에 "_ MP _" 시그니처가 있는지 확인하면 된다.

시스템 기본 메모리는 과거 1MB 미만의 메모리를 사용했을 때 시스템이 사용하는 영역을 제외한 나머지 부분 중에서 가용한 공간을 의미한다.<br>시스템 기본 메모리의 실제 크기는 BIOS 데이터 영역인 0x0413에서 찾을 수 있으며 역시 2바이트이다.<br>주의할 점은 확장 BIOS 데이터 영역과 달리 저장된 값이 KB 단위이므로,<br>1024를 곱하여 실제 크기를 계산해야 한다.<br>실제 시스템 기본 메모리 크기를 구했다면 해당 크기에서 1KB를 뺀 어드레스부터 끝까지 시그니처가 있는지 확인한다.

확장 BIOS 데이터 영역이나 시스템 기본 메모리 영역에서 찾을 수 없다면,<br>이제 남은 것은 BIOS의 롬 영역이다.<br>Multiprocessor Specification 문서는 0x0F0000 ~ 0x0FFFFF 범위에 MP 플로팅 포인터가 존재해야 한다고 정의하고 있으므로 해당 영역을 모두 뒤져서 확인한다.

<hr>

#### MP 플로팅 포인터를 검색하는 함수의 코드

```c
/**
 *  BIOS의 영역에서 MP Floating Header를 찾아서 그 주소를 반환
 */
BOOL kFindMPFloatingPointerAddress( QWORD* pstAddress )
{
    char* pcMPFloatingPointer;
    QWORD qwEBDAAddress;
    QWORD qwSystemBaseMemory;

    // 확장 BIOS 데이터 영역과 시스템 기본 메모리를 출력
    kPrintf( "Extended BIOS Data Area = [0x%X] \n", 
            ( DWORD ) ( *( WORD* ) 0x040E ) * 16 );
    kPrintf( "System Base Address = [0x%X]\n", 
            ( DWORD ) ( *( WORD* ) 0x0413 ) * 1024 );
    
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
            kPrintf( "MP Floating Pointer Is In EBDA, [0x%X] Address\n", 
                     ( QWORD ) pcMPFloatingPointer );
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
            kPrintf( "MP Floating Pointer Is In System Base Memory, [0x%X] Address\n",
                     ( QWORD ) pcMPFloatingPointer );
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
            kPrintf( "MP Floating Pointer Is In ROM, [0x%X] Address\n", 
                     pcMPFloatingPointer );
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    return FALSE;
}
```

<hr>

## MP 설정 테이블 분석 함수와 MP 설정 자료구조 반환 함수

MP 설정 테이블을 분석하는 함수는 MP 플로팅 포인터와 MP 설정 테이블 헤더, 기본 MP 설정 테이블 엔트리 정보를 분석하여 커널에서 필요한 정보를 미리 저장해둔다.

내부적으로는 MP 플로팅 포인터를 찾는 함수를 사용하며,<br>MP 플로팅 포인터를 시작으로 MP 설정 테이블 헤더와 기본 MP 설정 테이블 엔트리의 시작 어드레스를 계산한다.

MP 설정 테이블 헤더의 시작 어드레스는 MP 플로팅 포인터에 저장되어 있고,<br>기본 MP 설정 테이블 엔트리의 시작 어드레스는 MP 설정 테이블 헤더 바로 뒤에 위치하므로 헤더의 크기만큼을 더하면 구할 수 있다.

#### MP 설정 테이블을 분석하는 함수의 코드

```c
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
```

#### MP 설정을 관리하는 자료구조를 반환하는 함수

```c
/**
 *  MP 설정 테이블을 관리하는 자료구조를 반환
 */
MPCONFIGRUATIONMANAGER* kGetMPConfigurationManager( void )
{
    return &gs_stMPConfigurationManager;
}
```

<hr>

## MP 설정 테이블 출력 함수

#### MP 설정 테이블을 화면에 출력하는 함수의 코드

```c

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
```

