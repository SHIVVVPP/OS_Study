# 멀티코어 프로세서용 OS로 업그레이드

## 멀티코어 프로세서 처리의 핵심, 코어 아이덴티티

멀티코어 프로세서를 처리하는 데 있어서 가장 중요한 것은 현재 코어를 수행하는 코어가 어떤 것인지 아는 것(Identity)이다.

### 코어 인식

코어를 구분하는 기준은 크게 BSP와 AP를 구분하는 것과 개별 코어를 구분하는 것으로 나눌 수 있다.

#### BSP와 AP 구분

BPS와 AP를 구분하는 이유는 부팅할 때 BSP의 역할이 AP의 역할과 다르기 때문이다.<BR>BSP는 부트 로더 이미지를 메모리에 로딩한 후 A20 게이트 활성화를 거쳐 보호모드 전환, 커널 영역 초기화, 페이지 테이블 생성, IA-32e 모드 전환, 각종 디바이스 드라이버 초기화 등의 작업을 한다.<br>하지만, AP는 BSP가 작업을 다 끝낸 상태에서 시작하므로 보호 모드를 거쳐 IA-32e 모드로 전한한 뒤 IA-32e 모드 커널만 실행하면 된다.

BSP와 AP를 구분하는 방법은 비교적 간단한데,<BR>BSP는 가장 먼저 OS 코드를 실행하므로 메모리의 한 영역을 지정하여 BSP임을 기록해두면 된다.<BR>처음에 부팅을 수행하는 코어가 BSP이므로 처음에 이 값은 BSP이다.<BR>그리고 BSP가 부팅을 완료하여 IA-32e 모드 커널로 진입하면, BSP라는 것을 저장하는 메모리를 변경하여 AP로 설정한다. 그후 BSP가 AP를 깨우면 AP가 OS를 실행할 때는 이 값이 AP로 설정되어 있으므로 BSP와 AP를 쉽게 구분할 수 있다.

다른 방법으로는 IA32_APIC_BASE MSR 레지스터를 읽는 것이다.<BR>IA32_APIC_BASE MSR 레지스터의 BSP 필드는 현재 프로세서 또는 코어가 BSP인지를 나타내는 필드이므로,<BR>이 필드를 참고하여 작업을 진행할 수도 있다.

#### 개별 코어 구분

BSP와 AP를 구분하는 것보다 더 중요한 문제는 개별 코어를 인식하는 것이다.<BR>BSP와 AP를 구분하는 것은 부팅하는 동안만 유효하며, 부팅된 이후에는 BSP와 AP의 구분 없이 모두 동등하게 작업을 수행한다.

따라서 각 코어를 정확하게 식별할 수 있는 것이 필요하다.<BR>이때 사용되는 것이 바로 로컬 APIC ID이다.

로컬 APIC ID는 로컬 APIC ID 레지스터에 저장되어 있으며, 로컬 APIC 사이에서 유일한 값이다.<BR>로컬 APIC는 코어마다 하나 씩 존재하므로 충분히 코어를 구분하는 ID로 사용할 수 있다.<BR>또한 로컬 APIC ID는 0에서 시작하는 연속된 값이므로, 자료구조를 할당할 때 인덱스로도 쓸 수 있다.

<HR>

## 부트 로더 업그레이드

부트 로더는 OS 이미지를 디스크에서 로딩한 뒤 보호 모드 커널로 이동시킨다.<BR.사실 OS 이미지를 로딩하는 작업은 최초 한 번만 하면 되므로 부트 로더 코드는 BSP를 위한 코드라 할 수 있다.

비록 부트 로더에 AP를 위한 코드는 없지만, 대신 AP를 위한 데이터가 있다.

우리 OS는 BSP 여부를 저장하는 플래그 영역으로 부트로더의 앞부분을 사용하기 때문이다. <BR>부트 로더의 앞부분에 BSP 플래그를 저장하는 이유는 어드레스를 쉽게 계산할 수 있기 때문이다.<BR>BSP 플래그는 보호 모드 커널과 IA-32e 모드 커널에서 참조해야 하므로,<BR>어드레스를 쉽게 계산할 수 있으며 다른 것에 영향을 받지 않는 안전한 영역이 좋다.<BR>부트 로더가 위치하는 0x7C00 ~ 0x7E00 영역은 BIOS가 부트 로더를 위해 예약한 영역이어서 안전하며 어드레스도 계산하기 쉬우므로 안성맞춤이다.

