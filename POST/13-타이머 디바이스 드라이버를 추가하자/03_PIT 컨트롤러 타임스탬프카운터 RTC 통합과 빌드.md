# PIT 컨트롤러, 타임스탬프 카운터

# RTC 통합과 빌드

## PIT 컨트롤러 파일 추가

#### 02.Kernel64/Source/PIT.h

```c
#ifndef __PIT_H__
#define __PIT_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
#define PIT_FREQUENCY               1193182
#define MSTOCOUNT( x )              ( PIT_FREQUENCY * ( x ) / 1000 )
#define USTOCOUNT( x )              ( PIT_FREQUENCY * ( x ) / 1000000 )

// I/O 포트
#define PIT_PORT_CONTROL            0x43
#define PIT_PORT_COUNTER0           0x40
#define PIT_PORT_COUNTER1           0x41
#define PIT_PORT_COUNTER2           0x42

// 모드
#define PIT_CONTROL_COUNTER0        0x00
#define PIT_CONTROL_COUNTER1        0x40
#define PIT_CONTROL_COUNTER2        0x80
#define PIT_CONTROL_LSBMSBRW        0x30
#define PIT_CONTROL_LATCH           0x00
#define PIT_CONTROL_MODE0           0x00
#define PIT_CONTROL_MODE2           0x04

// Binary or BCD
#define PIT_CONTROL_BINARYCOUNTER   0x00
#define PIT_CONTROL_BCDCOUNTER      0x01

#define PIT_COUNTER0_ONCE     ( PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | \
                                PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER )
#define PIT_COUNTER0_PERIODIC ( PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | \
                                PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_LATCH    ( PIT_CONTROL_COUNTER0 | PIT_CONTROL_LATCH )

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
void kInitializePIT( WORD wCount, BOOL bPeriodic );
WORD kReadCounter0( void );
void kWaitUsingDirectPIT( WORD wCount );

#endif /*__PIT_H__*/

```

#### 02.Kernel64/Source/PIT.c

```c
#include "PIT.h"

/*
 *  PIT를 초기화
 */
void kInitializePIT( WORD wCount, BOOL bPeriodic )
{
    // PIT 컨트롤 레지스터(포트 0x43)에 값을 초기화하여 카운트를 멈춘 뒤에
    // 모드 0에 바이너리 카운터로 설정
    kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_ONCE );
    
    // 만약 일정한 주기로 반복하는 타이머라면 모드 2로 설정
    if( bPeriodic == TRUE )
    {
        // PIT 컨트롤 레지스터(포트 0x43)에 모드 2에 바이너리 카운터로 설정
        kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC );
    }    
    
    // 카운터 0(포트 0x40)에 LSB -> MSB 순으로 카운터 초기 값을 설정
    kOutPortByte( PIT_PORT_COUNTER0, wCount );
    kOutPortByte( PIT_PORT_COUNTER0, wCount >> 8 );
}

/**
 *  카운터 0의 현재 값을 반환
 */
WORD kReadCounter0( void )
{
    BYTE bHighByte, bLowByte;
    WORD wTemp = 0;
    
    // PIT 컨트롤 레지스터(포트 0x43)에 래치 커맨드를 전송하여 카운터 0에 있는
    // 현재 값을 읽음
    kOutPortByte( PIT_PORT_CONTROL, PIT_COUNTER0_LATCH );
    
    // 카운터 0(포트 0x40)에서 LSB -> MSB 순으로 카운터 값을 읽음
    bLowByte = kInPortByte( PIT_PORT_COUNTER0 );
    bHighByte = kInPortByte( PIT_PORT_COUNTER0 );

    // 읽은 값을 16비트로 합하여 반환
    wTemp = bHighByte;
    wTemp = ( wTemp << 8 ) | bLowByte;
    return wTemp;
}

/**
 *  카운터 0를 직접 설정하여 일정 시간 이상 대기
 *      함수를 호출하면 PIT 컨트롤러의 설정이 바뀌므로, 이후에 PIT 컨트롤러를 재설정
 *      해야 함
 *      정확하게 측정하려면 함수 사용 전에 인터럽트를 비활성화 하는 것이 좋음
 *      약 50ms까지 측정 가능
 */
void kWaitUsingDirectPIT( WORD wCount )
{
    WORD wLastCounter0;
    WORD wCurrentCounter0;
    
    // PIT 컨트롤러를 0~0xFFFF까지 반복해서 카운팅하도록 설정
    kInitializePIT( 0, TRUE );
    
    // 지금부터 wCount 이상 증가할 때까지 대기
    wLastCounter0 = kReadCounter0();
    while( 1 )
    {
        // 현재 카운터 0의 값을 반환
        wCurrentCounter0 = kReadCounter0();
        if( ( ( wLastCounter0 - wCurrentCounter0 ) & 0xFFFF ) >= wCount )
        {
            break;
        }
    }
}
```

