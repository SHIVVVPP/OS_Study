#include "Types.h"

// 함수 선언

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);

// 아래 함수는 C 언어 커널의 시작 부분이다.
void Main(void)
{
    kPrintString(0,11,0x2F,"Switch To IA-32e Mode Success!");
    kPrintString(0,12,0x2F,"IA-32e C Language Kernel Start..............[    ]");
    kPrintString(45,12,0xA9,"Pass");
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