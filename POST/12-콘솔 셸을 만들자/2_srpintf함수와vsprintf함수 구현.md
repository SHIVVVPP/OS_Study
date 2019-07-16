# *sprintf( )* 함수와 *vsprintf( )* 함수 구현

*sprintf( )* 함수는 *printf( )* 함수는 거의 같은 기능을 한다.

두 함수의 차이점이라면 *printf( )* 함수는 결과를 화면에 출력하고,<br>*sprintf( )* 함수는 결과를 문자열 버퍼에 출력한다는 점이다.

> *printf( )* 함수는 *vsprintf( )* 함수를 이용하여 처리할 수 있으므로<br> 먼저 *sprintf( )* 함수와 *vsprintf( )* 함수를 알아본자.

<hr>

*vsprintf( )* 함수는 *sprintf( )* 함수의 실질적인 기능을 담당하는 함수로<br>가변인자 대신 va_list 타입의 가변인자 리스트를 받아서 작업을 처리한다.

#### *sprintf( )*

*vsprintf( )* 함수가 가변인자가 아니라 va_lit 타입의 가변인자 리스트를 받는 이유는 다음과 같다.

```c
int printf(const char* pcFormatString, ...)
{
    char vcBuffer[1024];
    int iLength;
    
    // 문법 오류
    sprintf( vcBuffer, pcFormatString, ...); 	// '...'를 다른 함수의 파라미터로
    											// 전달할 수 없으므로 에러가 발생한다.
    PrintStringInternal(vcBuffer);
    
    return iLength;
}

int sprintf(char* pcBuffer, const char* pcFormatString, ...)
{
    ... 생략 ...
    return iLength;
}
```

위와 같이 가변인자를 다른 함수로 전달할 수 없다.

그래서 '...' 대신 va_list 타입의 가변 인자 리스트를 넘겨 처리하는 것이다.

다음은 가변 인자 리스트를 사용하여 처리하게 변경한 것이다.

```c
// printf() 함수의 구현
int printf( const char* pcFormatString, ... )
{
    char vcBuffer[1024];
    int iLength;
    va_list va;
    
    // va_list를 받는 vsprintf() 함수 호출
    va_start(va, pcFormatString);	// pcFormatString 이후 부터 가변인자로 지정
    vsprintf(vcBuffer, pcFormatString, va);
    va_end(va);		// 가변인자 사용 종료
    
    PrintStringInternal(vcBuffer);
    
    return iLength;
}

// sprintf() 함수 구현
int sprintf( char* pcBuffer, const char* pcFormatString, ... )
{
    int iLength;
    va_list va;
    
    // va_list 를 받는 vsprintf() 함수 호출
    va_start(va, pcFormatString);
    vsprintf(vcBuffer, pcFormatString, va);		// '...'을 인자로 전달할 수 없으므로
    											// 가변인자변수로 가변인자를 전달
    va_end(va);
    
    return iLength;
}

//  실질적인 처리를 수행하는 함수
int vsprintf(char* pcBuffer, const char* pcFormatString, va_list va)
{
    ... 생략 ...
    
    return iLength;
}
```

<hr>

#### *vsprintf( )*

*vsprintf( )* 함수는 포맷 스트링과 가변 인자 리스트를 받아서 포맷 스트링의 형식에 따라 출력 버퍼를 채운다.

C 라이브러리에 포함된 *vsprintf( )* 함수의 포맷 스트링은 '%' 기호와 알파벳으로 구성되어,<br>10진수와 16진수 정수 타입부터 문자와 문자열, 실수에 이르기까지 다양한 타입을 사용할 수 있다.

다음은 우리가 만들 OS에서 지원하는 데이터 타입 예이다.

| 데이터 타입 | 설명                                               | 비고          |
| ----------- | -------------------------------------------------- | ------------- |
| %s          | 문자열 타입의 파라미터                             |               |
| %c          | 문자 타입의 파라미터                               |               |
| %d, %i      | 부호 있는 정수 타입의 파라미터                     | 10진수로 출력 |
| %x, %X      | 4바이트 부호 없는 정수 타입(DWORD)의 파라미터      | 16진수로 출력 |
| %q, %Q      | 8바이트 부호 없는 정수 타입(QWORD)의 파라미터      |               |
| %p          | 8바이트 포인터 타입의 파라미터<br />%q, %Q와 같다. |               |

<hr>

#### *vsprintf( )* 함수의 전체적인 알고리즘

```c
while( 포맷 스트링의 끝이 아님)
{
    if( 포맷 스트링의 현재 문자가 데이터 타입을 나타내는 '%' 문자인가?)
    {
        '%' 이후의 문자에 따라 가변 인자 타입을 해석하여 출력 버퍼에 복사
        가변 인자의 어드레스를 다음으로 이동
    }
    else
    {
        포맷 스트링의 현재 문자를 출력 버퍼에 복사
    }
    포맷 스트링 내의 문자 위치를 다음으로 이동
}
return 출력 버퍼의 문자 길이와 내용
```

