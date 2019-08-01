# 버디 블록 알고리즘 구현

## 동적 메모리 할당을 위한 메모리 영역 지정

메모리를 동적으로 할당하고 해제하려면 전체 메모리 영역에서 어느 만큼을 동적 메모리로 사용할지 정해야 한다.

멀티태스킹 기능이 추가된 지금까지 OS가 사용한 메모리는 다음과 같다.

![image](https://user-images.githubusercontent.com/34773827/61959625-c600b580-affe-11e9-8453-f9dcbd7842f1.png)

약 17MB 영역까지 메모리를 사용한다.<br>따라서 동적 할당을 위한 영역은 최소한 스택 풀이 끝나는 위치에서 시작해야 한다.

동적 메모리 영역의 시작 어드레스를 17MB로 결정 되었으니, 마지막 어드레스를 정해야한다.<br>마지막 어드레스는 실제 존재하는 물리 메모리의 범위 안에만 범위 안에만 존재한다면 어느 곳을 지정해도 되지만, 

시작 어드레스와 마지막 어드레스에 따라 최대로 할당할 수 있는 블록의 크기와 블록의 수가 달라지므로 이 점을 고려해 결정해야 한다.

우리 OS는 단순한 메모리 할당과 해제부터 응용프로그램 실행, 그래픽 유저 인터페이스까지 메모리가 필요한 모든 작업을 동적 메모리로 처리한다.

따라서, 동적 메모리 영역이 크면 클수록 더 많은 작업을 할 수 있으므로,<br>3GB 범위 내의 물리 메모리의 끝까지를 동적 메모리 영역으로 설정한다.

<hr>

## 버디 블록 알고리즘 세부 설계

버디 블록 알고리즘은 메모리를 할당하고 해제할 때마다 블록을 분할하고 결합하는 작업을 한다.<br>최악의 경우 분할하고 결합하는 작업을 매번 반복해야 하므로,<br>이러한 작업을 빠르게 수행할 수 있도록 **블록 리스트를 유지하는 것이 핵심이다.**

검색이 빈번한 경우는 리스트가 적합하지 않으므로, 일반적으로 많이 사용하는 비트맵을 통해 구현한다.



### 비트맵

비트맵(Bitmap)이란 **데이터당 1비트를 할당하여 0이나 1로 정보를 표시하는 방법** 이다.

> 1바이트는 8비트이므로 비트맵을 사용하면 작은 공간으로 많은 데이터를 저장할 수 있다.<br>또한 데이터의 인덱스를 바이트 오프셋(Byte Offset)과 바이트 내에 비트 오프셋(Bit Offset)으로 변환하여 특정 데이터에 직접 접근할 수 있으므로, 리스트와 달리 인접한 블록에 바로 접근할 수 있는 장점이 있다.



### 블록 인덱스와 비트맵의 연결

비트맵으로 블록 리스트를 저장하는 방법은 블록을 어떠한 순서로 비트와 연결할 것인가만 결정되면<br>간단한 계산식으로 처리할 수 있다.

일반적으로 1바이트 내에서는 비트 0부터 비트 7의 순서로 비트를 할당하며,<br>바이트 순서는 첫번째 바이트부터 마지막 바이트의 순으로 할당하는 방식을 많이 사용한다.

다음은 메모리 블록과 비트맵의 관계를 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/61962460-8341dc00-b004-11e9-8906-81f39de2b1dc.png)

위 두 그림을 비교하면 두 가지를 알 수 있다.

1. 블록 인덱스와 비트맵의 관계

   가장 위에 있는 블록 리스트는 총 16개 이며, 블록 인덱스는 0 ~ 15가 된다.

   이를 비트맵으로 옮기면 1바이트에 8개 저장할 수 있으므로 0 ~ 7, 8 ~ 15씩 2바이트로 나누어 저장한다.

   그리고 블록의 수가 8개보다 작은 경우에도 최소한 1바이트가 필요하며,<BR>8비트 중에서 실제로 블록이 존재하는 비트만 사용한다.