<hr>

## 어셈블리어 유틸리티 파일 수정

#### 02.Kernel64/Source/AssemblyUtility.h

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


void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);

////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
QWORD kReadTSC(void);
////////////////////////////////////////////////////////////////////////////////

#endif /* __ASSEMBLYUTILITY_H__ */
```

#### 02.Kernel64/Source/AssemblyUtility.asm

```assembly
[BITS 64]

SECTION .text

; C 언어에서 호출할 수 있도록 이름을 노출한다.
global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
global kReadTSC
////////////////////////////////////////////////////////////////////////////////

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


;=========================================================
; 추가
;=========================================================

; 인터럽트를 활성화
;   PARAM: 없음
kEnableInterrupt:
    sti             ; 인터럽트를 활성화
    ret

; 인터럽트를 비활성화
;   PARAM: 없음
kDisableInterrupt:
    cli             ; 인터럽트를 비활성화
    ret

; RFLAGS 레지스터를 읽어서 되돌려줌
;   PARAM: 없음
kReadRFLAGS:
    pushfq                  ; RFLAGS 레지스터를 스택에 저장
    pop rax                 ; 스택에 저장된 RFLAGS 레지스터를 RAX 레지스터에 저장하여
                            ; 함수의 반환 값으로 설정
    ret

////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
; 타임 스탬프 카운터를 읽어서 반환
;   PARAM: 없음
kReadTSC:
    push rdx    ; RDX 레지스터를 스택에 저장

    rdtsc       ; 타임 스탬프 카운터를 읽어서 RDX:RAX에 저장

    shl rdx, 32 ; RDX 레지스터에 있는 상위 32비트 TSC 값과 RAX 레지스터에 있는
    or rax, rdx ; 하위 32비트 TSC 값을 OR 하여 RAX 레지스터에 64비트 TSC 값을 저장
    
    pop rdx
    ret

////////////////////////////////////////////////////////////////////////////////
```

<hr>

## RTC 컨트롤러 파일 추가

#### 02.Kernel64/Source/RTC.h

```c
#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
// 매크로
//
////////////////////////////////////////////////////////////////////////////////
// I/O 포트
#define RTC_CMOSADDRESS         0x70
#define RTC_CMOSDATA            0x71

// CMOS 메모리 어드레스
#define RTC_ADDRESS_SECOND      0x00
#define RTC_ADDRESS_MINUTE      0x02
#define RTC_ADDRESS_HOUR        0x04
#define RTC_ADDRESS_DAYOFWEEK   0x06
#define RTC_ADDRESS_DAYOFMONTH  0x07
#define RTC_ADDRESS_MONTH       0x08
#define RTC_ADDRESS_YEAR        0x09

// BCD 포맷을 Binary로 변환하는 매크로
#define RTC_BCDTOBINARY( x )    ( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0F ) )

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
void kReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond );
void kReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, 
                   BYTE* pbDayOfWeek );
char* kConvertDayOfWeekToString( BYTE bDayOfWeek );

#endif /*__RTC_H__*/

```

#### 02.Kernel64/Source/RTC.c

```c
#include "RTC.h"

/**
 *  CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 시간을 읽음
 */
void kReadRTCTime( BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond )
{
    BYTE bData;
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 시간을 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_HOUR );
    // CMOS 데이터 레지스터(포트 0x71)에서 시간을 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbHour = RTC_BCDTOBINARY( bData );
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 분을 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE );
    // CMOS 데이터 레지스터(포트 0x71)에서 분을 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbMinute = RTC_BCDTOBINARY( bData );
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 초를 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_SECOND );
    // CMOS 데이터 레지스터(포트 0x71)에서 초를 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbSecond = RTC_BCDTOBINARY( bData );
}

/**
 *  CMOS 메모리에서 RTC 컨트롤러가 저장한 현재 일자를 읽음
 */
