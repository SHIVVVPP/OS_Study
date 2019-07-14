# IDT, TSS 통합과 빌드

## 디스크립터 파일 추가

### 02.Kernel64/Source/Descriptor.h

```c
#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
//==============================================================================
// GDT
//==============================================================================
// 조합에 사용할 기본 매크로
#define GDT_TYPE_CODE           0x0A
#define GDT_TYPE_DATA           0x02
#define GDT_TYPE_TSS            0x09
#define GDT_FLAGS_LOWER_S       0x10
#define GDT_FLAGS_LOWER_DPL0    0x00
#define GDT_FLAGS_LOWER_DPL1    0x20
#define GDT_FLAGS_LOWER_DPL2    0x40
#define GDT_FLAGS_LOWER_DPL3    0x60
#define GDT_FLAGS_LOWER_P       0x80
#define GDT_FLAGS_UPPER_L       0x20
#define GDT_FLAGS_UPPER_DB      0x40
#define GDT_FLAGS_UPPER_G       0x80

// 실제로 사용할 매크로
// Lower Flags는 Code/Data/TSS, DPL0, Present로 설정
#define GDT_FLAGS_LOWER_KERNELCODE ( GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
        GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_KERNELDATA ( GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
        GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_TSS ( GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_USERCODE ( GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
        GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P )
#define GDT_FLAGS_LOWER_USERDATA ( GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
        GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P )

// Upper Flags는 Granulaty로 설정하고 코드 및 데이터는 64비트 추가
#define GDT_FLAGS_UPPER_CODE ( GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L )
#define GDT_FLAGS_UPPER_DATA ( GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L )
#define GDT_FLAGS_UPPER_TSS ( GDT_FLAGS_UPPER_G )

// 세그먼트 디스크립터 오프셋
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// 기타 GDT에 관련된 매크로
// GDTR의 시작 어드레스, 1Mbyte에서 264Kbyte까지는 페이지 테이블 영역
#define GDTR_STARTADDRESS   0x142000
// 8바이트 엔트리의 개수, 널 디스크립터/커널 코드/커널 데이터
#define GDT_MAXENTRY8COUNT  3
// 16바이트 엔트리의 개수, TSS
#define GDT_MAXENTRY16COUNT 1
// GDT 테이블의 크기
#define GDT_TABLESIZE       ( ( sizeof( GDTENTRY8 ) * GDT_MAXENTRY8COUNT ) + \
        ( sizeof( GDTENTRY16 ) * GDT_MAXENTRY16COUNT ) )
#define TSS_SEGMENTSIZE     ( sizeof( TSSSEGMENT ) )

//==============================================================================
// IDT
//==============================================================================
// 조합에 사용할 기본 매크로
#define IDT_TYPE_INTERRUPT      0x0E
#define IDT_TYPE_TRAP           0x0F
#define IDT_FLAGS_DPL0          0x00
#define IDT_FLAGS_DPL1          0x20
#define IDT_FLAGS_DPL2          0x40
#define IDT_FLAGS_DPL3          0x60
#define IDT_FLAGS_P             0x80
#define IDT_FLAGS_IST0          0
#define IDT_FLAGS_IST1          1

// 실제로 사용할 매크로
#define IDT_FLAGS_KERNEL        ( IDT_FLAGS_DPL0 | IDT_FLAGS_P )
#define IDT_FLAGS_USER          ( IDT_FLAGS_DPL3 | IDT_FLAGS_P )

// 기타 IDT에 관련된 매크로
// IDT 엔트리의 개수
#define IDT_MAXENTRYCOUNT       100
// IDTR의 시작 어드레스, TSS 세그먼트의 뒤쪽에 위치
#define IDTR_STARTADDRESS       ( GDTR_STARTADDRESS + sizeof( GDTR ) + \
        GDT_TABLESIZE + TSS_SEGMENTSIZE )
// IDT 테이블의 시작 어드레스
#define IDT_STARTADDRESS        ( IDTR_STARTADDRESS + sizeof( IDTR ) )
// IDT 테이블의 전체 크기
#define IDT_TABLESIZE           ( IDT_MAXENTRYCOUNT * sizeof( IDTENTRY ) )

// IST의 시작 어드레스
#define IST_STARTADDRESS        0x700000
// IST의 크기
#define IST_SIZE                0x100000

////////////////////////////////////////////////////////////////////////////////
//
// 구조체
//
////////////////////////////////////////////////////////////////////////////////
// 1바이트로 정렬
#pragma pack( push, 1 )

// GDTR 및 IDTR 구조체
typedef struct kGDTRStruct
{
    WORD wLimit;
    QWORD qwBaseAddress;
    // 16바이트 어드레스 정렬을 위해 추가
    WORD wPading;
    DWORD dwPading;
} GDTR, IDTR;

// 8바이트 크기의 GDT 엔트리 구조
typedef struct kGDTEntry8Struct
{
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bUpperBaseAddress1;
    // 4비트 Type, 1비트 S, 2비트 DPL, 1비트 P
    BYTE bTypeAndLowerFlag;
    // 4비트 Segment Limit, 1비트 AVL, L, D/B, G
    BYTE bUpperLimitAndUpperFlag;
    BYTE bUpperBaseAddress2;
} GDTENTRY8;

// 16바이트 크기의 GDT 엔트리 구조
typedef struct kGDTEntry16Struct
{
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bMiddleBaseAddress1;
    // 4비트 Type, 1비트 0, 2비트 DPL, 1비트 P
    BYTE bTypeAndLowerFlag;
    // 4비트 Segment Limit, 1비트 AVL, 0, 0, G
    BYTE bUpperLimitAndUpperFlag;
    BYTE bMiddleBaseAddress2;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} GDTENTRY16;

// TSS Data 구조체
typedef struct kTSSDataStruct
{
    DWORD dwReserved1;
    QWORD qwRsp[ 3 ];
    QWORD qwReserved2;
    QWORD qwIST[ 7 ];
    QWORD qwReserved3;
    WORD wReserved;
    WORD wIOMapBaseAddress;
} TSSSEGMENT;

// IDT 게이트 디스크립터 구조체
typedef struct kIDTEntryStruct
{
    WORD wLowerBaseAddress;
    WORD wSegmentSelector;
    // 3비트 IST, 5비트 0
    BYTE bIST;
    // 4비트 Type, 1비트 0, 2비트 DPL, 1비트 P
    BYTE bTypeAndFlags;
    WORD wMiddleBaseAddress;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} IDTENTRY;

#pragma pack ( pop )

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
void kInitializeGDTTableAndTSS( void );
void kSetGDTEntry8( GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType );
void kSetGDTEntry16( GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType );
void kInitializeTSSSegment( TSSSEGMENT* pstTSS );

void kInitializeIDTTables( void );
void kSetIDTEntry( IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, 
        BYTE bIST, BYTE bFlags, BYTE bType );
void kDummyHandler( void );

#endif

```



