#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

// 함수
BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR( QWORD qwGDTRAddress );
void kLoadTR( WORD wTSSSegmentOffset );
void kLoadIDTR( QWORD qwIDTRAddress);


void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);
////////////////////////////////////////////////////////////////////////////////
//
// 태스크 개념을 추가해 멀티태스킹 구현
//
////////////////////////////////////////////////////////////////////////////////
void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// 멀티레벨 큐 스케줄러와 태스크 종료기능 추가
//
////////////////////////////////////////////////////////////////////////////////
void kHlt(void);
////////////////////////////////////////////////////////////////////////////////

//==============================================================================
//  태스크와 인터럽트, 태스크와 태스크 사이의 동기화 문제를 해결하자
//==============================================================================
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource);
//==============================================================================

#endif /* __ASSEMBLYUTILITY_H__ */