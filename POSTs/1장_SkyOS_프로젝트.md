

![Image](https://i.imgur.com/oCtWyvK.png)

# 1장 SkyOS 프로젝트


### 운영체제를 개발하기 위해 필요한 개발 도구 정리
### SkyOS를 대략적으로 살피고, 정상적으로 소스코드가 빌드되는지 확인
### 가상 에뮬레이터에서 SkyOS 커널 실행 테스트

### [C++로 나만의 운영체제 만들기 Github 자료](https://github.com/pdpdds/skyos)



## 예비 지식

알고 있다고 가정

- C/C++ 언어
- 자료구조
- 운영체제 이론
- Visual Studio

반드시 알고 있을 필요는 없지만 도움이 되는 내용

- 디자인 패턴 / UML
- 시스템 프로그래밍
- 어셈블리 언어
- STL



## SkyOS 콘셉

- 윈도우 운영체제가 동작하는 원리와 유사하게 구현한다.
- 객체지향 언어인 C++로 구현한다.
- STL의 활용
- 디버깅 활용 기법
- new, delete 연산자 구현
- 애플리케이션 간의 독립적인 주소공간 제공
  - 페이징 구현에 힘쓴다.
- 커널 레이어와 기타 레이어를 분리
  - 레이어간 별도 개발이 가능하도록 구조화
- USB에 부팅 가능한 콘솔 운영체제 개발
- 16비트 리얼모드와 관계된 내용 지양
  - 처음부터 32비트 보호 모드 상태에서 개발 진행
- 어셈블리 코드의 최소화



## 준비물

- Visual Studio 2017
  - 윈도우 7 - Windows SDK 8.1
  - 윈도우 10 - Windows SDK 10
- WinImage
  - 가상 디스크 이미지 생성 편집 도구
  
  - [winImage 다운로드 사이트](http://www.winimage.com/)
  
    ![Image](https://i.imgur.com/Em86Z6v.png)
- QEMU (각자 실습 폴더를 만들어 설치하자)
  - 가상 머신 환경 제공 프로그램
  - [QEMU 다운로드 사이트1](www.omledom.com)
    - 64bit
    - ![Image](https://i.imgur.com/1p8kHZl.png)
    - 32bit
    - ![Image](https://i.imgur.com/NImaZtq.png)
  - [저자 GoogleDrive](https://drive.google.com/drive/folders/1Lwm2t7rAEHrBl3G7IUoIHzbG4QEPgFnw) 를 통한 다운로드 (추천)
- SkyOS 소스코드
  - [TortoiseSVN 다운](https://osdn.net/projects/tortoisesvn/storage/1.12.0/Application/TortoiseSVN-1.12.0.28568-x64-svn-1.12.0.msi/)
    
    > SVN ? 
    >
    > SubVersion의 줄임말로 형상관리/ 소스 관리 툴이다.
    >
    > 목적 : 여러명이서 작업하는 프로젝트의 경우 버전관리나 각자 만든 소스의 통합과 같은 문제를 해결하기 위해 저장소를 만들어 그곳에 소스를 저장해 소스 중복이나 여러 문제를 해결하기 위한 Software이다.
    >
    > 즉, 하나의 서버에서 소스를 쉽고 유용하게 관리할 수 있게 도와주는 툴

    - 1. svn 주소 를 얻고
    
         ![Image](https://i.imgur.com/9VBD3Ao.png)
    
      2. 사용하고자 하는 폴더에 우클릭 후 **SVN Checkout..** 클릭
    
      3. OK
    
         ![Image](https://i.imgur.com/f6qtnt1.png)
    
      4. 다운로드![Image](https://i.imgur.com/hdGWTPt.png)
  
  


## 프로젝트 빌드

### 구성

> 다운받은 폴더\trunk\

![Image](https://i.imgur.com/Cq70PLD.png)

각 폴더의 역할

- Chapter : 책의 각 파트와 관계된 튜토리얼 프로젝트
- CommonLib : 애플리케이션과 커널이 동시에 사용하는 공통 라이브러리
- Module : SkyOS 커널을 운용하는 데 도움이 되는 모듈
- Reference : 개념 및 이해를 돕기 위한 레퍼런스 코드



### 프로젝트 빌드 

1. Chapter 폴더의 **01_HelloWorld.sln**

   > 디버그 모드, x86으로 빌드, SDK 1.10.16299.0 버전
   >
   > 1.10.16299 -> 도구 -> 도구 및 기능 가져오기
   >
   > ![Image](https://i.imgur.com/pAn8ObY.png)

2. 솔루션 빌드

   > 솔루션이 빌드되면 Debug 폴더에 **SKYOS32.EXE 파일** 이 생성된다.
   >
   > ![Image](https://i.imgur.com/xHAK4Hb.png)
   >
   > 이 파일이 **SkyOS의 커널 이미지**이다.

   

## 커널의 실행

운영체제를 개발하고 그 결과를 확인하는 과정

- 커널을 빌드해서 SKYOS32.EXE 파일을 생성
- SKYOS32.EXE 파일을 가상 이미지에 복사
- QEMU를 통해 커널의 동작을 확인

가상 이미지를 만드는 방법은 나중에 설명하므로,  샘플 가상 이미지를 다운받아 정상적으로 실행되는지 테스트 한다.

> 저자의 구글 드라이브의 [IMAGE/01_HELLO](https://drive.google.com/drive/folders/1ZNwxZh6QGgjhIP59J_UmhVZpMwQnGCcs) 폴더에서 SkyOS.IMA 파일과 SkyOS.BAT 파일을 다운
>
> 이 파일들을 열기 위해 **WinImage** 필요

- SkyOS.IMA 더블클릭

  ![Image](https://i.imgur.com/5lstiMs.png)

  ![Image](https://i.imgur.com/kiympvl.png)

​	

- SKYOS32.EXE 파일을 가상 이미지로 복사하기 위해서 파일을 해당 폴더로 끌어놓거나 인젝션 버튼을 통해서 파일을 선택한다.

- ``C:\Program Files\qemu`` 로 QEMU가 설치된 폴더로 이동.

  > qemu-system-x86_64.exe 파일 실행 전, 
  >
  > 실행파일에 명령 인자를 주기 위해서 간단한 배치 파일을 만들어야 하는데
  >
  > 다운받은 SkyOS.BAT 배치 파일을 사용한다.

- QEMU 폴더에 SkyOS.BAT 파일과 SkyOS.IMA를 같이 복사한 후 SkyOS 배치 파일을 실행한다.

  > qemu 폴더 경로 : C:Program Files/qemu 에 SkyOS.BAT과 SkyOS.IMA 같이 복사

  > 오류
  >
  > ![Image](https://i.imgur.com/MTIHzzl.png)
  >
  > 해결법
  >
  > 먼저 qemu-system-x86_64.exe 파일을 우클릭 한후 관리자 권한으로 실행할 수 있도록 설정
  >
  > ![Image](https://i.imgur.com/8eyWija.png)
  >
  > 이후 SkyOS.bat 배치파일을 텍스트로 연뒤
  >
  > > cd C:\Program Files\qemu<br>qemu-system-x86_64 -L . -m 128 -fda SkyOS.IMA -soundhw sb16,es1370 -localtime -M pc -boot a<br>SET /P P = Press any key continue: // 배치파일오류시 바로 꺼지지 않도록 설정으로 밑의 내용을 수정해준다.
  > >
  > > ![Image](https://i.imgur.com/J5QemSd.png)
  > 
  > 이제 실행 하면
  > 
  > ![Image](https://i.imgur.com/XNNUZs8.png)
  > 
  > **"Hello World!"**
  >
  > ![Image](https://i.imgur.com/nI5Bs45.png)



## 정리

1장에서는

- SkyOS 프로젝트의 레이아웃 확인
- 소스코드 빌드
- 결과물을 에뮬레이터에서 실행