부트 로더 코드에서 BSP 플래그 필드는 보호 모드 커널의 총 섹터 수의 뒤쪽에 위치하며 1바이트이다.<BR>이 플래그의 값이 1이면 BSP를 뜻하면 0이면 AP를 뜻한다.  부팅 과정은 BSP가 진행하므로 OS 이미지에는 1로 저장되고 BSP가 부팅을 완료하여 IA-32e 모드로 진입하면 0으로 변경한다.<BR>BSP 필드가 0으로 변경된 뒤에 AP가 코드를 실행할 때는 이 필드를 참조하여 AP용 코드를 실행한다.

#### BSP 플래그가 추가된 부트로더 코드

```ASSEMBLY
[ORG 0x00]          ; 코드의 시작 어드레스를 0x00으로 설정
[BITS 16]           ; 이하의 코드는 16비트 코드로 설정

SECTION .text       ; text 섹션(세그먼트)을 정의

jmp 0x07C0:START    ; CS 세그먼트 레지스터에 0x07C0을 복사하면서, START 레이블로 이동

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	MINT64 OS에 관련된 환경 설정 값
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TOTALSECTORCOUNT:   dw  0x02    ; 부트 로더를 제외한 MINT64 OS 이미지의 크기
                                ; 최대 1152 섹터(0x90000byte)까지 가능
KERNEL32SECTORCOUNT: dw 0x02    ; 보호 모드 커널의 총 섹터 수
BOOTSTRAPPROCESSOR: db 0x01     ; Bootstrap Processor인지 여부

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
    mov ax, 0x07C0  ; 부트 로더의 시작 어드레스(0x7C00)를 세그먼트 레지스터 값으로 변환
    mov ds, ax      ; DS 세그먼트 레지스터에 설정
    mov ax, 0xB800  ; 비디오 메모리의 시작 어드레스(0x7C00)를 세그먼트 레지스터 값으로 변환
    mov es, ax      ; ES 세그먼트 레지스터에 설정

... 생략 ...
```

BOOTSTRAPPROCESSOR 변수의 어드레스는 앞부분에 위치하느 ㄴ코드와 변수 크기의 합과 같다.

앞 부분에 위치한 코드와 변수는 (어셈블리 명령어 5바이트) + (2바이트 변수 * 2) = 9바이트이므로,<BR>BSP 필드의 실제 어드레스는 0x7C09가 된다.

<hr>

## 보호 모드 커널 업그레이드

보호 모드 커널은 AP가 코드를 수행하는 시작 위치이자, BSP와 AP가 본격적으로 나뉘는 영역이다.

보호 모드 커널은 프로세서를 리얼 모드에서 보호 모드로 전환시키는 것 외에 IA-32e 모드 커널 영역 초기화, 페이지 테이블 생성, IA-32e 모드 커널 이미지 복사 등의 작업을 수행한다.

이중에서 IA-32e 모드 커널 영역 초기화나 페이지 테이블 생성과 같은 작업은 BSP에만 해당하므로,<BR>BSP와 AP로 실행 경로를 구분해야 한다.

다음 그림은 보호 모드 커널의 작업 수행 순서와 BSP와 AP별 코드 실행경로를 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62600324-85236d80-b929-11e9-9722-c900945b6190.png)

#### AP가 코드를 수행할 때 특정 코드 영역을 제외하는 코드

```ASSEMBLY
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 리얼 모드용 코드
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
mov ax, 0x0000              ; Application Processor 플래그를 확인하려고
mov es, ax                  ; ES 세그먼트 레지스터 의 시작 어드레스를 0으로 설정

cmp byte [ es: 0x7C09 ], 0x00       ; 플래그가 0이면 Application Processor이므로
je .APPLICATIONPROCESSORSTARTPOINT  ; Application Processor용 코드로 이동

	... AP 실행 시 제외할 코드 ...

.APPLICATIONPROCESSORSTARTPOINT:
    
    ... BSP와 AP가 공통으로 실행할 코드 ...
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 보호 모드 또는 IA-32e 모드용 코드 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;    
 ; Application Processor이면 아래의 과정을 모두 뛰어넘어서 C 언어 커널 엔트리
    ; 포인트로 이동
    cmp byte [ 0x7C09 ], 0x00
    je .APPLICATIONPROCESSORSTARTPOINT    
    
	... AP 실행 시 제외할 코드 ...

.APPLICATIONPROCESSORSTARTPOINT:
    
    ... BSP와 AP가 공통으로 실행할 코드 ...
    
```

