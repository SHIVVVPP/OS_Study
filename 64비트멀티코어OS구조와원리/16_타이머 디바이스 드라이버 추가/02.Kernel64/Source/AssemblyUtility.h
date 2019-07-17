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