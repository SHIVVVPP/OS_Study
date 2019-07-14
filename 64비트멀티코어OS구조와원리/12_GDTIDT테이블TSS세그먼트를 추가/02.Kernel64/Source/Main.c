#include "Types.h"
///////////////////////////////////////////////////////////////
// 				키보드 드바이스 드라이버 추가				  ///
//////////////////////////////////////////////////////////////
#include "Keyboard.h"
/////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// 				GDT, IDT 테이블 TSS 세그먼트 추가			  //
//////////////////////////////////////////////////////////////
#include "Descriptor.h"
//////////////////////////////////////////////////////////////


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

	///////////////////////////////////////////////////////////////
	// 				GDT, IDT 테이블 TSS 세그먼트 추가			  //
	//////////////////////////////////////////////////////////////
	kPrintString(0, 13, 0xCF,"GDT Initialize And Switch For IA-32e Mode...[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kPrintString(45,13,0x4A,"Pass");

	kPrintString(0,14,0xCF,"TSS Segment Load............................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kPrintString(45,14,0x4A,"Pass");

	kPrintString(0,15,0xCF,"IDT Initialize..............................[    ]");
	kInitializeIDTTables();
	kLoadIDTR(IDTR_STARTADDRESS);
	kPrintString(45,15,0x4A,"Pass");
	//////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////
	// 				키보드 드바이스 드라이버 추가				///
	/////////////////////////////////////////////////////////////
	kPrintString(0,16,0x39,"Keyboard Active.............................[    ]");

	// 키보드 활성화
	if(kActivateKeyboard() == TRUE)
	{
		kPrintString(45,16,0xA9,"Pass");
		kChangeKeyboardLED(FALSE,FALSE,FALSE);
	}
	else
	{
		kPrintString(45,16,0x34,"Fail");
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
					///////////////////////////////////////////////////////////////
					// 				GDT, IDT 테이블 TSS 세그먼트 추가			  //
					//////////////////////////////////////////////////////////////
					// 0이 입력되면 변수를 0으로 나누어 Divide Error 예외(벡터 0번)를 발생시킨다.
					if(vcTemp[0] == '0')
					{
						// 아래 코드를 수행하면 Divide Error 예외가 발생하여 
						// 커널의 임시 핸들러가 수행된다.
						bTemp = bTemp / 0;
					}
					//////////////////////////////////////////////////////////////
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