#### AP 처리가 추가된 보호 모드 커널 엔트리 포인트의 코드

```ASSEMBLY
[ORG 0x00]          ; 코드의 시작 어드레스를 0x00으로 설정
[BITS 16]           ; 이하의 코드는 16비트 코드로 설정

SECTION .text       ; text 섹션(세그먼트)을 정의

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
    mov ax, 0x1000  ; 보호 모드 엔트리 포인트의 시작 어드레스(0x10000)를 
                    ; 세그먼트 레지스터 값으로 변환
    mov ds, ax      ; DS 세그먼트 레지스터에 설정
    mov es, ax      ; ES 세그먼트 레지스터에 설정
    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Application Processor 이면 아래의 과정을 모두 뛰어넘어서 보호 모드 커널로
    ; 이동
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov ax, 0x0000              ; Application Processor 플래그를 확인하려고
    mov es, ax                  ; ES 세그먼트 레지스터 의 시작 어드레스를 0으로 설정
    
    cmp byte [ es: 0x7C09 ], 0x00       ; 플래그가 0이면 Application Processor이므로
    je .APPLICATIONPROCESSORSTARTPOINT  ; Application Processor용 코드로 이동
    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Bootstrap Processor만 실행하는 부분
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; A20 게이트를 활성화
    ; BIOS를 이용한 전환이 실패했을 때 시스템 컨트롤 포트로 전환 시도
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; BIOS 서비스를 사용해서 A20 게이트를 활성화
    mov ax, 0x2401          ; A20 게이트 활성화 서비스 설정
    int 0x15                ; BIOS 인터럽트 서비스 호출

    jc .A20GATEERROR        ; A20 게이트 활성화가 성공했는지 확인
    jmp .A20GATESUCCESS

.A20GATEERROR:
    ; 에러 발생 시, 시스템 컨트롤 포트로 전환 시도
    in al, 0x92     ; 시스템 컨트롤 포트(0x92)에서 1 바이트를 읽어 AL 레지스터에 저장
    or al, 0x02     ; 읽은 값에 A20 게이트 비트(비트 1)를 1로 설정
    and al, 0xFE    ; 시스템 리셋 방지를 위해 0xFE와 AND 연산하여 비트 0를 0으로 설정
    out 0x92, al    ; 시스템 컨트롤 포트(0x92)에 변경된 값을 1 바이트 설정
    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Bootstrap Processor와 Application Processor가 공통으로 수행하는 부분
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.A20GATESUCCESS:
.APPLICATIONPROCESSORSTARTPOINT:
    cli             ; 인터럽트가 발생하지 못하도록 설정
    lgdt [ GDTR ]   ; GDTR 자료구조를 프로세서에 설정하여 GDT 테이블을 로드

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; 보호모드로 진입
    ; Disable Paging, Disable Cache, Internal FPU, Disable Align Check, 
    ; Enable ProtectedMode
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov eax, 0x4000003B ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
    mov cr0, eax        ; CR0 컨트롤 레지스터에 위에서 저장한 플래그를 설정하여 
                        ; 보호 모드로 전환

    ; 커널 코드 세그먼트를 0x00을 기준으로 하는 것으로 교체하고 EIP의 값을 0x00을 기준으로 재설정
    ; CS 세그먼트 셀렉터 : EIP
    jmp dword 0x18: ( PROTECTEDMODE - $$ + 0x10000 )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; 보호 모드로 진입
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]               ; 이하의 코드는 32비트 코드로 설정
PROTECTEDMODE:
    mov ax, 0x20        ; 보호 모드 커널용 데이터 세그먼트 디스크립터를 AX 레지스터에 저장
    mov ds, ax          ; DS 세그먼트 셀렉터에 설정
    mov es, ax          ; ES 세그먼트 셀렉터에 설정
    mov fs, ax          ; FS 세그먼트 셀렉터에 설정
    mov gs, ax          ; GS 세그먼트 셀렉터에 설정
    
    ; 스택을 0x00000000~0x0000FFFF 영역에 64KB 크기로 생성
    mov ss, ax          ; SS 세그먼트 셀렉터에 설정
    mov esp, 0xFFFE     ; ESP 레지스터의 어드레스를 0xFFFE로 설정
    mov ebp, 0xFFFE     ; EBP 레지스터의 어드레스를 0xFFFE로 설정

    ; Application Processor이면 아래의 과정을 모두 뛰어넘어서 C 언어 커널 엔트리
    ; 포인트로 이동
    cmp byte [ 0x7C09 ], 0x00
    je .APPLICATIONPROCESSORSTARTPOINT    
    
    ; 화면에 보호 모드로 전환되었다는 메시지를 표시
    push ( SWITCHSUCCESSMESSAGE - $$ + 0x10000 )    ; 출력할 메시지의 어드레스를 스택에 삽입
    push 2                                          ; 화면 Y 좌표(2)를 스택에 삽입
    push 0                                          ; 화면 X 좌표(0)를 스택에 삽입
    call PRINTMESSAGE                               ; PRINTMESSAGE 함수 호출
    add esp, 12                                     ; 삽입한 파라미터 제거

.APPLICATIONPROCESSORSTARTPOINT:
    jmp dword 0x18: 0x10200 ; C 언어 커널이 존재하는 0x10200 어드레스로 이동하여 C 언어 커널 수행


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   함수 코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

... 생략 ...
```

