#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"


// 구조체
// 1바이트로 정렬
#pragma pack(push,1)

// 큐에 대한 구조체
typedef struct kQueueManagerStruct
{
    // 큐를 구성하는 데이터 하나의 크기와 최대 개수
    int iDataSize;
    int iMaxDataCount;

    // 큐 버퍼의 포인터와 삽입/제거 인덱스
    void* pvQueueArray;
    int iPutIndex;
    int iGetIndex;

    // 큐에 수행된 마지막 명령이 삽인인지를 저장
    BOOL bLastOperationPut;
}QUEUE;

#pragma pack(pop)

// 함수
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
BOOL kIsQueueFull(const QUEUE* pstQueue);
BOOL kIsQueueEmpty(const QUEUE* pstQueue);
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData);
BOOL kGetQueue(QUEUE* pstQueue, void* pvData);

#endif /* __QUEUE_H__ */