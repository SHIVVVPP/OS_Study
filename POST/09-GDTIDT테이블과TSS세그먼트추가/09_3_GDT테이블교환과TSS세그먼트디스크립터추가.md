# GDT 테이블 교환과 

# TSS 세그먼트 디스크립터 추가



## GDT 테이블 교환 이유

지금까지 사용한 코드와 데이터 세그먼트 디스크립터는 보호 모드 커널 엔트리 포인트 영역에서 어셈블리어로 작성하였다.

이 영역에 TSS 세그먼트와 TSS 세그먼트 디스크립터를 추가해도 되지만,<br>커널 엔트리 포인트 영역(512바이트)에 비해 104바이트의 TSS 세그먼트 디스크립터가 너무 크다는 문제가 있다.

또한 멀티코어를 활성화하게 되면 코어 별로 TSS 세그먼트가 필요하므로<br> 미리 대비하여 1MB 이상의 공간에 GDT 테이블을 생성하는 것이다.

<hr>



## GDT 테이블 생성과 TSS 세그먼트 디스크립터 추가

GDT는 GDT 테이블과 GDT에 대한 정보를 나타내는 자료구조로 구성되어 있다.

따라서 이 두 가지 자료구조를 정의하고 각 자료구조에 맞는 매크로를 정의해보자.

< 세그먼트 디스크립터, GDT 테이블, TSS 세그먼트를 위한 자료구조와 매크로 >

```c
// 조합에 사용할 기본 매크로
#define GDT_TYPE_CODE			0x0A
#define GDT_TYPE_DATA			0x02
#define GDT_TYPE_TSS			0x09
#define GDT_FLAGS_LOWER_S		0x10
#define GDT_FLAGS_LOWER_DPL0	0x00
#define GDT_FLAGS_LOWER_DPL1	0x20
#define GDT_FLAGS_LOWER_DPL2	0x40
#define GDT_FLAGS_LOWER_DPL3	0x60
#define GDT_FLAGS_LOWER_P		0x80
#define GDT_FLAGS_UPPER_L		0x20
#define GDT_FLAGS_UPPER_DB		0x40
#define GDT_FLAGS_UPPER_G		0x80

// 실제로 사용할 매크로
// Lower Flags는 Code/Data/TSS, DPL0, Present로 설정
#define GDT_FLAGS_LOWER_KERNELCODE 	(GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LLOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS			(GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE	(GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA 	(GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | \
			GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

// Upper flags는 Granulaty로 설정하고 코드 및 데이터는 64비트 추가
#define GDT_FLAGS_UPPER_CODE 	(GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA 	(GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS		(GDT_FLAGS_UPPER_G)

// 1바이트로 정렬
#pragma pack(push,1)

// GDTR 및 IDTR 구조체
typedef struct kGDTRStruct
{
    WORD 	wLimit;
    QWORD 	qwBaseAddress;
    // 16바이트 어드레스 정렬을 위해 추가
    WORD 	wPadding;
    DWORD 	dwPadding;
}GDTR,IDTR;

// 8바이트 크기의 GDT 엔트리 구조
typedef struct kGDTEntry8Struct
{
    WORD	wLowerLimit;
    WORD	wLowerBaseAddress;
    BYTE	bUpperBaseAddress1;
    // 4비트 Type, 1비트 S 2비트 DPL, 1비트 P
    BYTE	bTypeAndLowerFlag;
    // 4비트 Segment Limit, 1비트 AVL, L, D/B, G
    BYTE	bUpperLimitAndUpperFlag;
    BYTE	bUpperBaseAddress2;
}GDTENTRY8;

// 16바이트 크기의 GDT 엔트리 구조
typedef struct kGDTEntry16Struact
{
    WORD 	wLowerLimit;
    WORD	wLowerBaseAddress;
    BYTE	bMiddleBaseAddress1;
    // 4비트 Type, 1비트 0, 2비트 DPL, 1비트 P
    BYTE	bTypeAndLowerFlag;
    // 4비트 Segment Limit, 1bit AVL, 0, 0, G
    BYTE	bUpperLimitAndUpperFlag;
    BYTE	bMiddleBaseAddress2;
    DWORD	dwUpperBaseAddress;
    DWORD	dwReserved;
} GDTENTRY16;

typedef struct kTSSDataStruct
{
    DWORD	dwReserved1;
    QWORD	qwRsp[3];
    QWORD	qwReserved2;
    QWORD	qwIST[7];
    QWORD	qwReserved3;
    WORD	wReserved;
    WORD	wIOMapBaseAddress;
}TSSSEGMENT;

#pragma pack(pop)
```

