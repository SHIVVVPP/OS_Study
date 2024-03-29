#ifndef __UTILTIY_H__
#define __UTILTIY_H__

#include "Types.h"

////////////////////////////////////////////////////////////////////////////////
//
//  함수
//
////////////////////////////////////////////////////////////////////////////////
void kMemSet( void* pvDestination, BYTE bData, int iSize );
int kMemCpy( void* pvDestination, const void* pvSource, int iSize );
int kMemCmp( const void* pvDestination, const void* pvSource, int iSize );

////////////////////////////////////////////////////////////////////////////////
//
// 키보드 디바이스 드라이버 업데이트 추가
//
////////////////////////////////////////////////////////////////////////////////
BOOL kSetInterruptFlag( BOOL bEnableInterrupt );
////////////////////////////////////////////////////////////////////////////////

#endif /*__UTILTITY_H__ */