void kReadRTCDate( WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, 
                   BYTE* pbDayOfWeek )
{
    BYTE bData;
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 연도를 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_YEAR );
    // CMOS 데이터 레지스터(포트 0x71)에서 연도를 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pwYear = RTC_BCDTOBINARY( bData ) + 2000;
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 월을 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_MONTH );
    // CMOS 데이터 레지스터(포트 0x71)에서 월을 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbMonth = RTC_BCDTOBINARY( bData );
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 일을 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH );
    // CMOS 데이터 레지스터(포트 0x71)에서 일을 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbDayOfMonth = RTC_BCDTOBINARY( bData );
    
    // CMOS 메모리 어드레스 레지스터(포트 0x70)에 요일을 저장하는 레지스터 지정
    kOutPortByte( RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK );
    // CMOS 데이터 레지스터(포트 0x71)에서 요일을 읽음
    bData = kInPortByte( RTC_CMOSDATA );
    *pbDayOfWeek = RTC_BCDTOBINARY( bData );
}

/**
 *  요일 값을 이용해서 해당 요일의 문자열을 반환
 */
char* kConvertDayOfWeekToString( BYTE bDayOfWeek )
{
    static char* vpcDayOfWeekString[ 8 ] = { "Error", "Sunday", "Monday", 
            "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    
    // 요일 범위가 넘어가면 에러를 반환
    if( bDayOfWeek >= 8 )
    {
        return vpcDayOfWeekString[ 0 ];
    }
    
    // 요일을 반환
    return vpcDayOfWeekString[ bDayOfWeek ];
}
```

<hr>

## 콘솔 셸 파일 수정

다섯 개의 커맨드가 추가된다.

PIT 컨트롤러를 초기화하고 일정시간동안 대기하는 **settimer** 와 **wait** 커맨드

타임 스탬프 카운터를 읽고 이를 이용해서 CPU 속도를 간접적으로 측정하는 **rdtsc** 와 **cpuspeed** 커맨드

현재 일자와 시간을 출력하는 **date** 커맨드

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
void kHelp( const char* pcParameterBuffer );
void kCls( const char* pcParameterBuffer );
void kShowTotalRAMSize( const char* pcParameterBuffer );
void kStringToDecimalHexTest( const char* pcParameterBuffer );
void kShutdown( const char* pcParameterBuffer );

////////////////////////////////////////////////////////////////////////////////
//
// 타이머 디바이스 드라이버 추가
//
////////////////////////////////////////////////////////////////////////////////
void kSetTimer(const char* pcParameterBuffer);
void kWaitUsingPIT(const char* pcParameterBuffer);
void kReadTimeStampCounter(const char* pcParameterBuffer);
void kMeasureProcessorSpeed(const char* pcParameterBuffer);
void kShowDateAndTime(const char* pcParameterBuffer);
////////////////////////////////////////////////////////////////////////////////
#endif /*__CONSOLESHELL_H__*/
```

#### 02.Kernel64/Source/ConsoleShell.c

```c
#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
        { "help", "Show Help", kHelp },
        { "cls", "Clear Screen", kCls },
        { "totalram", "Show Total RAM Size", kShowTotalRAMSize },
        { "strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest },
        { "shutdown", "Shutdown And Reboot OS", kShutdown },
        
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

...생략...

/**
 *  PC를 재시작(Reboot)
 */
void kShutdown( const char* pcParamegerBuffer )
{
    kPrintf( "System Shutdown Start...\n" );
    
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
void kSetTimer( const char* pcParameterBuffer )
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
void kWaitUsingPIT( const char* pcParameterBuffer )
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
void kReadTimeStampCounter( const char* pcParameterBuffer )
{
    QWORD qwTSC;
    
    qwTSC = kReadTSC();
    kPrintf( "Time Stamp Counter = %q\n", qwTSC );
}

/**
 *  프로세서의 속도를 측정
 */
void kMeasureProcessorSpeed( const char* pcParameterBuffer )
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
void kShowDateAndTime( const char* pcParameterBuffer )
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
```

<hr>



## 빌드와 실행

![image](https://user-images.githubusercontent.com/34773827/61359277-f5a11680-a8b6-11e9-906e-20e8fc20f21e.png)
![image](https://user-images.githubusercontent.com/34773827/61359301-018cd880-a8b7-11e9-8b53-18afdbb17cf8.png)
![image](https://user-images.githubusercontent.com/34773827/61359352-1bc6b680-a8b7-11e9-85e0-f2fc774632dd.png)

![image](https://user-images.githubusercontent.com/34773827/61359443-44e74700-a8b7-11e9-9b14-c794451c1834.png)

