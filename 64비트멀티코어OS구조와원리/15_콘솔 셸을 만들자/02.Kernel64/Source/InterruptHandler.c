#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
////////////////////////////////////////////////////////////////////////////////
//
// 콘솔 셸 추가
//
////////////////////////////////////////////////////////////////////////////////
#include "Console.h"
////////////////////////////////////////////////////////////////////////////////


/**
 *  공통으로 사용하는 예외 핸들러
 */
void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode )
{
    char vcBuffer[ 3 ] = { 0, };

    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[ 0 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 1 ] = '0' + iVectorNumber % 10;

    
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 콘솔 셸 추가
    //
    ////////////////////////////////////////////////////////////////////////////////
    kPrintStringXY( 0, 20, "====================================================" );
    kPrintStringXY( 0, 21, "                 Exception Occur~!!!!               " );
    kPrintStringXY( 0, 22, "                    Vector:                         " );
    kPrintStringXY(27, 22, vcBuffer );
    kPrintStringXY( 0, 23, "====================================================" );
    ////////////////////////////////////////////////////////////////////////////////


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
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 콘솔 셸 추가
    //
    ////////////////////////////////////////////////////////////////////////////////
    kPrintStringXY( 70, 20, vcBuffer );
    ////////////////////////////////////////////////////////////////////////////////
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
    BYTE bTemp;

    //=========================================================================
    // 인터럽트가 발생했음을 알리려고 메시지를 출력하는 부분
    // 인터럽트 벡터를 화면 왼쪽 위에 2자리 정수로 출력
    vcBuffer[ 5 ] = '0' + iVectorNumber / 10;
    vcBuffer[ 6 ] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[ 8 ] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = ( g_iKeyboardInterruptCount + 1 ) % 10;
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 콘솔 셸 추가
    //
    ////////////////////////////////////////////////////////////////////////////////
    kPrintStringXY( 0, 20, vcBuffer );
    ////////////////////////////////////////////////////////////////////////////////
    //=========================================================================


    // 키보드 컨트롤러에서 데이터를 읽어서 ASCII로 변환하여 큐에 삽입
    if(kIsOutputBufferFull() == TRUE)
    {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    // EOI 전송
    kSendEOIToPIC( iVectorNumber - PIC_IRQSTARTVECTOR );
}
