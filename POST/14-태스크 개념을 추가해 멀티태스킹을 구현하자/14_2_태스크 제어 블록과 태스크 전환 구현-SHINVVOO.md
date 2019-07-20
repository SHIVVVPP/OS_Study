# 태스크 제어 블록과 태스크 전환 구현

## 태스크 제어 블록 정의

탯흐크 제어 블록(*TCB, Task Control Block*)은 태스크의 종합적인 정보가 들어있는 자료구조로써<br>코드와 데이터, 콘텍스트와 스택, 태스크를 구분하는 ID와 기타 속성 값을 관리한다.

TCB를 사용하는 이유는<br> 태스크의 정보를 TCB에 모아 관리하면 태스크에 관련된 작업을 편리하게 처리할 수 있기 때문이다.

TCB는 각 태스크의 상태를 그대로 반영하는 만큼<br>생성된 태스크와 TCB는 반드시 하나씩만 연결되어야 한다.

#### TCB 자료구조 정의

```c
// 태스크의 상태를 관리하는 자료구조
typedef struct kTaskControlBlockStruct
{
    // 콘텍스트
    CONTEXT stContext;
    
    // ID와 플래그
    QWORD qwID;
    QWORD qwFlags;
    
    // 스택의 어드레스와 크기
    void* pvStackAddress;
    QWORD qwStackSize;
}TCB;
```

<hr>

콘텍스트는 프로세서의 상태, **즉 레지스터를 저장하는 자료구조이다.**

여기에는 인터럽트와 예외 처리에서 저장하는 것과 같은 방식으로 총 24개의 레지스터를 저장한다.

< 콘텍스트 자료구조에 저장되는 24개의 레지스터 >

