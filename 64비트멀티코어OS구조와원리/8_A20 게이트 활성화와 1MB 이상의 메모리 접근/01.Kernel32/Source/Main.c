#include "Types.h"

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);
BOOL kInitializeKernel64Area(void);

//Main
void Main(void)
{
	DWORD i;
	kPrintString( 0, 3, 0xE9, "C Language Kernel Started!");
	
	kInitializeKernel64Area();
	kPrintString(0,4,0x4A,"IA-32e Kernel Area Initialization Complete");
	
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
	
	pdwCurrentAddress = (DWORD*) 0x100000;
	
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