#### AP 처리가 추가된 보호 모드 C 언어 커널 엔트리 포인트

```C
... 생략 ...
    
// Bootstrap Processor 여부가 저장된 어드레스, 부트 로더 영역의 앞쪽에 위치
#define BOOTSTRAPPROCESSOR_FLAGADDRESS  0x7C09

/**
 *  아래 함수는 C 언어 커널의 시작 부분임
 *      반드시 다른 함수들 보다 가장 앞쪽에 존재해야 함
 */
void Main( void )
{
    DWORD i;
    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[ 13 ] = { 0, };
    
    // Application Processor이면 아래의 코드를 생략하고 바로 64비트 모드로 전환
    if( *( ( BYTE* ) BOOTSTRAPPROCESSOR_FLAGADDRESS ) == 0 )
    {
        kSwitchAndExecute64bitKernel();
        while( 1 ) ;
    }
    
    //==========================================================================
    // BSP만 수행하는 코드
    //==========================================================================    
    kPrintString( 0, 3, "Protected Mode C Language Kernel Start......[Pass]" );
    
    // 최소 메모리 크기를 만족하는 지 검사
    kPrintString( 0, 4, "Minimum Memory Size Check...................[    ]" );
    if( kIsMemoryEnough() == FALSE )
    {
        kPrintString( 45, 4, "Fail" );
        kPrintString( 0, 5, "Not Enough Memory~!! MINT64 OS Requires Over "
                "64Mbyte Memory~!!" );
        while( 1 ) ;
    }
    else
    {
        kPrintString( 45, 4, "Pass" );
    }
    
    ... IA-32e 모드 커널 영역 초기화와 페이지 테이블 생성, 커널 이미지 이동 등...
    ... 기능 수행 ...
    ... 생략 ...
        
    // IA-32e 모드로 전환
    kPrintString( 0, 10, "Switch To IA-32e Mode" );
    kSwitchAndExecute64bitKernel();
    
    while( 1 ) ;
}
```

<hr>

## IA-32e 모드 커널 업그레이드

### AP를 위한 자료구조 생성

AP가 BSP와 같은 수준으로 동작하려면 자료구조 역시 같은 수준으로 생성해주어야 한다.<BR>일단 코어 별로 인터럽트와 코드가 동시에 실행되므로 태스크 상태 세그먼트, 스택, IST가 반드시 별도로 생성되어야 한다.<BR>그리고 멀티태스킹을 수행하려면 스케줄러와 동기화 객체도 필요하다.

우리의 목표는 최대 16개의 코어를 동시에 지원하는 것이다.<BR>그러므로 위에서 설명한 자료구조 역시 모두 16쌍이 필요하다.<BR>이를 대비하여 OS 설계 초기에 디스크립터 영역을 512KB 이상 할당하고 스택 IST 영역을 1MB로 할당해 둔 것이다

TSS 세그먼트는 태스크의 콘텍스트를 저장하는 세그먼트이다.<BR>IA-32e 모드가 추가되면서 TSS 세그먼트의 역할은 권한(Privilege)별 스택 어드레스와 IST, I/O 비트마스크를 저장하는 정도로 역할이 축소되긴 하지만, 여전히 중요한 위치를 차지하고 있다.<BR>바로 인터럽트 처리를 할 때 참조하는 IST 필드 때문이다.<BR>우리 OS는 인터럽트를 처리할 때 IST를 사용하며 IST는 스택의 일종이므로 코어끼리 공유할 수 없다.<BR>따라서 코어별로 IST를 분리해야 하며 이를 위해서는 TSS 세그먼트도 별도로 생성할 수 밖에 없다.

