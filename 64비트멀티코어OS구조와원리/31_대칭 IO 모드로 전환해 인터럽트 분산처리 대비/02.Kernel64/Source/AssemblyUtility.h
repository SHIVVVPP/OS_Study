#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
BYTE kInPortByte( WORD wPort );
void kOutPortByte( WORD wPort, BYTE bData );
void kLoadGDTR( QWORD qwGDTRAddress );
void kLoadTR( WORD wTSSSegmentOffset );
void kLoadIDTR( QWORD qwIDTRAddress);
void kEnableInterrupt( void );
void kDisableInterrupt( void );
QWORD kReadRFLAGS( void );
QWORD kReadTSC( void );
void kSwitchContext( CONTEXT* pstCurrentContext, CONTEXT* pstNextContext );
void kHlt( void );
BOOL kTestAndSet( volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource );
void kInitializeFPU( void );
void kSaveFPUContext( void* pvFPUContext );
void kLoadFPUContext( void* pvFPUContext );
void kSetTS( void );
void kClearTS( void );
////////////////////////////////////////////////////////////////
//
// 하드디스크 디바이스 드라이버 추가 
//
////////////////////////////////////////////////////////////////
WORD kInPortWord(WORD wPort);
void kOutPortWord(WORD wPort, WORD wData);
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//
// 잠자는 코어 깨우기
//
////////////////////////////////////////////////////////////////
void kEnableGlobalLocalAPIC( void );
////////////////////////////////////////////////////////////////

#endif /*__ASSEMBLYUTILITY_H__*/
