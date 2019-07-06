## Make 프로그램

make 프로그램은 
소스 파일을 이용해서 자동으로 실행 파일 또는 라이브러리 파일을 만들어주는 **빌드 관련 유틸리티** 이다.

소스 파일과 목적 파일을 비교한 뒤 마지막 빌드 후에 수정된 파일만 선택하여 빌드를 수행한다.

> 빌드 시간을 크게 줄여준다.

또한, 빌드를 편리하게 해주는 여러 가지 문법과 규칙을 지원한다.

> 많은 수의 소스 파일을 한 번에 빌드할 수 있다.



하지만 make 프로그램이 빌드를 자동으로 수행하려면
각 소스 파일의 의존 관계나 빌드 순서, 빌드 옵션 등에 대한 정보가 필요하다.

이러한 내용이 저장된 파일이 바로 **makefile** 이다.

<hr>

### make 문법

make 문법의 기본 형식은 다음과 같이 ***Target, Dependency, Command*** 세 부분으로 구성된다.

```makefile
Target: Dependency ...
<tab> Command
<tab> Command
<tab> ...			# <tab>으로 표시한 부분은 TAB 문자로 띄워줘야한다. 공백 X
```

***Target*** 은 일반적으로 생성할 파일을 나타내며,
특정 레이블(Label)을 지정하여 해당 레이블과 관련된 부분만 빌드하는 것도 가능하다.

***Dependency*** 는 *Target* 생성에 필요한 소스 파일이나 오브젝트 파일등을 나타낸다.

***Command*** 는 *Dependency* 에 관련된 파일이 수정되면 실행할 명령을 의미한다.

> ***Command*** 에는 명령창이나 터미널(cmd.exe or 셸)에서 실행할 명령 또는 프로그램을 기술한다.

<hr>

### makefile 예제1

```makefile
# a.c b.c 를 통해서 output.exe 파일을 생성하는 예제

all: ouput.exe # 별다른 옵션이 없을 때 기본적으로 생성하는 Target

a.o : a.c
	gcc -C a.c
	
b.o : b.c
	gcc -C b.c
	
output.exe: a.o b.o
	gcc -o output.exe a.o b.o
```

make는 최종으로 생성할 Target의 의존성을 추적하면서 빌드를 처리하기 때문에 
makefile은 역순으로 따라가면 된다.

우리가 최종적으로 생성해야 하는 결과물이 output.exe 파일이므로,
가장 아래에 있는 ``output.exe:`` 라인을 보면,
오른쪽에 Dependency에서 빌드하는 과정에서 ``a.o b.o`` 가 필요한 것을 알 수 있고,
그 위쪽에서 두 파일은 ``a.c``와 ``b.c`` 로부터 생성 하는 것을 알 수 있다.

가장 윗 부분에 ``all :`` 이라고 표기한 특별한 Target 이 있다.
``all`` 은 make를 실행하면서 옵션으로 Target을 직접 명시하지 않았을 때 기본적으로 사용하는 Target이다.
여러 Target을 빌드할 때 ``all`` Target의 오른쪽에 순서대로 나열하면 한 번에 처리할 수 있다.

<hr>

### makefile 예제 2

make는 빌드를 수행하는 도중에 다른 make를 실행할 수 있다.
이는 빌드 단계를 세부적으로 나누고, 계층적으로 수행할 수 있음을 의미한다.

최상위 디렉터리의 하위에 Library 디렉터리가 있고, 빌드 과정에서 Library 디렉터리를 빌드해야 한다면,
-C 옵션을 사용해서 다음과 같이 간단히 처리할 수 있다.

```makefile
# output.exe를 빌드한다.
all: output.exe

# Library 디렉터리로 이동한 후 make를 수행
libtest.a:
	make -C Library
	
output.o: output.c
	gcc -c output.c
	
output.exe: libtest.a output.o
	gcc -o output.exe output.c -ltest -L./
```

<hr>