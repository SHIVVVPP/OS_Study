#include "Types.h"

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);

//Main
void Main(void)
{
	kPrintString( 0, 3, 0xE9, "C Language Kernel Started!");
	
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