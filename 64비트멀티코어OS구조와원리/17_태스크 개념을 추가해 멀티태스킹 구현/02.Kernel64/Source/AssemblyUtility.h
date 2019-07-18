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

#endif /* __ASSEMBLYUTILITY_H__ */