#### 16개의 TSS 디스크립터를 추가하고 TSS 세그먼트를 설정하도록 수정된<BR>디스크립터 초기화 코드

```C
// 지원 가능한 최대 프로세서 또는 코어의 개수
#define MAXPROCESSORCOUNT                   16

// 기타 GDT에 관련된 매크로
// GDTR의 시작 어드레스, 1Mbyte에서 264Kbyte까지는 페이지 테이블 영역
#define GDTR_STARTADDRESS   0x142000
// 8바이트 엔트리의 개수, 널 디스크립터/커널 코드/커널 데이터
#define GDT_MAXENTRY8COUNT  3
// 16바이트 엔트리의 개수, 즉 TSS는 프로세서 또는 코어의 최대 개수만큼 생성
#define GDT_MAXENTRY16COUNT ( MAXPROCESSORCOUNT )
// GDT 테이블의 크기
#define GDT_TABLESIZE       ( ( sizeof( GDTENTRY8 ) * GDT_MAXENTRY8COUNT ) + \
        ( sizeof( GDTENTRY16 ) * GDT_MAXENTRY16COUNT ) )
// TSS 세그먼트의 전체 크기
#define TSS_SEGMENTSIZE     ( sizeof( TSSSEGMENT ) * MAXPROCESSORCOUNT )

/**
 *  GDT 테이블을 초기화
 */
void kInitializeGDTTableAndTSS( void )
{
    GDTR* pstGDTR;
    GDTENTRY8* pstEntry;
    TSSSEGMENT* pstTSS;
    int i;
    
    // GDTR 설정
    pstGDTR = ( GDTR* ) GDTR_STARTADDRESS;
    pstEntry = ( GDTENTRY8* ) ( GDTR_STARTADDRESS + sizeof( GDTR ) );
    pstGDTR->wLimit = GDT_TABLESIZE - 1;
    pstGDTR->qwBaseAddress = ( QWORD ) pstEntry;
    
    // TSS 세그먼트 영역 설정, GDT 테이블의 뒤쪽에 위치
    pstTSS = ( TSSSEGMENT* ) ( ( QWORD ) pstEntry + GDT_TABLESIZE );

    // NULL, 64비트 Code/Data, TSS를 위해 총 3 + 16개의 세그먼트를 생성
    kSetGDTEntry8( &( pstEntry[ 0 ] ), 0, 0, 0, 0, 0 );
    kSetGDTEntry8( &( pstEntry[ 1 ] ), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, 
            GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE );
    kSetGDTEntry8( &( pstEntry[ 2 ] ), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA,
            GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA );
    
    // 16개 코어 지원을 위해 16개의 TSS 디스크립터를 생성
    for( i = 0 ; i < MAXPROCESSORCOUNT ; i++ )
    {
        // TSS는 16바이트이므로, kSetGDTEntry16() 함수 사용
        // pstEntry는 8바이트이므로 2개를 합쳐서 하나로 사용
        kSetGDTEntry16( ( GDTENTRY16* ) &( pstEntry[ GDT_MAXENTRY8COUNT + 
                ( i * 2 ) ] ), ( QWORD ) pstTSS + ( i * sizeof( TSSSEGMENT ) ), 
                sizeof( TSSSEGMENT ) - 1, GDT_FLAGS_UPPER_TSS, 
                GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS ); 
    }
    
    // TSS 초기화, GDT 이하 영역을 사용함
    kInitializeTSSSegment( pstTSS );    
}
```