<hr>



## 디스크립터 생성 함수

디스크립터를 생성하는 함수는 파라미터로 넘어온 각 필드의 값을 세그먼트 디스크립터의 구조에 맞추어 삽입한다.

다음은 GDTENTRY8, GDTENTRY16의 자료구조에 맞추어 값을 삽입하는<br>*kSetGDTEntry8( )*, *kSetGDTEntry16( )* 함수와 이를 사용하여 GDT를 생성하는 코드이다.

널(NULL) 디스크립터와 커널 코드, 커널 데이터, TSS 세그먼트 디스크립터 순으로 생성한다.

```c
// GDT 테이블을 초기화
void kInitializeGDTTableAndTSS(void)
{
    GDTR* pstGDTR;
    GDTENTRY8* pstEntry;
    TSSSEGMENT* pstTSS;
    int i;
    
    //GDTR 설정
    pstGDTR = (GDTR*) 0x142000; // 1MB 어드레스에서 264KB를 페이지 테이블 영역으로 사용하고
    							// 있으므로 그 이후 영역을 GDTR, GDT 테이블, TSS 세그먼트
    							// 영역으로 사용한다.
    pstEntry = (GDTENTRY8*)(0x142000 + sizeof(GDTR));
    pstGDTR->wLimit = (sizeof(GDTENTRY8)*3) + (sizeof(GDTENTRY16)*1) - 1;
    pstGDTR->qwBaseAddress = (QWORD)pstEntry;
    
    //TSS 영역 설정
    pstTSS = (TSSSEGMENT*)((QWORD)pstEntry + GDT_TABLESIZE);
    
    //NULL, 64비트 Code/Data, TSS를 위해 총 4개의 세그먼트를 생성한다.
    kSetGDTEntry8(&(pstEntry[0]),0,0,0,0,0);
    kSetGDTEntry8(&(pstEntry[1]),0,0xFFFFF,GDT_FLAGS_UPPER_CODE,
                 GDT_FLAGS_LOWER_KERNELCODE,GDT_TYPE_CODE);
    kSetGDTEntry8(&(pstEntry[2]),0,0xFFFFF,GDT_FLAGS_UPPER_DATA,
                 GDT_FLAGS_LOWER_KERNELDATA,GDT_TYPE_DATA);
    kSetGDTEntry16((GDTENTRY16*)&(pstEntry[3]),(QWORD)pstTSS,
                  sizeof(TSSSEGMENT)-1,GDT_FLAGS_UPPER_TSS,GDT_FLAGS_LOWER_TSS,
                  GDT_TYPE_TSS);
    
    //TSS 초기화 GDT 이하 영역을 사용한다.
    kInitializeTSSSegment(pstTSS);
    
}

// 8바이트 크기의 GDT 엔트리에 값을 설정
// 코드와 데이터 세그먼트의 디스크립터
voidkSetGDTEntry8(GDTENTRY8* pstEntry, DOWRD dwBaseAddress, DWORD dwLimit,
                 BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType)
{
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
    pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF)| bUpperFlags;
    pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}

// 16바이트 크기의 GDT 엔트리에 값을 설정
// TSS 세그먼트 디스크립터
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
                   BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType)
{
    pstentry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
    pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF)| bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
    pstEntry->dwUpperbaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}
```

위 코드에서 GDTR 자료구조의 시작 어드레스를 0x142000으로 설정하고,<br>그 이후 어드레스 부터 각 자료구조를 위치시킨 이유는 0x100000(1MB) 영역부터 264KB를 페이지 테이블로 사용하기 때문이다.

그래서 그 이후부터 GDT와 TSS 세그먼트를 생성하여 IA-32e 모드에서 사용하도록 하였다.

<1MB ~ 2MB 영역의 메모리 구조>

