#include "Types.h"
#include "Page.h"
//----------------------------------------------------------------------------------
//  추가
//----------------------------------------------------------------------------------
#include "ModeSwitch.h"

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough( void );
//----------------------------------------------------------------------------------
//  추가
//----------------------------------------------------------------------------------
void kCopyKernel64ImageTo2Mbyte(void);
//----------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////
//
// 잠자는 코어 깨우기
//
////////////////////////////////////////////////////////////////
// Bootstrap Processor 여부가 저장된 어드레스, 부트 로더 영역의 앞쪽에 위치
#define BOOTSTRAPPROCESSOR_FLAGADDRESS  0x7C09
////////////////////////////////////////////////////////////////

//Main
void Main(void)
{
	 DWORD i;
//----------------------------------------------------------------------------------
//  추가
// 
// Minimum Memory Size Check...................[    ]
//----------------------------------------------------------------------------------
    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    char vcVendorString[13] = {0,};

    ////////////////////////////////////////////////////////////////
    //
    // 잠자는 코어 깨우기
    //
    ////////////////////////////////////////////////////////////////
    // Application Processor이면 아래의 코드를 생략하고 바로 64비트 모드로 전환
    if( *( ( BYTE* ) BOOTSTRAPPROCESSOR_FLAGADDRESS ) == 0 )
    {
        kSwitchAndExecute64bitKernel();
        while( 1 ) ;
    }
    
    //==========================================================================
    // BSP만 수행하는 코드
    //==========================================================================  
    ////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------
    
    kPrintString( 0, 3,0xF9, "Protected Mode C Language Kernel Start......[Pass]" );
    
    // 최소 메모리 크기를 만족하는 지 검사
    kPrintString( 0, 4,0x0A, "Minimum Memory Size Check...................[    ]" );
    if( kIsMemoryEnough() == FALSE )
    {
        kPrintString( 45, 4,0x4A, "Fail" );
        kPrintString( 0, 5,0xC1, "Not Enough Memory~!! CYNOS64 OS Requires Over "
                "64Mbyte Memory~!!" );
        while( 1 ) ;
    }
    else
    {
        kPrintString( 45, 4,0xA9, "Pass" );
    }
    
    // IA-32e 모드의 커널 영역을 초기화
    kPrintString( 0, 5,0x0A, "IA-32e Kernel Area Initialize...............[    ]" );
    if( kInitializeKernel64Area() == FALSE )
    {
        kPrintString( 45, 5,0x4A, "Fail" );
        kPrintString( 0, 6,0xC1, "Kernel Area Initialization Fail~!!" );
        while( 1 ) ;
    }
    kPrintString( 45, 5,0xA9, "Pass" );

	// IA-32e 모드 커널을 위한 페이지 테이블 생성
	kPrintString(0,6,0xB9,"IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45,6,0xA9,"Pass");

//----------------------------------------------------------------------------------
//  추가
//----------------------------------------------------------------------------------
    // 프로세서 제조사 정보 읽기
    kReadCPUID( 0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(DWORD*) vcVendorString = dwEBX;
    *((DWORD*)vcVendorString+1) = dwEDX;
    *((DWORD*)vcVendorString+2) = dwECX;
    kPrintString(0,7,0xB9,"Processor Vendor String.....................[             ]");
    kPrintString(45, 7, 0x09, vcVendorString);

    // 64비트 지원 유무 확인
    kReadCPUID(0x80000001, &dwEAX,&dwEBX,&dwECX,&dwEDX);
    kPrintString(0,8,0xB9,"64bit Mode Support Check....................[    ]");
    if(dwEDX & (1 << 29))
    {
        kPrintString(45,8,0x09,"Pass");
    }
    else
    {
        kPrintString(45,8,0x4A,"Fail");
        kPrintString(0,9,0x4E,"This Processor does not support 64bit mode!");
        while(1);
    }

//----------------------------------------------------------------------------------
//  추가
//----------------------------------------------------------------------------------
    // IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스로 이동
    kPrintString(0,9,0xF9,"Copy IA-32e Kernel To 2M Address............[    ]");
    kCopyKernel64ImageTo2Mbyte();
    kPrintString(45,9,0x09,"Pass");
    
    // IA-32e 모드로 전환
    kPrintString(0,10,0xB1,"Switch To IA-32e Mode");
    kSwitchAndExecute64bitKernel();
//----------------------------------------------------------------------------------
    
    while( 1 ) ;
}

// 문자열 출력 함수
void kPrintString( int iX, int iY, BYTE Attr,  const char* pcString)
{
	CHARACTER* pstScreen = ( CHARACTER* ) 0xB8000;
	int i;
	
	pstScreen += ( iY * 80 ) + iX;
	for(i = 0; pcString[ i ] != 0; i++)
	{
		pstScreen[ i ].bCharactor = pcString[ i ];
		pstScreen[ i ].bAttribute = Attr;
	}
}

BOOL kInitializeKernel64Area(void)
{
	DWORD* pdwCurrentAddress;
    
    // 초기화를 시작할 어드레스인 0x100000(1MB)을 설정
    pdwCurrentAddress = ( DWORD* ) 0x100000;
    
    // 마지막 어드레스인 0x600000(6MB)까지 루프를 돌면서 4바이트씩 0으로 채움
    while( ( DWORD ) pdwCurrentAddress < 0x600000 )
    {        
        *pdwCurrentAddress = 0x00;

        // 0으로 저장한 후 다시 읽었을 때 0이 나오지 않으면 해당 어드레스를 
        // 사용하는데 문제가 생긴 것이므로 더이상 진행하지 않고 종료
        if( *pdwCurrentAddress != 0 )
        {
            return FALSE;
        }
        
        // 다음 어드레스로 이동
        pdwCurrentAddress++;
    }
    
    return TRUE;
}

// OS를 실행하기에 충분한 메모리를 가지고 있는지 체크
BOOL kIsMemoryEnough( void )
{
	DWORD* pdwCurrentAddress;
   
    // 0x100000(1MB)부터 검사 시작
    pdwCurrentAddress = ( DWORD* ) 0x100000;
    
    // 0x4000000(64MB)까지 루프를 돌면서 확인
    while( ( DWORD ) pdwCurrentAddress < 0x4000000 )
    {
        *pdwCurrentAddress = 0x12345678;
        
        // 0x12345678로 저장한 후 다시 읽었을 때 0x12345678이 나오지 않으면 
        // 해당 어드레스를 사용하는데 문제가 생긴 것이므로 더이상 진행하지 않고 종료
        if( *pdwCurrentAddress != 0x12345678 )
        {
           return FALSE;
        }
        
        // 1MB씩 이동하면서 확인
        pdwCurrentAddress += ( 0x100000 / 4 );
    }
    return TRUE;
}

//----------------------------------------------------------------------------------
// 추가
//----------------------------------------------------------------------------------
// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스에 복사
void kCopyKernel64ImageTo2Mbyte(void)
{
    WORD wKernel32SectorCount, wTotalKernelSectorCount;
    DWORD* pdwSourceAddress, *pdwDestinationAddress;
    int i;

    // 0x7C05에 총 커널 섹터 수, 0x7C07에 보호 모드 커널 섹터 수가 들어있다.
    wTotalKernelSectorCount = *((WORD*)0x7C05);
    wKernel32SectorCount = *((WORD*)0x7C07);

    pdwSourceAddress = (DWORD*)(0x10000 + (wKernel32SectorCount * 512));
    pdwDestinationAddress = (DWORD*)0x200000;

    // IA-32e 모드 커널 섹터 크기만큼 복사
    for(i = 0; i < 512 * (wTotalKernelSectorCount - wKernel32SectorCount)/4 ; i++)
    {
        *pdwDestinationAddress = *pdwSourceAddress;
        pdwDestinationAddress++;
        pdwSourceAddress++;
    }
}