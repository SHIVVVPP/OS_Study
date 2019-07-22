#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"

////////////////////////////////////////////////////////////////////////////////
//
// 라운드 로빈 스케줄러를 추가하자
//
////////////////////////////////////////////////////////////////////////////////
#include "Task.h"
#include "PIT.h"
////////////////////////////////////////////////////////////////////////////////

// 아래 함수는 C 언어 커널의 시작 부분이다.
void Main(void)
{
////////////////////////////////////////////////////////////////////////////////
//
// 콘솔 셸 추가
//
////////////////////////////////////////////////////////////////////////////////
	int iCursorX, iCursorY;

    // 콘솔을 먼저 초기화한 후, 다음 작업을 수행
    kInitializeConsole( 0, 10 ); 
    kPrintf( "Switch To IA-32e Mode Success~!!\n" );
    kPrintf( "IA-32e C Language Kernel Start..............[Pass]\n" );
    kPrintf( "Initialize Console..........................[Pass]\n" );
    
    // 부팅 상황을 화면에 출력
    kGetCursor( &iCursorX, &iCursorY );
    kPrintf( "GDT Initialize And Switch For IA-32e Mode...[    ]" );
    kInitializeGDTTableAndTSS();
    kLoadGDTR( GDTR_STARTADDRESS );
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass\n" );
    
    kPrintf( "TSS Segment Load............................[    ]" );
    kLoadTR( GDT_TSSSEGMENT );
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass\n" );
    
    kPrintf( "IDT Initialize..............................[    ]" );
    kInitializeIDTTables();    
    kLoadIDTR( IDTR_STARTADDRESS );
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass\n" );
    
    kPrintf( "Total RAM Size Check........................[    ]" );
    kCheckTotalRAMSize();
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass], Size = %d MB\n", kGetTotalRAMSize() );
    
////////////////////////////////////////////////////////////////////////////////
//
// 라운드 로빈 스케줄러를 추가하자
//
////////////////////////////////////////////////////////////////////////////////
    kPrintf( "TCB Pool And Scheduler Initialize...........[Pass]\n");
    iCursorY++;
    kInitializeScheduler();
    // 1ms당 한 번씩 인터럽트가 발생하도록 설정
    kInitializePIT(MSTOCOUNT(1),1);
////////////////////////////////////////////////////////////////////////////////

    kPrintf( "Keyboard Activate And Queue Initialize......[    ]" );
    // 키보드를 활성화
    if( kInitializeKeyboard() == TRUE )
    {
        kSetCursor( 45, iCursorY++ );
        kPrintf( "Pass\n" );
        kChangeKeyboardLED( FALSE, FALSE, FALSE );
    }
    else
    {
        kSetCursor( 45, iCursorY++ );
        kPrintf( "Fail\n" );
        while( 1 ) ;
    }
    
    kPrintf( "PIC Controller And Interrupt Initialize.....[    ]" );
    // PIC 컨트롤러 초기화 및 모든 인터럽트 활성화
    kInitializePIC();
    kMaskPICInterrupt( 0 );
    kEnableInterrupt();
    kSetCursor( 45, iCursorY++ );
    kPrintf( "Pass\n" );

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
    // 유휴 태스크를 생성하고 셸을 시작
    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_IDLE, (QWORD)kIdleTask);
////////////////////////////////////////////////////////////////////////////////

    // 셸을 시작
    kStartConsoleShell();
}