2. 상위 블록 인덱스와 하위 블록 인덱스의 관계

   첫번째 그림을 보면 상위 블록 인덱스와 하위 블록 인덱스 간에 일정한 규칙이 있다.

   상위 블록은 모두 2개의 하위 블록으로 분할될 수 있으므로,<BR>하위 블록의 인덱스는 상위 블록의 인덱스에 2배와 2배에 1을 더한 값이 된다.

   반대로 상위 블록의 인덱스는 하위 블록 인덱스를 반으로 나누면 구할 수 있다.

<HR>

### 동적 메모리 영역을 관리하는 자료구조의 코드

```c
////////////////////////////////////////////////////////////////////////////////
// 비트맵을 관리하는 자료구조///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 비트맵의 어드레스와 비트맵 내에 존재하는 데이터의 개수로 구성된다.
typedef struct kBitmapStruct
{
    BYTE* pbBitmap;
    QWORD qwExistBitCount;
}BITMAP;

// 버디 블록을 관리하는 자료구조
typedef struct kDynamicMemoryManagerStruct
{
    ////////////////////////////////////////////////////////////////////////////////
    // 블록 리스트의 총 개수와 가장 크기가 가장 작은 블록의 개수, 그리고 할당된 메모리/////////
    ////////////////////////////////////////////////////////////////////////////////
    /*
    버디 블록 알고리즘 특성상 상위 리스트로 갈수록 블록 개수가 절반이 되므로,
    가장 크기가 작은 블록 리스트에 포함된 블록의 수를 이용해서 전체 비트맵의 크기를 계산할 수 있다.
    */
    // 메모리 영역을 블록 리스트로 구성할 때 필요한 블록 리스트의 수, 즉 계층의 수
    int iMaxLevelCount;
    // 사용 가능한 동적 메모리 영역을 가장 크기가 작은 블록으로 나눈 개수
    int iBlockCountOfSmallestBlock;
    QWORD qwUsedSize;
    
    ////////////////////////////////////////////////////////////////////////////////
    // 블록 풀의 시작 어드레스와 마지막 어드레스//////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    /*
    블록의 개수는 동적 메모리 영역의 크기에 따라서 달라지며,
    동적 메모리 영역의 크기는 물리 메모리 크기에 따라 달라진다.
    따라서 메모리의 크기에 따라서 비트맵과 할당된 메모리 크기를 저장하는 영역을 다르게 지정해야 하며,
    밑의 변수는 이를 위해 추가한 것이다.
    */
    QWORD qwStartAddress;
    QWORD qwEndAddress;
    
    
    ////////////////////////////////////////////////////////////////////////////////
    // 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역과 비트맵 자료구조의 어드레스////
    ////////////////////////////////////////////////////////////////////////////////
    /* 
    할당된 블록이 속하는 블록 리스트의 인덱스를 저장한다.
    할당된 블록이 속한 블록 리스트가 필요한 이유는 블록을 해제할 때
    블록 리스트의 인덱스를 알아야 해당 블록 리스트에 삽입할 수 있기 때문이다.
    1KB 리스트의 첫 번째 블록과 2KB 리스트의 첫 번째 블록, 4KB 등 첫 번째 블록은 시작위치가 같다.
    이는 블록의 크기와 관계없이 모두 같은 메모리 어드레스가 나올 수 있다는 것을 의미한다.
    따라서 어드레스로 블록 리스트의 인덱스를 구분하려면 결국 어드레스와 블록리스트의 인덱스를
    같이 저장해야 한다. 
    */
    BYTE* pbAllocatedBlockListIndex;
    
    
    // 버디 블록의 핵심으로 블록리스트의 정보를 담고 있는 비트맵 자료구조의 배열을 가리킨다.
    BITMAP* pstBitmapOfLevel;
}DYNAMICMEMORY;
```

<hr>

### 동적 메모리 영역 크기에 따른 비트맵 영역과<br>할당된 메모리를 저장하는 영역의 크기 계산

