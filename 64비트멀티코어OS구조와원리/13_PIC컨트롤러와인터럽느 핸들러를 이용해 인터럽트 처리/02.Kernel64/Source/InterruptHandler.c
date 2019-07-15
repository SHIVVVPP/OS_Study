#include "InterruptHandler.h"
#include "PIC.h"

/**
 *  공통으로 사용하는 예외 핸들러
 */
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
    char vcBuffer[ 3 ] = { 0, };

    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 1 ] = '0' + iVectorNumber % 10;

    kPrintString( 0, 20,0xE4, "====================================================" );
    kPrintString( 0, 21,0xE4, "                 Exception Occur~!!!!               " );
    kPrintString( 0, 22,0xE4, "                    Vector:                         " );
    kPrintString( 27, 22,0xE4, vcBuffer );
    kPrintString( 0, 24,0xE4, "====================================================" );

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
    kPrintString( 70, 0,0xE4, vcBuffer );
    //=========================================================================

    // EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}

/**
 *  키보드 인터럽트의 핸들러
 */
void kKeyboardHandler( int iVectorNumber )
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    kPrintString( 0, 19,0xE4, vcBuffer );
    //=========================================================================

    // EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}