![image](https://user-images.githubusercontent.com/34773827/61182364-ceef9f80-a66c-11e9-933e-7ef53cbdcdae.png)

<hr>



## TSS 세그먼트 초기화

TSS 세그먼트는 스택에 대한 필드와 I/O 맵에 대한 필드로 구성되어 있다.

우리 OS는 I/O 맵을 사용하지 않으며 스택 스위칭 방식 역시 고전적인 방식이 아닌 IST 방식을 사용하므로<br>이에 맞추어 TSS 세그먼트 필드를 설정해보자.

I/O 맵을 사용하지 않게 설정하는 방법은 간단하다. <br>I/O 맵 기준 주소를 TSS 세그먼트 디스크립터에서 설정한 Limit 필드 값보다 크게 설정하면 된다.

> TSS 세그먼트 디스크립터에서는 TSSSEGMENT 자료구조의 크기 만큼 Limit을 설정 했으므로<br>그보다 큰 0xFFFF를 설정한다.



IST 역시 간단히 설정할 수 있다.<br>IDT 게이트 디스크립터의 IST 필드를 0이 아닌 값으로 설정하고,<br>TSS 세그먼트의 해당 IST 영역에 핸들러가 사용할 스택 어드레스를 설정한다.

> 단 스택 어드레스는 반드시 16바이트로 정렬된 위치에서 시작해야 하므로 이 점만 주의하면 된다.



<TSS 세그먼트를 초기화 하는 코드 >

다음은 I/O맵을 사용하지 않고 7MB ~ 8MB 영역을 IST 1로 사용하도록 TSS 세그먼트를 초기화 하는 코드이다.

```c
void kInitializeTSSSegment(TSSSEGMENT* pstTSS)
{
    kMemSet(pstTSS, 0, sizeof(TSSDATA));
    pstTSS->qwIST[0] = 0x800000;
    pstTSS->wIOMapBaseAddress = 0xFFFF;
}
```

<hr>



## GDT 테이블 교체와 TSS 세그먼트 로드

이제 GDT 테이블을 교체하고 TSS 세그먼트를 프로세서에 로드할 차례이다.

GDT 테이블을 교체하는 방법은 LGDT 명령을 사용하여 GDT 정보를 수정하는 것이다.

> 세그먼트 디스크립터의 오프셋은 기존 테이블과 같으므로 변경할 필요는 없다.



#### TSS 세그먼트를 로드하려면?

x86 프로세서에는 태스크에 관련된 정보를 저장하는 태스크 레지스터(Task Register, TR)가 있다.<br>TR 레지스터는 현재 프로세서가 수행 중인 태스크의 정보를 관리하며,<br>GDT 테이블 내에 TSS 세그먼트 디스크립터의 오프셋이 저장되어 있다.

따라서 LTR 명령어를 사용하여 GDT 테이블 내의  TSS 세그먼트 인덱스인 0x18을 지정함으로써,<br>TSS 세그먼트를 프로세서에 설정할 수 있다.



< LGDT와 LTR 명령어를 수행하는 어셈블리어 함수와 C 함수 선언 코드 >

LGDT, LTR 명령어는 모두 어셈블리어 명령이므로, 이를 수행하는 어셈블리어 함수를 만들어 C코드에서 호출한다.

```assembly
; GDTR 레지스터에 GDT 테이블을 설정
;	PARAM: GDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadGDTR:
	lgdt [rdi]	; 파라미터 1(GDTR의 어드레스)를 프로세서에 로드하여
				; GDT 테이블을 설정
	ret
	
; TR 레지스터에 TSS 세그먼트 디스크립터 설정
;	PARAM: TSS 세그먼트 디스크립터의 오프셋
kLoadTR:
	ltr di		; 파라미터 1(TSS 세그먼트 디스크립터의 오프셋)을 프로세서에
				; 설정하여 TSS 세그먼트를 로드
	ret
```

```c
// C 함수 선언
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
```

<hr>

이것으로 GDT 테이블 교체와 TSS 세그먼트 생성에 대한 모든 준비가 끝난다.

앞서 작성한 함수들을 호출하여 프로세서에 GDT 테이블을 갱신하고 TSS 세그먼트를 프로세서에 로드해보자.

< GDT 테이블을 갱신하고  TSS 세그먼트를 프로세서에 로드하는 코드 >

```c
void Main(void)
{
    ... 생략 ...
        
    kInitializeGDTTableAndTSS();
    kLoadGDTR(0x142000); 	// GDT 테이블의 정보를 나타내는 자료구조(GDTR)의 어드레스
    kLoadTR(0x18);			// GDT 테이블에서 TSS 세그먼트의 오프셋
    
    ... 생략 ...
}
```

