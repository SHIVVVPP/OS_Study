#include "Types.h"
///////////////////////////////////////////////////////////////
// 				키보드 드바이스 드라이버 추가					///
//////////////////////////////////////////////////////////////
#include "Keyboard.h"
/////////////////////////////////////////////////////////////

// 함수 선언

void kPrintString( int iX, int iY, BYTE Attr, const char* pcString);

// 아래 함수는 C 언어 커널의 시작 부분이다.
void Main(void)
{
	/////////////////////////////////////////////////////////////
	// 				키보드 드바이스 드라이버 추가				///
	/////////////////////////////////////////////////////////////
	char vcTemp[2] = {0,};
	BYTE bFlags;
	BYTE bTemp;
	int i = 0;
	/////////////////////////////////////////////////////////////

    kPrintString(0,11,0x2F,"Switch To IA-32e Mode Success!");
    kPrintString(0,12,0x2F,"IA-32e C Language Kernel Start..............[    ]");
    kPrintString(45,12,0xA9,"Pass");

	/////////////////////////////////////////////////////////////
	// 				키보드 드바이스 드라이버 추가				///
	/////////////////////////////////////////////////////////////
	kPrintString(0,13,0x39,"Keyboard Active.............................[    ]");

	// 키보드 활성화
	if(kActivateKeyboard() == TRUE)
	{
		kPrintString(45,13,0xA9,"Pass");
		kChangeKeyboardLED(FALSE,FALSE,FALSE);
	}
	else
	{
		kPrintString(45,13,0x34,"Fail");
		while(1);
	}

	while(1)
	{
		kPrintString(0,18,0xFC,"Keyboard :");
		// 출력 버퍼(포트 0x60)가 차 있으면 스캔 코드를 읽을 수 있다.
		if(kIsOutputBufferFull() == TRUE)
		{
			// 출력 버퍼(포트 0x60)에서 스캔 코드를 읽어서 저장
			bTemp = kGetKeyboardScanCode();

			// 스캔 코드를 ASCII 코드로 변환하는 함수를 호출하여 ASCII 코드와
			// 눌림 또는 떨어짐 정보를 반환한다.
			if(kConvertScanCodeToASCIICode(bTemp,&(vcTemp[0]),&bFlags) == TRUE)
			{
				// 키가 눌러졌으면 키의 ASCII 코드 값을 화면에 출력한다.
				if(bFlags & KEY_FLAGS_DOWN)
				{
					kPrintString(i++, 20, 0xF1,vcTemp);
				}
			}
		}
	} 
	/////////////////////////////////////////////////////////////
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