# *sprintf( )* 와 가변 인자 처리

*sprintf( )* 함수는 주어진 형식에 맞게 정수, 문자, 문자열을 조합하여 버퍼에 출력한다.

우리에게 익숙한 *printf( )* 함수도 내부에서는 *sprintf( )* 함수를 사용하며,<br>콘솔 모드 뿐 아니라 GUI 모드에서도 널리 쓰이는 함수이다.

<hr>

## 포맷 스트링과 가변 인자

**포맷 스트링은 다른 파라미터를 어떻게 해석해야 하는지를 기술한 문자열이다.**

> *printf( )* 함수의 첫 번째 파라미터를 생각하면 쉽다.
>
> *printf( )* 함수는 첫 번째 파라미터 외에 다른 파라미터는 개수와 데이터 타입이 정해져 있지 않다.
>
> 대신 첫 번째 파라미터로 전달된 문자열에 포함된 타입 필드(%s, %d, %f 등)에 따라 파라미터 개수와 데이터 타입이 결정된다.

다음은 *printf( )* 함수의 다양한 사용 예를 나타낸 것이다.

```c
printf("Powered By CYNOS64 OS v1.0\n");
printf("%s\n", "Powered By CYNOS64 OS v1.0");
printf("Powered By CYNOS%d %s v1.0\n",64,"OS");
printf("Powered By %s%d %s v%f\n", "CYNOS",64,"OS",1.0f);
```

위 코드에서 보는 것과 같이 포맷 스트링에는 **함수의 파라미터가 가변적이라는 전제가 포함되어 있다.**

**함수의 파라미터가 정해지지 않은 함수를 가변 인자 함수(Variable Argument Function)** 라고 하며,<br>C 언어에서는 다음과 같이 정의한다.

```c
int printf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);
```

> 인자와 파라미터는 쓰이는 관점에 따라 구분된다.
>
> 파라미터(*Parameter*) 는 호출되는 함수의 관점에서 전달받기를 원하는 값을 의미하며,<br>함수를 정의할 때 ( ) 안에 기술되는 항목을 말한다.
>
> 반면 인자(*Argument*) 는 호출하는 함수의 관점에서 호출되는 함수에 전달하는 실제 값을 말한다.
>
> 비록 관점의 차이는 있지만 의미상 큰 차이는 없다.

<hr>

### 함수 파라미터를 가변적으로 처리하려면 어떻게 해야 하는가?

이것은 호출하는 쪽과 호출되는 쪽으로 나누어 생각할 수 있다.

함수를 호출하는 입장에서는 넘겨준 파라미터의 정리를 반드시 자신이 해야 한다.

호출되는 함수에서 파라미터를 정리할 수 없는 이유는 포맷 스트링 같은 데이터를 통해 파라미터의 수를 추측할 수 있지만, 이는 정확한 파라미터의 수를 나타낸다고 볼 수 없기 때문이다.

그리고 호출되는 입장에서는 포맷 스트링과 같이 파라미터 개수와 데이터 타입을 추측할 수 있는 정보와 가변 파라미터의 시작 위치에 대한 정보가 필요하다.

***stdcall, cdecl, fastcall*** 이 세가지 호출 규약 중 위의 두 가지 조건을 만족하는 규약을 찾으면<br>***cdecl, fastcall*** 이 나온다.

> ***stdcall*** 방식은 호출되는 함수에서 스택을 정리하는 방식이므로 파라미터의 개수를 호출되는 함수가 정확히 알아야 한다.
>
> 따라서 ***stdcall*** 방식은 가변 인자를 사용할 수 없다.

<hr>

그렇다면 ***cdecl*** 과 ***fastcall*** 방식을 사용해서 어떻게 가변인자를 처리할 수 있을까?

x86프로세서의 함수 호출 방식은 파라미터를 주로 스택에 넣어 전달한다.

따라서 호출하는 함수는 파라미터를 모두 스택에 넣어 전달하고,<br>호출되는 함수는 포맷 스트링을 참조해 스택에서 파라미터를 직접 참조하는 방법으로 가변인자를 처리할 수 있다. 

#### 스택을 직접 참조하여 가변인자를 처리하는 코드(*cdecl 방식, IA-32e 모드*)