동적 할당용으로 사용할 영역이 64MB이고 최소 블록의 크기가 1KB라면,

최소 블록의 크기가 1KB이고 메모리 영역이 64MB이므로 최소 블록의 개수는 총 64 X 1024(64K) 개가 된다.

비트맵을 사용하면 1바이트에 8개의 블록을 저장할 수 있으므로,<br>크기가 가장 작은 블록 리스트의 비트맵은 8KB(64K/8)가 필요하다.

상위 블록 리스트로 갈수록 블록의 개수가 절반이 되고 비트맵의 크기 역시 절반이 되므로, 전체 비트맵의 크기는 블록 리스트의 수만 알면 구할 수 있다.

블록 리스트의 수는 블록이 하나가 남을 때까지 2로 나누는 것을 반복하면 되고 64K 는 2의 17승으로 나타낼 수 있으므로 블록 리스트의 수는 총 17개가 된다.

따라서 비트맵을 저장하는데 필요한 메모리는 64K/8 + 32K/8 + 16K/8 + ... + 1/8이 되어 총 16386바이트가 된다.

할당된 메모리의 크기(블록 리스트의 인덱스)를 저장하는 영역은 크기가 작은 블록의 수만큼 필요하므로,<br>64K X sizeof(BYTE)가 되어  64KB가 된다.

이를 앞서 계산한 비트맵의 크기와 합하면 최종적으로 필요한 크기는 약 80KB가 되며<br>필요함 메모리의 크기는 동적 메모리 영역의 크기와 비례하므로 PC에 설치된 메모리의 크기가 128MB라면 2배인 약 160KB 정도가 필요하며, 1GB라면 약 1280KB(1.25MB) 정도의 메모리가 필요하다.

#### 비트맵과 할당된 블록 리스트의 인덱스를 저장하는 용도로 사용되는 영역