### 02.Kernel64/Source/Descriptor.c

```C
#include "Descriptor.h"
#include "Utility.h"

//==============================================================================
//  GDT 및 TSS
//==============================================================================

/**
 *  GDT 테이블을 초기화
 */
void kInitializeGDTTableAndTSS( void )
{
    GDTR* pstGDTR;
    GDTENTRY8* pstEntry;
    TSSSEGMENT* pstTSS;
    int i;
    
    // GDTR 설정
    pstGDTR = ( GDTR* ) GDTR_STARTADDRESS;
    pstEntry = ( GDTENTRY8* ) ( GDTR_STARTADDRESS + sizeof( GDTR ) );
    pstGDTR->wLimit = GDT_TABLESIZE - 1;
    pstGDTR->qwBaseAddress = ( QWORD ) pstEntry;
    // TSS 영역 설정
    pstTSS = ( TSSSEGMENT* ) ( ( QWORD ) pstEntry + GDT_TABLESIZE );

    // NULL, 64비트 Code/Data, TSS를 위해 총 4개의 세그먼트를 생성한다.
    kSetGDTEntry8( &( pstEntry[ 0 ] ), 0, 0, 0, 0, 0 );
    kSetGDTEntry8( &( pstEntry[ 1 ] ), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, 
            GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE );
    kSetGDTEntry8( &( pstEntry[ 2 ] ), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA,
            GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA );
    kSetGDTEntry16( ( GDTENTRY16* ) &( pstEntry[ 3 ] ), ( QWORD ) pstTSS, 
            sizeof( TSSSEGMENT ) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, 
            GDT_TYPE_TSS ); 
    
    // TSS 초기화 GDT 이하 영역을 사용함
    kInitializeTSSSegment( pstTSS );
}

/**
 *  8바이트 크기의 GDT 엔트리에 값을 설정
 *      코드와 데이터 세그먼트 디스크립터를 설정하는데 사용
 */
void kSetGDTEntry8( GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType )
{
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
    pstEntry->bUpperBaseAddress1 = ( dwBaseAddress >> 16 ) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ( ( dwLimit >> 16 ) & 0xFF ) | 
        bUpperFlags;
    pstEntry->bUpperBaseAddress2 = ( dwBaseAddress >> 24 ) & 0xFF;
}

/**
 *  16바이트 크기의 GDT 엔트리에 값을 설정
 *      TSS 세그먼트 디스크립터를 설정하는데 사용
 */
void kSetGDTEntry16( GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
        BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType )
{
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
    pstEntry->bMiddleBaseAddress1 = ( qwBaseAddress >> 16 ) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ( ( dwLimit >> 16 ) & 0xFF ) | 
        bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = ( qwBaseAddress >> 24 ) & 0xFF;
    pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}

/**
 *  TSS 세그먼트의 정보를 초기화
 */
void kInitializeTSSSegment( TSSSEGMENT* pstTSS )
{
    kMemSet( pstTSS, 0, sizeof( TSSSEGMENT ) );
    pstTSS->qwIST[ 0 ] = IST_STARTADDRESS + IST_SIZE;
    // IO 를 TSS의 limit 값보다 크게 설정함으로써 IO Map을 사용하지 않도록 함
    pstTSS->wIOMapBaseAddress = 0xFFFF;
}


//==============================================================================
//  IDT
//==============================================================================
/**
 *  IDT 테이블을 초기화
 */
void kInitializeIDTTables( void )
{
    IDTR* pstIDTR;
    IDTENTRY* pstEntry;
    int i;
        
    // IDTR의 시작 어드레스
    pstIDTR = ( IDTR* ) IDTR_STARTADDRESS;
    // IDT 테이블의 정보 생성
    pstEntry = ( IDTENTRY* ) ( IDTR_STARTADDRESS + sizeof( IDTR ) );
    pstIDTR->qwBaseAddress = ( QWORD ) pstEntry;
    pstIDTR->wLimit = IDT_TABLESIZE - 1;
    
    // 0~99까지 벡터를 모두 DummyHandler로 연결
    for( i = 0 ; i < IDT_MAXENTRYCOUNT ; i++ )
    {
        kSetIDTEntry( &( pstEntry[ i ] ), kDummyHandler, 0x08, IDT_FLAGS_IST1, 
            IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
    }
}

/**
 *  IDT 게이트 디스크립터에 값을 설정
 */
void kSetIDTEntry( IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, 
        BYTE bIST, BYTE bFlags, BYTE bType )
{
    pstEntry->wLowerBaseAddress = ( QWORD ) pvHandler & 0xFFFF;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x3;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ( ( QWORD ) pvHandler >> 16 ) & 0xFFFF;
    pstEntry->dwUpperBaseAddress = ( QWORD ) pvHandler >> 32;
    pstEntry->dwReserved = 0;
}

/**
 *  임시 예외 또는 인터럽트 핸들러
 */
void kDummyHandler( void )
{
    kPrintString( 0, 0,0xAD, "====================================================" );
    kPrintString( 0, 1,0xA4, "          Dummy Interrupt Handler Execute~!!!       " );
    kPrintString( 0, 2,0xA4, "           Interrupt or Exception Occur~!!!!        " );
    kPrintString( 0, 3,0xAD, "====================================================" );

    while( 1 ) ;
}
```