```c
void main(void)
{
    int iSum;
    
    // 첫 번째 파라미터가 5이므로 뒤에 5개의 파라미터가 있다는 것을 의미
    iSum = Sum(5,1,2,3,4,5);
    printf("Sum = %d\n",iSum);
}

int Sum(int iParameterCount,...)
{
    QWORD qwArgumentStartAddress;
    int i;
    int iSum = 0;
    
    // 가변 인자의 시작 어드레스 계산, iParameter 뒤에 위치
    qwArgumentStartAddress = ((QWORD)(&iParameterCount)) + 8;
    // 남은 인자의 수 만큼 데이터를 꺼내 더한다.
    for( i = 0; i< iParameterCount; i++)
    {
        iSum += *((int*)(qwArgumentStartAddress));
        qwArgumentStartAddress += 8;
    }
    
    return iSum;
}
```

#### 가변 인자가 전달된 *Sum( )* 함수의 스택 내용(cdecl 방식, IA-32e 모드)

![image](https://user-images.githubusercontent.com/34773827/61293068-d7331080-a80d-11e9-9bd9-d658c0da47a3.png)

스택을 직접 참조하여 가변인자를 처리하는 방식은 모든 파라미터를 스택으로 전달할 때만 가능하다.

만약 일부 파라미터를 레지스터로 전달하고 나머지 파라미터를 스택에 전달하는 ***fastcall*** 방식이나<br>64비트 호출 규약을 사용한다면 레지스터의 파라미터를 스택에 옮겨주는 작업이 필요하다.

> 중계함수를 거쳐 레지스터에 있는 파라미터를 스택에 넣어가면서 가변 인자를 쓴다는 것은<br>너무 비효율적이다.

<hr>

### C언어에서의 가변인자 처리 방식

C 언어는 가변인자를 위해 **va_list** 데이터 타입과 ***va_start( ), va_arg( ), va_end( )*** 매크로를 제공한다.

다음은 각 매크로와 가변인자 리스트(va_list)의 사용 예이다.

- ***va_start(va_list va, 가변인자 앞의 파라미터 이름 )***

  가변 인자 앞에 위치하는 파라미터 이름으로 가변인자의 시작 어드레스를 계산하여 가변인자 리스트에 삽입

- ***va_arg(va_list va, 데이터 타입 )***

  가변 인자 리스트에서 데이터 타입에 해당하는 값을 꺼내고,<br>가변인자 리스트를 데이터 크기만큼 이동한다.

- ***va_end( va_list va )***

  가변인자 리스트를 사용 종료

```c
int Sum(int iParameterCount, ...)
{
    va_list va;
    int i;
    int iSum = 0;
    
    // 가변 인자의 시작 어드레스 계산
    va_start( va , iParameterCount ); // 가변 인자의 어드레스를 가변 인자 리스트에 등록
    
    // 남은 인자의 수만큼 데이터를 꺼내 더한다.
    for( i = 0 ; i < iParameterCount ; i++ )
    {
        iSum += (int)va_arg(va,int);
    }
    
    // 가변인자 사용 종료
    va_end(va);
    
    return iSum;
}
```

위의 코드를 보면 가변인자 매크로와 리스트를 사용하여 훨씬 편리하게 처리할 수 있음을 알 수 있다.

<hr>

#### 가변인자 매크로와 데이터 타입이 어떻게 정의되어 있는지 확인해보자

다음은 GCC의 헤더 파일 중 ``stdarg.h`` 파일에 정의되어 있는 가변 인자 리스트와 매크로이다.

```c
// 데이터 타입
typedef int __gnuc_va_list;
typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list va_list;

// 매크로
#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)		__builtin_va_end(v)
#define va_arg(v,l)		__builtin_va_arg(v,l)
```

va_list 타입은 결국 int 타입으로 정의되어 있으며,<br>매크로는 ``__builtin_va_start()``,``builtin_va_end( )`` , ``__builtin_va_arg()`` 함수로 대체되는 것을 알 수 있다.

``__builtin_va_ ``  부류의 함수는 GCC가 다양한 프로세서를 지원하기 위해 정의한 내부 함수 파일로<br>컴파일 할 때 코드로 확장되어 라이브러리를 사용하지 않는다.

따라서 가변 인자 매크로를 사용해도 우리 OS를 빌드하는 데 아무 문제가 없다.