#### *vsprintf( )* 함수의 구현

```c
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap)
{
    QWORD i, j;
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char* pcCopyString;
    QWORD qwValue;
    int iValue;
    
    // 포맷 문자열의 길이를 읽어서 문자열의 길이만큼 데이터를 출력 버퍼에 출력
    iFormatLength = kStrLen(pcFormatString);
    for( i = 0 ; i < iFormatLength ; i++ )
    {
        // %로 시작하면 데이터 타입 문자로 처리
        if(pcFormatString[i] == '%')
        {
            // % 다음 문자로 이동
            i++;
            switch(pcFormatString[i])
            {
                    // 문자열 출력
                case 's':
                    // 가변 인자에 들어 있는 파라미터를 문자열 타입으로 변환
                    pcCopyString = (char*)(va_arg(ap,char*));
                    iCopyLength = kStrLen(pcCopyString);
                    // 문자열의 길이 만큼을 출력 버퍼로 복사하고 출력한 길이만큼 버퍼의 인덱스를 이동
                    kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
                    iBufferIndex += iCopyLength;
                    break;
                    // 문자 출력
                case 'c':
                    // 가변 인자에 들어 있는 파라미터를 문자 타입으로 변환하여
                    // 출력 버퍼에 복사하고 버퍼의 인덱스를 1만큼 이동
                    pcBuffer[iBufferIndex] = (char)(va_arg(ap,int));
                    iBufferIndex++;
                    break;
                    // 정수 출력
                case 'd':
                case 'i':
                    // 가변인자에 들어 있는 파라미터를 정수 타입으로 변환
                    // 출력 버퍼에 복사하고 출력한 길이만큼 버퍼의 인덱스를 이동
                    iValue = (int)(va_arg(ap,int));
                    iBufferIndex += kIToA(iValue,pcBuffer + iBufferIndex, 10);
                    break;
                    // 4바이트 Hex 출력
                case 'x':
                case 'X':
                    // 가변 인자에 들어있는 파라미터를 DWORD 타입으로 변환
                    // 출력 버퍼에 복사하고 출력한 길이만큼 버퍼의 인덱스를 이동
                    qwValue = (DWORD)(va_arg(ap,DWORD))& 0xFFFFFFFF;
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;
                    
                    // 8바이트 Hex 출력
                case 'q':
                case 'Q':
                case 'p':
                    // 가변 인자에 들어 있는 파라미터를 QWORD 타입으로 변환하여
                    // 출력 버퍼에 복사하고 출력한 길이만큼 버퍼의 인덱스를 이동
                    qwValue = (QWORD)(va_arg(ap, QWORD));
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;
                    // 위에 해당하지 않으면 문자를 그대로 출력하고 버퍼의 인덱스를 1만큼 이동
                default:
                    pcBuffer[iBufferIndex] = pcFormatString[i];
                    iBufferIndex++;
                    break;
            }
        }
        // 일반 문자열 처리
        else
        {
            // 문자를 그대로 출력하고 버퍼의 인덱스를 1만큼 이동
            pcBuffer[iBufferIndex] = pcFormatString[i];
            iBufferIndex++;
        }
    }
    
    // NULL을 추가하여 완전한 문자열로 만들고 출력한 문자의 길이를 반환
    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}
```

<hr>



## *itoa( )* 함수와 *atoi( )* 함수 구현

*itoa( )* 함수는 정수 값을 문자열로 변환하며 *atoi( )* 함수는 문자열을 정수 값으로 변환한다.

#### *itoa( )* 함수 구현

```c
int kIToA(long lValue, char* pcBuffer, int iRadix)
{
    int iReturn;
    
    switch(iRadix)
    {
            //16진수
        case 16:
            iReturn = kHexToString(lValue, pcBuffer);
        	break;
            
            // 10진수 또는 기타
        case 10:
        default:
            iReturn = kDecimalToString(lValue, pcBuffer);
            break;
    }
    
    return iReturn;
}
```

*kIToA( )* 함수의 실질적인 역할은 *kHexToString( )* 함수와 *KDecimalToString( )* 함수가 담당하며,<br>각 함수는 정수를 16진수나 10진수 문자열로 변환한다.

#### 정수를 10진수나 16진수 문자열로 변환하는 방법

변환하는 방법에는 여러 가지가 있지만, 여기서는 1의 자리를 나머지 연산으로 추출하여 버퍼에 담는 방법을 사용한다.

즉 변환하는 수가 0이 될 때까지 10이나 16으로 나누기를 반복하면서 1의 자리 숫자를 버퍼에 차례대로 담은 후 버퍼의 순서를 뒤집는 것이다.