## 어셈블리어 유틸리티 파일 수정

#### 02.Kernel64/Source/AssemblyUtility.asm

```assembly
[BITS 64]

SECTION .text

; C 언어에서 호출할 수 있도록 이름을 노출한다.
global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR

; 포트로부터 1바이트를 읽는다.
;   PARAM : 포트 번호
kInPortByte:
    push rdx

    mov rdx, rdi
    mov rax, 0
    in al, dx

    pop rdx
    ret

; 포트에 1바이트를 쓴다.
;   PARAM : 포트 번호, 데이터
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al

    pop rax
    pop rdx
    ret

;=========================================================
; 추가
;=========================================================

; GDTR 레지스터에 GDT 테이블을 설정
;   PARAM: GDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadGDTR:
    lgdt [ rdi ]    ; 파라미터 1(GDTR의 어드레스)를 프로세서에 로드하여 
                    ; GDT 테이블을 설정
    ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
;   PARAM: TSS 세그먼트 디스크립터의 오프셋
kLoadTR:
    ltr di          ; 파라미터 1(TSS 세그먼트 디스크립터의 오프셋)을 프로세서에
                    ; 설정하여 TSS 세그먼트를 로드
    ret
    
; IDTR 레지스터에 IDT 테이블을 설정
;   PARAM: IDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadIDTR:
    lidt [ rdi ]    ; 파라미터 1(IDTR의 어드레스)을 프로세서에 로드하여
                    ; IDT 테이블을 설정
    ret
```

