#include "Types.h"

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough( void );

//Main
void Main(void)
{
	DWORD i;
	kPrintString( 0, 3, 0xE9, "C Language Kernel Start ....................[PASS]");
	
	// 최소 메모리 크기를 만족하는 지 검사
	kPrintString(0,4,0x4A,"Minimum Memory Size Check ..................[    ]");
	if(kIsMemoryEnough() == FALSE)
	{
		kPrintString( 45, 4, 0x0A, "Fail");
		kPrintString(0,5,0x0C,"Not Enourgh Memory~!! CYNOS64 OS Requires Over 64MB Memory!");
		while(1);
	}
	else
	{
		kPrintString(45,4,0x0A,"Pass");
	}

	// IA-32e 모드 커널 영역을 초기화
	kPrintString(0,5,0x4A,"IA-32e Kernel Area Initialize...................[    ]");
	if(kInitializeKernel64Area())
	{
		kPrintString(45, 5,0x0C, "Fail");
		kPrintString(0,6,0xC1,"Kernel Area Initialization Fail!");
		while(1);
	} 
	kPrintString(45, 5, 0x0A,"Pass");
	
	while(1);
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
	
	pdwCurrentAddress = (DWORD*) 0x10000;
	
	while((DWORD) pdwCurrentAddress < 0x600000)
	{
		*pdwCurrentAddress = 0x00;
		
		if(*pdwCurrentAddress != 0)
		{
			return FALSE;
		}
		
		pdwCurrentAddress++;
	}
	return TRUE;
}

// OS를 실행하기에 충분한 메모리를 가지고 있는지 체크
BOOL kIsMemoryEnough( void )
{
	DWORD* pdwCurrentAddress;

	pdwCurrentAddress = (DWORD*) 0x100000;

	while((DWORD)pdwCurrentAddress < 0x4000000)
	{
		*pdwCurrentAddress = 0x12345678;

		if(*pdwCurrentAddress != 0x12345678)
		{
			return FALSE;
		}

		pdwCurrentAddress += (0x100000 /4);
	}
	return TRUE;
}