![image](https://user-images.githubusercontent.com/34773827/61300651-51b75c80-a81d-11e9-830f-e33dd1295c13.png)

<hr>

이제 이 알고리즘을 코드로 작성해보자.

다음은 정수를 10진수 문자열로 바꾸는 함수와 문자열을 뒤집는 함수의 코드이다.

```C
// 10진수 값을 문자열로 변환
int kDecimalToString(long lValue, char* pcBuffer)
{
    long i;
    
    // 0이 들어오면 바로 처리
    if(lValue == 0)
    {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }
    
    // 만약 음수면 - 추가하고 양수로 변환
    if(lValue < 0)
    {
        i = 1;
        pcBuffer[0] = '-';
        lValue = -lValue;
    }
    else
    {
        i = 0;
    }
    
    // 버퍼에 1의 자리부터 10, 100, 1000, ...의 자리 순서로 숫자 삽입
    for(; lValue > 0 ; i++)
    {
        pcBuffer[i] = '0' + lValue % 10;
        lValue = lValue / 10;
    }
    pcBuffer[i] = '\0';
    
    // 버퍼에 들어 있는 문자열을 뒤집어서 ..., 1000, 100, 10, 1의 자리 순서로 변경
    if(pcBuffer[0] == '-')
    {
        // 음수인 경우는 부호를 제외하고 문자열을 뒤집는다.
        kReverseString(&(pcBuffer[1]));
    }
    else
    {
        kReverseString(pcBuffer);
    }
    
    return i;
}

// 문자열의 순서를 뒤집는다.
void kReverseString(char* pcBuffer)
{
    int iLength;
    int i;
    char cTemp;
    
    // 문자열의 가운데를 중심으로 좌/우를 바꿔서 순서를 뒤집는다.
    iLength = kStrLen(pcBuffer);
    for(i = 0; i < iLength / 2; i++)
    {
        cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength - 1 - i];
        pcBuffer[iLength - 1 - i] = cTemp;
    }
}
```

<hr>

#### 정수를 16진수로 변환하는 코드

```c
int kHexToString(QWORD qwValue, char* pcBuffer)
{
    QWORD i;
    QWORD qwCurrentValue;
    
    // 0이 들어오면 바로 처리
    if(qwValue == 0)
    {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }
    
    // 버퍼에 1의 자리부터 16, 256, ...의 자리 순서로 숫자 삽입
    for(i = 0; qwValue > 0; i++)
    {
        qwCurrentValue = qwValue % 16;
        if(qwCurrentValue >= 10)
        {
            pcBuffer[i] = 'A' + (qwCurrentValue - 10);
        }
        else
        {
            pcBuffer[i] = '0' + qwCurrentValue;
        }
        
        qwValue = qwValue / 16;
    }
    pcBuffer[i] = '\0';
    
    // 버퍼에 들어 있는 문자열을 뒤집어서 ...256, 16, 1의 자리 순서로 변경
    kReverseString(pcBuffer);
    return i;
}
```

<hr>

#### *atoi( )* 함수 구현

이제 반대로 문자열을 정수로 바꾸는 방법을 살펴보자.

```c
long kAToI(const char* pcBuffer, int iRadix)
{
    long lReturn;
    
    switch(iRadix)
    {
            // 16진수
        case 16:
            lReturn = kHexStringToQword(pcBuffer);
            break;
            
            // 10진수
        case 10:
        default:
            lReturn = kDecimalStringToLong(pcBuffer);
            break;
    }
    return lReturn;
}
```

*kAToI( )* 함수 역시 *kHexStringToQword( )* 함수와 *kDecimalStringToLong( )* 함수가 실질적인 기능을 담당하며,<br>각자 16진수 문자열과 10진수 문자열을 정수로 변환한다.

*kAToI( )* 함수는 *kIToA( )* 함수의 반대 기능을 하므로 정수를 문자열로 변환하는 알고리즘을 참고하여 역순으로 작성하면 된다.

```c
long kDecimalStringToLong(const char* pcBuffer)
{
    long lValue = 0;
    int i;
    
    // 음수면 -를 제외하고 나머지를 먼저 long으로 변환
    if(pcBuffer[0] == '-')
    {
        i = 1;
    }
    else
    {
        i = 0;
    }
    
    // 문자열을 돌면서 차례로 변환
    for( ; pcBuffer[i] != '\0' ; i++)
    {
        lValue *= 10;
        lValue += pcBuffer[i] - '0';
    }
    
    // 음수면 - 추가
    if(pcBuffer[0] == '-')
    {
        lValue = -lValue;
    }
    return lValue;
}
```

16진수 문자열을 정수로 변환하는 *kHexStringToQword( )* 함수는 *kDecimalStringToString( )* 함수와 거의 같다.