![image](https://user-images.githubusercontent.com/34773827/62601322-0d0a7700-b92c-11e9-9a46-134b91066e41.png)

<HR>

### TSS 세그먼트 생성

TSS 세그먼트 디스크립터를 할당했으니 이제 TSS 세그먼트를 생성해야한다.<BR>TSS 세그먼트는 GDT 테이블의 끝부분, 즉 마지막 TSS 디스크립터에 이어서 시작한다.

TSS 세그먼트를 설정할 때 중요한 부분은 IST를 설정하는 부분이다.<BR>인터럽트 처리에 사용하는 스택 영역인 IST는 0x700000 ~ 0x7FFFFF(7~8MB)로 할당되어 있으므로,<BR>이 영역을 16등분하여 TSS 세그먼트에 설정해야 한다.

IST 영역의 크기는 1MB이므로 이를 우리 OS가 지원하는 최대 코어 개수인 16으로 나누면 64KB가 된다.

따라서 TSS 세그먼트의 IST 0 필드에 IST의 끝부분인 0x7FFFFF부터 TSS 디스크립터의 인덱스에 따라 64KB씩 감소시키면서 할당하면 IST 설정을 완료할 후 있다.

이러한 방식은 스택에도 똑같이 적용할 수 있으며 0x600000 ~ 0x6FFFFF(6~7MB)이다.

![image](https://user-images.githubusercontent.com/34773827/62601553-9752db00-b92c-11e9-8e0f-6cbb1e5c3701.png)

#### 16개 TSS 세그먼트를 설정하도록 수정된 TSS 세그먼트 설정 코드

```C
// IST의 시작 어드레스
#define IST_STARTADDRESS        0x700000
// IST의 크기
#define IST_SIZE                0x100000

/**
 *  TSS 세그먼트의 정보를 초기화
 */
void kInitializeTSSSegment( TSSSEGMENT* pstTSS )
{
    int i;
    
    // 최대 프로세서 또는 코어의 수만큼 루프를 돌면서 생성
    for( i = 0 ; i < MAXPROCESSORCOUNT ; i++ )
    {
        // 0으로 초기화
        kMemSet( pstTSS, 0, sizeof( TSSSEGMENT ) );

        // IST의 뒤에서부터 잘라서 할당함. (주의, IST는 16바이트 단위로 정렬해야 함)
        pstTSS->qwIST[ 0 ] = IST_STARTADDRESS + IST_SIZE - 
            ( IST_SIZE / MAXPROCESSORCOUNT * i );
        
        // IO Map의 기준 주소를 TSS 디스크립터의 Limit 필드보다 크게 설정함으로써 
        // IO Map을 사용하지 않도록 함
        pstTSS->wIOMapBaseAddress = 0xFFFF;

        // 다음 엔트리로 이동
        pstTSS++;
    }
}
```

<HR>

### 스택 할당

IA-32e 모드 커널 엔트리 포인트에서 스택을 할당하는 것 자체는 어렵지 않다.

스택의 최대값부터 아래로 64KB씩 감소시키면서 코어의 인덱스에 따라 순차적으로 할당하는 것이 전부이다.

코어의 인덱스는 로컬 APIC ID이다.<BR>로컬 APIC는 코어마다 하나씩 존재하고 ID는 0부터 값이 증가한다. 만일 프로세서의 코어가 16개라면 BSP는 로컬 APIC ID가 0이 되고 나머지 AP는 로컬 APIC ID가 1 ~ 15가 된다.

```ASSEMBLY
[BITS 64]           ; 이하의 코드는 64비트 코드로 설정

SECTION .text       ; text 섹션(세그먼트)을 정의

; 외부에서 정의된 함수를 쓸 수 있도록 선언함(Import)
extern Main
; APIC ID 레지스터의 어드레스와 깨어난 코어의 개수
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
    mov ax, 0x10        ; IA-32e 모드 커널용 데이터 세그먼트 디스크립터를 AX 레지스터에 저장
    mov ds, ax          ; DS 세그먼트 셀렉터에 설정
    mov es, ax          ; ES 세그먼트 셀렉터에 설정
    mov fs, ax          ; FS 세그먼트 셀렉터에 설정
    mov gs, ax          ; GS 세그먼트 셀렉터에 설정

    ; 스택을 0x600000~0x6FFFFF 영역에 1MB 크기로 생성
    mov ss, ax          ; SS 세그먼트 셀렉터에 설정
    mov rsp, 0x6FFFF8   ; RSP 레지스터의 어드레스를 0x6FFFF8로 설정
    mov rbp, 0x6FFFF8   ; RBP 레지스터의 어드레스를 0x6FFFF8로 설정
    
    ; 부트 로더 영역의 Bootstrap Processor 플래그를 확인하여, Bootstrap Processor이면
    ; 바로 Main 함수로 이동
    cmp byte [ 0x7C09 ], 0x01
    je .BOOTSTRAPPROCESSORSTARTPOINT
    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Application Processor만 실행하는 영역
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; 스택의 꼭대기(Top)는 APIC ID를 이용해서 0x700000 이하로 이동
    ; 최대 16개 코어까지 지원 가능하므로 스택 영역인 1M를 16으로 나눈 값인 
    ; 64Kbyte(0x10000)만큼씩 아래로 이동하면서 설정 
    ; 로컬 APIC의 APIC ID 레지스터에서 ID를 추출. ID는 Bit 24~31에 위치함
    mov rax, 0                              ; RAX 레지스터 초기화
    mov rbx, qword [ g_qwAPICIDAddress ]    ; APIC ID 레지스터의 어드레스를 읽음
    mov eax, dword [ rbx ] ; APIC ID 레지스터에서 APIC ID를 읽음(비트 24~31)
    shr rax, 24            ; 비트 24~31에 존재하는 APIC ID를 시프트 시켜서 비트 0~7로 이동
    
    ; 추출한 APIC ID에 64Kbyte(0x10000)을 곱하여 스택의 꼭대기를 이동시킬 거리를 계산
    mov rbx, 0x10000       ; RBX 레지스터에 스택의 크기(64Kbyte)를 저장
    mul rbx                ; RAX 레지스터에 저장된 APIC ID와 RBX 레지스터의 스택 값을 곱함
    
    sub rsp, rax   ; RSP와 RBP 레지스터에서 RAX 레지스터에 저장된 값(스택의 꼭대기를
    sub rbp, rax   ; 이동시킬 거리)을 빼서 각 코어 별 스택을 할당해줌

    ; 깨어난 Application Processor 수를 1 증가시킴. lock 명령어를 사용하여 변수에
    ; 배타적(Exclusive) 접근이 가능하도록 함
    lock inc dword [ g_iWakeUpApplicationProcessorCount ]
    
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Bootstrap Processor와 Application Processor가 공통으로 실행하는 영역
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.BOOTSTRAPPROCESSORSTARTPOINT:
    call Main           ; C 언어 엔트리 포인트 함수(Main) 호출
    
    jmp $
```

위의 코드를 보면 AP에 스택을 할당한 후 g_iWakeUpApplicationProcessorCount 변수를 1 증가시킨다.<br>이 변수를 1 증가시키는 이유는 AP가 정상적으로 커널에 진입했는지 확인하기 위함이다.

실제 kWakeUpApplicationProcessor( ) 코드를 보면 AP를 깨우는 과정에서<BR>초기화 IPI > 시작 IPI > 시작 IPI을 보낸 후 ICR 레지스터의 전달 상태 필드를 통해 전달 여부를 확인한다.

하지만 이는 IPI 메시지의 전달 상태를 의미할 뿐 코어가 정상적으로 깨어나서 커널에 진입했는지는 정확히 알 수 없다. 그래서 변수를 하나 할당하여 AP가 IA-32e 커널로 진입할 때마다 1씩 증가시키고 BSP는 이를 확인하는 방법으로 한 번 더 검사하는 것이다.

g_iWakeUpApplicationProcessorCount를 1증가시키는 코드를 유심히 보면 lock이라는 명령을 사용하고 있다. lock 명령어는 lock 이후에 오는 명령어를 수행할 때  #LOCK 신호를 어설트하여 메모리에 배타적으로 접근할 수 있게 한다. 다시 말해 여러 코어가 동시에 메모리에 접근하는 것을 막고, 명령어를 수행하는 시점에 해당 코어만 메모리에 접근하는 것을 보장한다.

프로세서에 내장된 코어 수가 3개 이상이라면 g_iWakeUpApplicationProcessorCount 변수는 최소 2개 이상의 AP가 동시에 접근할 수 있다. 위의 코드를 보면 변수를 1증가시키는 작업을 inc 명령어로 처리하고 있어서 마치 한 번에 처리되는 것 처럼 보이지만, 메모리에 있는 값을 증가시켜야 하므로, 실제로는 값을 읽은 후 1증가시켜 다시 쓰는 3단계를 거친다.

AP가 두 개 이상이면 이러한 작업을 동시에 수행할 수 있으므로, 동기화 문제가 생길 수 있다.<BR>lock 명령어를 사용하면 inc 명령어를 수행하는 동안 다른 코어가 메모리에 접근하는 것을 막아주므로 이러한 문제를 피할 수 있다.

<hr>

## 코드 수행 경로 설정

BSP는 IA-32e 모드 커널에 진입하면 GDT 테이블과 IDT 테이블, TSS 세그먼트를 생성하고, 생성한 자료구조를 코어에 설정한다. 그리고 각종 디바이스 드라이버를 초기화 한 뒤 콘솔 셸과 같은 태스크를 실행하여 본격적인 작업을 수행한다.

AP는 자료구조가 생성되고 디바이스 드라이버가 초기화된 뒤에 시작하므로, IA-32e 모드로 전환하는 코드와 자료구조를 설정하는 코드만 실행 하면 된다.

![image](https://user-images.githubusercontent.com/34773827/62602458-bfdbd480-b92e-11e9-91e5-152d79d06425.png)

다음 그림은 기능적인 단위로 코드 실행 경로를 구분한 것이다.

#### AP 처리가 추가된 BSP용 IA-32e 모드 C 언어 커널 엔트리 포인트 함수의 코드

```c
#define BOOTSTRAPPPROCESSOR_FLAGADDRESS 0x7C09

/**
 *  Bootstrap Processor용 C 언어 커널 엔트리 포인트
 *      아래 함수는 C 언어 커널의 시작 부분임
 */
void Main( void )
{
    int iCursorX, iCursorY;
    
    // 부트 로더에 있는 BSP 플래그를 읽어서 Application Processor이면 
    // 해당 코어용 초기화 함수로 이동
    if( *( ( BYTE* ) BOOTSTRAPPROCESSOR_FLAGADDRESS ) == 0 )
    {
        MainForApplicationProcessor();
    }
    
    // Bootstrap Processor가 부팅을 완료했으므로, 0x7C09에 있는 Bootstrap Processor를
    // 나타내는 플래그를 0으로 설정하여 Application Processor용으로 코드 실행 경로를 변경
    *( ( BYTE* ) BOOTSTRAPPROCESSOR_FLAGADDRESS ) = 0;

    // 콘솔을 먼저 초기화한 후, 다음 작업을 수행
    kInitializeConsole( 0, 10 );    
    kPrintf( "Switch To IA-32e Mode Success~!!\n" );
    kPrintf( "IA-32e C Language Kernel Start..............[Pass]\n" );
    kPrintf( "Initialize Console..........................[Pass]\n" );
    
    ... 각종 디바이스 드라이버 초기화 ...

    // 유휴 태스크를 시스템 스레드로 생성하고 셸을 시작
    kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, 
            ( QWORD ) kIdleTask );
    kStartConsoleShell();
}
```

#### AP용 IA-32e 모드 C 언어 커널 엔트리 포인트 함수의 코드

코어에 따라 별도로 설정하는 자료구조는 코어의 콘텍스트와 밀접한 관련이 있는 것이며,<BR>스택과 IST가 저장된 TSS 세그먼트가 여기에 해당된다.

스택 영역은 커널 엔트리 포인트에서 이미 처리 했으므로 AP용 C 언어 커널 엔트리 포인트에서는 TSS세그먼트만 설정해주면 된다.

```C
/**
 *  Application Processor용 C 언어 커널 엔트리 포인트
 *      대부분의 자료구조는 Bootstrap Processor가 생성해 놓았으므로 코어에 설정하는
 *      작업만 함
 */
void MainForApplicationProcessor( void )
{
    QWORD qwTickCount;

    // GDT 테이블을 설정
    kLoadGDTR( GDTR_STARTADDRESS );

    // TSS 디스크립터를 설정. TSS 세그먼트와 디스크립터를 Application Processor의 
    // 수만큼 생성했으므로, APIC ID를 이용하여 TSS 디스크립터를 할당
    kLoadTR( GDT_TSSSEGMENT + ( kGetAPICID() * sizeof( GDTENTRY16 ) ) );

    // IDT 테이블을 설정
    kLoadIDTR( IDTR_STARTADDRESS );

    // 1초마다 한번씩 메시지를 출력
    qwTickCount = kGetTickCount();
    while( 1 )
    {
        if( kGetTickCount() - qwTickCount > 1000 )
        {
            qwTickCount = kGetTickCount();
            
            kPrintf( "Application Processor[APIC ID: %d] Is Activated\n",
                    kGetAPICID() );
        }
    }
}

/**
 *  APIC ID 레지스터에서 APIC ID를 반환
 */
BYTE kGetAPICID( void )
{
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    QWORD qwLocalAPICBaseAddress;

    // APIC ID 어드레스의 값이 설정되지 않았으면, MP 설정 테이블에서 값을 읽어서 설정
    if( g_qwAPICIDAddress == 0 )
    {
        // MP 설정 테이블 헤더에 저장된 로컬 APIC의 메모리 맵 I/O 어드레스를 사용
        pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
        if( pstMPHeader == NULL )
        {
            return 0;
        }
        
        // APIC ID 레지스터의 어드레스((0xFEE00020)를 저장하여, 자신의 APIC ID를
        // 읽을 수 있게 함
        qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
        g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
    }
    
    // APIC ID 레지스터의 Bit 24~31의 값을 반환    
    return *( ( DWORD* ) g_qwAPICIDAddress ) >> 24;
}
```

<hr>