![image](https://user-images.githubusercontent.com/34773827/61965593-855b6900-b00b-11e9-8775-b8152790e242.png)

<hr>

## 동적 메모리 자료구조 초기화

동적 메모리를 초기화하는 함수는 비트맵과 할당된 크기를 저장하는 영역을 초기화한다.

두 영역을 초기화하려면 우선 동적 메모리 영역으로 사용할 크기를 계산한 후 각 영역이 얼마나 메모리를 사용하는지 알아야 한다.

그러므로 동적 메모리로 사용할 영역의 크기를 파라미터로 넘겨주면,<br>두 영역이 사용하는 크기르 반환해주는 함수를 먼저 작성한다.

#### 동적 메모리를 관리하는데 필요한 영역의 합을 반환하는 함수와 매크로

```c
// 동적 메모리 영역의 시작 어드레스, 1MB 단위로 정렬
#define DYNAMICMEMORY_START_ADDRESS	((TASK_STACKPOOLADDRESS + \
						(TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff)& 0xfffffffffff00000)
// 버디 블록의 최소 크기, 1MB
#define DYNAMICMEMORY_MIN_SIZE	(1 * 1024)

// 동적 메모리 영역을 관리하는데 필요한 정보가 차지하는 공간을 계산
//		최소 블록 단위로 정렬해서 반환
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize)
{
    long lBlockCountOfSmallestBlock;
    DWORD dwSizeOfAllocatedBlockListIndex;
    DWORD dwSizeOfBitmap;
    long i;
    
    // 가장 크기가 작은 블록의 개수를 계산하여 이를 기준으로 비트맵 영역과
    // 할당된 크기를 저장하는 영역을 계산
    lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
    // 할당된 블록이 속한 블록 리스트의 인덱스를 저장하는데 필요한 영역을 계산
    dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);
    
    // 비트맵을 저장하는데 필요한 공간 계산
    dwSizeOfBitmap = 0;
    for( i = 0 ; (lBlockCountOfSmallestBlock >> i) > 0 ; i++ )
    {
        // 블록 리스트의 비트맵 포인터를 위한 공간
        dwSizeOfBitmap += sizeof(BITMAP);
        // 블록 리스트의 비트맵 크기, 바이트 단위로 올림 처리
        dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
    }
    
    // 사용한 메모리 영역의 크기를 최소 블록 크기로 올림해서 반환
    return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap +
           DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}
```

<hr>

### 비트맵과 할당된 메모리 크기를 저장할 영역의 초기화

```c
// 동적 메모리 영역 초기화
void kInitializeDynamicMemory(void)
{
    QWORD qwDynamicMemorySize;
    int i, j;
    BYTE* pbCurrentBitmapPosition;
    int iBlockCountOfLevel, iMetaBlockCount;
    
    // 동적 메모리 영역으로 사용할 메모리 크기를 이용하여 블록을 관리하는데
    // 필요한 메모리 크기를 최소 블록 단위로 계산
    qwDynamicMemorySize = kCalculateDynamicMemorySize(); // 동적 메모리 영역의 크기 반환 함수
    iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);
    
    // 전체 블록 개수에서 관리에 필요한 메타블록의 개수를 제외한 나머지 영역에 대해서 메타 정보 구성
    gs_stDynamicMemory.iBlockCountOfSmallestBlock = 
        (qwDynamicMemorySize/ DYNAMICMEMORY_MIN_SIZE ) - iMetaBlockCount;
    
    // 최대 몇 개의 블록 리스트로 구성되는지를 계산
    for( i = 0 ; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0 ; i++ )
    {
        // DO Nothing
        ;
    }
    gs_stDynamicMemory.iMaxLevelCount = i;
    
    // 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역을 초기화
    gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
    for( i = 0 ; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++)
    {
        gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
    }
    ... 생략 ...    
}

// 동적 메모리 영역의 크기를 계산
static QWORD kCalculateDynamicMemorySize(void)
{
    QWORD qwRamSize;
    
    // 3GB 이상의 메모리에는 비디오 메모리와 프로세서가 사용하는 레지스터가 존재하므로
    // 최대 3GB 까지만 사용
    qwRamSize = (kGetTotalRamSize() * 1024 * 1024); // kGetTotalRamSize()는 MB단위 반환
    if(qwRamSize > (QWORD)3 * 1024 * 1024 * 1024)
    {
        qwRamSize > (QWORD)3* 1024 * 1024 * 1024;
    }
    return qwRamSize - DYNAMICMEMORY_START_ADDRESS;
}
```

<hr>

#### 초기화 함수의 코드 - 비트맵 초기화

다음은 초기화 함수 중에서 비트맵을 생성하는 부분과 나머지 정보를 설정하는 코드이다.

블록 리스트에 존재하는 블록은 1로 나타내고, 존재하지 않는 부분은 0으로 한다.

```c
// 비트맵의 플래그
#define DYNAMICMEMORY_EXIST 0x01
#define DYNAMICMEMORY_EMPTY 0x00

// 동적 메모리 영역 초기화
void kInitializeDynamicMemory(void)
{
    // 생략
    
    // 비트맵 자료구조의 시작 어드레스 지정
    // DYNAMICMEMORY_START_ADDRESS = 동적 메모리 영역이 시작하는 어드레스.
    //								 스택 풀의 마지막 어드레스 이후에 1MB로 정렬한 어드레스 사용
    gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS+
                        (sizeof(BYTE)*gs_stDynamicMemory.iBlockCountOfSmallestBlock));
    // 실제 비트맵의 어드레스를 지정
    pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) +
        (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);
    // 블록 리스트 별로 루프를 돌면서 비트맵을 생성한다.
    // 초기 상태는 가장 큰 블록과 자투리 블록만 존재하므로 나머지는 비어 있는 것으로 설정
    for(j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++)
    {
        gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
        gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;
        iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;
        
        // 8개 이상 남았으면 상위 블록으로 모두 결합할 수 있으므로, 모두 비어있는 것으로 설정
        for(i = 0; i < iBlockCountOfLevel / 8 ; i++)
        {
            *pbCurrentBitmapPosition = 0x00;
            pbCurrentBitmapPosition++;
        }
        
        // 8로 나누어 떨어지지 않는 나머지 블록들에 대한 처리
        if((iBlockCountOfLevel % 8) != 0)
        {
            *pbCurrentBitmapPosition = 0x00;
            // 남은 블록이 홀수라면 마지막 한 블록은 결합되어 상위 블록으로 이동하지 못한다.
            // 따라서 마지막 블록은 현재 블록 리스트에 존재하는 자투리 블록으로 설정한다.
            i = iBlockCountOfLevel%8;
            if((i%2) == 1)
            {
                *pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i-1));
                gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
            }
            pbCurrentBitmapPosition++;
        }
    }
    
    // 블록 풀의 어드레스와 사용된 메모리 크기 설정
    gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + 
        iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
    gs_stDynamicMemory.qwEndAddress = kCalculateDynamicMemorySize() +
        DYNAMICMEMORY_START_ADDRESS;
    gs_stDynamicMemory.qwUsedSize = 0;
}
```

<hr>

## 메모리 할당 기능 구현

메모리를 할당하는 과정은 요청된 메모리 크기를 버디 블록 크기로 정렬하고,<br>할당된 블록에 대한 정보를 저장하는 부수적인 부분과 실제로 버디 블록 리스트에서 블록을 할당하는 핵심적인 부분으로 나눌 수 있다.

부수적인 부분은 블록을 할당하는 함수 전/후에 존재한다.

#### 메모리 할당 함수 - 버디 블록을 할당받아 실제 메모리 어드레스로 변환

```c
// 메모리 할당
void* kAllocateMemory(QWORD qwSize)
{
	QWORD qwAlignedSize;
    QWORD qwRelativeAddress;
    long lOffset;
    int iSizeArrayOffset;
    int iIndexOfBlockList;
    
    // 메모리 크기를 버디 블록의 크기로 맞춘다.
    qwAlignedSize = kGetBuddyBlockSize(qwSize);
    if(qwAlignedSize == 0)
    {
        return NULL;
    }
    
    // 만약 여유 공간이 충분하지 않으면 실패
    if(gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize +
      qwAlignedSize > gs_stDynamicMemory.qwEndAddress)
    {
        return NULL;
    }
    
    // 버디 블록 할당하고 할당된 블록이 속한 블록 리스트의 인덱스를 반환
    lOffset = kAllocationBuddyBlock(qwAlignedSize);
    if(lOffset == -1)
    {
        return NULL;
    }
    
    iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);
    
    // 블록 크기를 저장하는 영역에 실제로 할당된 버디 블록이 속한 블록 리스트의 인덱스를 저장
    // 메모리를 해제할 때 블록 리스트의 인덱스를 사용
    qwRelativeAddress = qwAlignedSize * lOffset; /* 버디 블록의 크기와 블록 리스트의 오프셋을
    												이용해서 블록 풀을 기준으로 하는 상대적인
    												어드레스를 계산 */
    iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;
    gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 
        (BYTE)iIndexOfBlockList;
    gs_stDynamicMemory.qwUsedSize += qwAlignedSize;
    
    // 블록 풀을 기준으로 하는 어드레스와 블록 풀의 시작 어드레스를 더하여 실제 어드레스를 계산 후 반환
    return (void*)(qwRelatvieAddress + gs_stDynamicMemory.qwStartAddress);   
}

// 가장 가까운 버디 블록의 크기로 정렬된 크기를 반환
static QWORD kGetBuddyBlockSize(QWORD qwSize)
{
    long i;
    
    for(i = 0; i<gs_stDynamicMemory.iMaxLevelCount ; i++)
    {
        if(qwSize <= (DYNAMICMEMORY_MIN_SIZE << i))
        {
            return (DYNAMICMEMORY_MIN_SIZE << i);
        }
    }
    return 0;
}
```

위 코드에서 중요한 부분은 *kAllocateMemory()* 함수 아래쪽에 있는 어드레스 변환 부분과 할당한 블록이 속한 블록 리스트의 인덱스를 저장하는 부분이다.

버디 블록을 할당하는 *kAllocationBuddy()* 함수는 단지 블록 리스트 내의 블록 오프셋만 반환할 뿐이며,<br>할당된 블록 오프셋은 응용프로그램에서 바로 사용할 수 없다.

그러므로 할당받은 블록의 크기와 오프셋을 곱하여 블록 풀을 기준으로 하는 어드레스를 계산한 후, 다시 블록 풀의 시작 어드레스를 더해서 실제 메모리 어드레스로 변환해야 한다.

메모리 블록을 할당한 후에는 꼭 할당한 블록이 속한 블록 리스트의 인덱스를 저장해야 한다.

블록 풀을 기준으로 하는 어드레스를 구했으므로, 해당 어드레스를 버디 블록의 최소 크기로 나누면<br>할당한 크기를 저장한 영역의 오프셋을 구할 수 있다.<br>이 오프셋에 할당된 블록 리스트의 인덱스를 저장해두면 해제할 때 이 값을 참조해서 블록을 해제할 수 있다.

*kGetBuddyBlockSize( )* 함수는 요청한 메모리의 크기를 버디 블록 크기로 정렬한다.<br>이 함수는 요청한 크기를 만족하는 블록 크기를 찾을 때 까지 루프를 수행하는 것이 전부이다.

<hr>

#### 메모리 할당 함수 - 버디 블록을 할당

```c
static int kAllocationBuddyBlock(QWORD qwAlignedSize)
{
    int iBlockListIndex, iFreeOffset;
    int i;
    BOOL bPreviousInterruptFlag;
    
    // 블록 크기를 만족하는 블록 리스트의 인덱스를 검색
    iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
    if(iBlockListIndex == -1)
    {
        return -1;
    }
    
    // 동기화 처리
    bPreviousInterruptFlag = kLockForSystemData();
    
    // 만족하는 블록 리스트부터 최상위 블록 리스트까지 검색하여 블록을 선택
    for(i = iBlockListIncex ; i < gs-stDynamicMemory.iMaxLevelCount ; i++)
    {
        // 블록 리스트의 비트맵을 확인하여 블록이 존재하는지 확인
        iFreeOffset = kFindFreeBlockInBitmap(i);
        if(iFreeOffset != -1)
        {
            break;
        }
    }
    
    // 마지막 블록 리스트까지 검색했는데도 없다면 실패
    if(iFreeOffset = -1)
    {
        kUnlockForSystemData(bPreviousInterruptFlag);
        return -1;
    }
    
    // 블록을 찾았으니 빈 것으로 표시
    kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);
    
    // 상위 블록에서 블록을 찾았다면 상위 블록을 분할
    if(i > iBlockListIndex)
    {
        // 검색된 블록 리스트에서 검색을 시작한 블록 리스트까지 내려가면서 왼쪽 블록은
        // 빈 것으로 표시하고 오른쪽 블록은 존재하는 것으로 표시한다.
        for( i = i - 1 ; i >= iBlockListIndex ; i--)
        {
            // 왼쪽 블록은 빈 것으로 표시
            kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
            // 오른쪽 블록은 존재하는 것으로 표시
            kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);
            // 왼쪽 블록을 다시 분할
            iFreeOffset = iFreeOffset * 2;
        }
    }
    kUnlockForSystemData(bPreviousInterruptFlag);
    
    return iFreeOffset;    
}
```

블록을 할당하는 함수의 핵심은 상위 블록에서 블록을 찾았을 때 블록을 하위 블록으로 분할하는 것이다.

상위 블록에서 하위 블록으로 내려갈수록 블록의 인덱스즈는 2배가 되며,<br>왼쪽과 오른쪽블록은 인덱스가 1만큼 차이가 난다.



