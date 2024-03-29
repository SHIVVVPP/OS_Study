# I/O APIC와 로컬 APIC, 대칭 I/O 모드

대칭 I/O 모드는 모든 코어로 인터럽트를 전달하는 모드로서,<BR>PIC 컨트롤러를 통하지 않고 I/O APIC와 로컬 APIC를 통해 인터럽트를 처리한다.

이 절에서는 대칭 모드에 대해 간단히 살펴보고,<BR>I/O APIC와 로컬 APIC를 사용해서 대칭 I/O 모드로 변경하는 방법을 알아본다.

<HR>

## 가장 이상적인 인터럽트 처리 기법, 대칭 I/O 모드

멀티 프로세서 또는 멀티코어 프로세서 환경에서 가장 이상적인 인터럽트 전달 경로는 다음과 같이 모든 코어에 연결되는 것이다.

![image](https://user-images.githubusercontent.com/34773827/62617937-fd038f00-b94d-11e9-95d3-7de37fd0a7c2.png)

과거의 PC나 PIC 모드 또는 가상 연결 모드에서 주된 역할을 하던 PIC 컨트롤러는 여러 CPU에 인터럽트를 전달하는 기능이 없다.

따라서 모든 코어로 인터럽트를 전달하려면 PIC 컨트롤러는 비활성화하고 I/O APIC와 로컬 APIC를 통해서 처리해야 한다.

이처럼 두 APIC를 통해 인터럽트를 처리하는 방식을 대칭 I/O 모드(Symmetric I/O Mode)라고 부른다.

그림에서도 알 수 있듯이 대칭 I/O 모드의 핵심은 I/O APIC이다.

I/O APIC를 어떻게 설정하느냐에 따라서 인터럽트 전달 경로가 달라진다.<BR>I/O APIC에는 각 핀을 담당하는 I/O 리다이렉션 테이블 레지스터가 있으며,<BR>이 레지스터를 사용하여 I/O 핀에서 전달된 인터럽트의 극성, 트리거 모드, 전달 모드, 전달 경로를 동적으로 설정할 수 있다.

I/O 리다이렉션 테이블 레지스터에서 가장 핵심이 되는 필드는 목적지 필드이다.<BR>이 필드의 값에 따라 인터럽트가 특정 APIC에게 전달되기도 하고 모든 APIC에게 전달되기도 한다.

<HR>

## I/O APIC의 구조와 레지스터

I/O APIC는 새로운 인터럽트 컨트롤러로서, <BR>인터럽트를 최대 24개까지 지원하며 다양한 종류의 인터럽트를 처리할 수 있다.

I/O APIC는 멀티프로세서나 멀티코어 프로세서를 지원하지 않는 PIC 컨트롤러를 대체할 목적으로 개발되었으며, APIC와 함께 사용된다.

다음 그림은 I/O APIC의 구조를 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62618180-8a46e380-b94e-11e9-8689-a8a214d3c675.png)

I/O APIC는 로컬 APIC와 마찬가지로 메모리 맵 I/O 방식을 사용하므로 특정 어드레스에 값을 읽고 쓰는 방식으로 제어할 수 있다. 부팅 후에 I/O APIC의 기준 어드레스는 0xFEC0000이며, 다수의  I/O APIC가 존재한다면 64KB 범위 안에서 서로 다른 기준 어드레스를 사용한다.

![image](https://user-images.githubusercontent.com/34773827/62618256-beba9f80-b94e-11e9-9a14-2b1f1b3fb6d7.png)

<HR>

그림에는 총 29개의 레지스터가 존재하지만 표에는 두 개 뿐이다.

이것은 I/O APIC의 독특한 처리 방식 때문인데,<BR>I/O APIC는 로컬 APIC처럼 개별 레지스터에 모두 메모리 어드레스를 부여한 것이 아니라,<BR>표 처럼 레지스터를 선택하는 레지스터와 데이터를 읽고 쓰는 레지스터에만 부여한다.<BR>그리고 내부로 숨긴 레지스터에는 인덱스를 지정하여 I/O 레지스터 선택 레지스터로 선택할 수 있도록 한다.

이러한 방법은 레지스터를 사용할 때 한 단계를 더 거쳐야 하는 불편함이 있지만,<BR>메모리 어드레스를 적게 사용하는 장점이 있다.

I/O 레지스터 선택 레지스터를 통해 접근할 수 있는 나머지 27개의 레지스터는 다음과 같다.

![image](https://user-images.githubusercontent.com/34773827/62618458-3e486e80-b94f-11e9-8748-3ec9e9b04559.png)

<HR>

### I/O 리다이렉션 테이블 레지스터

I/O 리다이렉션 테이블 레지스터는 I/O APIC에서 가장 중요한 레지스터이다.

I/O 리다이렉션 테이블 레지스터는 인터럽트 입력 핀에 전달된 인터럽트를 어떻게 처리할지를 정한다.<BR>우리가 I/O APIC를 사용하는 목적이 인터럽트를 특정 로컬 APIC 혹은 전체에 전달하는 것인데,<BR>그 중심에 있는 것이 바로 I/O 리다이렉션 테이블 레지스터이다.

I/O 리다이렉션 테이블 레지스터는 다른 레지스터와 달리 64비트이며, 인터럽트 입력 핀의 개수에 맞추어 24개가 존재한다.

다음 그림은 I/O 리다이렉션 테이블 레지스터의 구성과 필드 의미를 나타낸 것이다.

![image](https://user-images.githubusercontent.com/34773827/62618616-9e3f1500-b94f-11e9-9bf0-fcdc1800b565.png)

![image](https://user-images.githubusercontent.com/34773827/62618670-c29af180-b94f-11e9-9347-37c3ee9d85bd.png)

전달 모드(Delivery Mode) 필드는 외부 인터럽트 타입이 추가된 것만 제외하면 로컬 APIC에 있는 ICR 레지스터의 전달 모드와 같다. 추가된 외부 인터럽트 타입은 I/O APIC를 이용한 가상 연결 모드처럼 I/O APIC를 통해서 PIC 컨트롤러를 처리할 때 사용한다.

외부 인터럽트 타입은 인터럽트 벡터를 PIC 컨트롤러에서 전달한 것으로 사용하고,<BR>I/O APIC에 연결된 인터럽트 라인을 제외하면 I/O APIC의 핀 대부분이 외부 디바이스에서 직접 연결되어 있다.

따라서 전달 모드의 대부분은 인터럽트 벡터 필드에 저장된 값으로 인터럽트를 전달하는 고정타입을 사용한다

![image](https://user-images.githubusercontent.com/34773827/62618801-202f3e00-b950-11e9-9235-b2b4304551f9.png)

