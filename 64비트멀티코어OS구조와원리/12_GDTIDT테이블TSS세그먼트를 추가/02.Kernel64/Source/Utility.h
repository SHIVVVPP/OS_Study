#ifndef __UTILTIY_H__
#define __UTILTIY_H__

#include "Types.h"

// 함수
void kMemSet(void* pvDestination, BYTE pData, int iSize);
void kMemCpy(void* pvDestination, const void* pvSource, int iSize);
void kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

#endif /*__UTILTITY_H__ */