### 02.Kernel64/Source/AssemblyUtiltiy.h

```c
#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"

// 함수
BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR( QWORD qwGDTRAddress );
void kLoadTR( WORD wTSSSegmentOffset );
void kLoadIDTR( QWORD qwIDTRAddress);

#endif /* __ASSEMBLYUTILITY_H__ */
```

<hr>



## 유틸리티 파일 추가

유틸리티 파일은 앞으로 커널에서 사용할 유틸리티 함수를 정의한 파일로 이번 장에서 사용한 *kMemSet( )* 함수가 여기에 속한다.

앞으로 자주 사용할 메모리 관련 함수인 *kMemSet( )*, *kMemCpy( )*, *kMemCmp( )* 함수가 포함되어 있다.

#### 02.Kernel64/Source/Utility.h

```c
#ifndef __UTILTIY_H__
#define __UTILTIY_H__

#include "Types.h"

// 함수
void kMemSet(void* pvDestination, BYTE pData, int iSize);
void kMemCpy(void* pvDestination, const void* pvSource, int iSize);
void kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

#endif /*__UTILTITY_H__ */
```

#### 02.Kernel64/Source/Utility.c

```c
#include "Utility.h"

/**
 *  메모리를 특정 값으로 채움
 */
void kMemSet( void* pvDestination, BYTE bData, int iSize )
{
    int i;
    
    for( i = 0 ; i < iSize ; i++ )
    {
        ( ( char* ) pvDestination )[ i ] = bData;
    }
}

/**
 *  메모리 복사
 */
int kMemCpy( void* pvDestination, const void* pvSource, int iSize )
{
    int i;
    
    for( i = 0 ; i < iSize ; i++ )
    {
        ( ( char* ) pvDestination )[ i ] = ( ( char* ) pvSource )[ i ];
    }
    
    return iSize;
}

/**
 *  메모리 비교
 */
int kMemCmp( const void* pvDestination, const void* pvSource, int iSize )
{
    int i;
    char cTemp;
    
    for( i = 0 ; i < iSize ; i++ )
    {
        cTemp = ( ( char* ) pvDestination )[ i ] - ( ( char* ) pvSource )[ i ];
        if( cTemp != 0 )
        {
            return ( int ) cTemp;
        }
    }
    return 0;
}
```

<hr>

