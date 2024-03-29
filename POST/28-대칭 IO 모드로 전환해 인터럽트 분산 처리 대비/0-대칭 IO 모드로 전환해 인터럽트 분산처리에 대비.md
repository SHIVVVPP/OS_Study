# 대칭 I/O 모드로 전환해<br>인터럽트 분산 처리에 대비

지금까지 인터럽트는 PIC 모드나 가상 연결 모드를 사용하여 BSP에만 전달되었다.<BR>이제 BSP만 동작하는 것이 아니므로 I/O APIC와 로컬 APIC를 사용해서 모든 코어가 인터럽트를 처리 할 수 있도록 바꾼다.

모든 코어가 인터럽트를 처리할 수 있다는 것은 의미있는 일이다.<BR>인터럽트가 AP로도 전달되므로 멀티태스킹을 할 수 있는 것은 물론이고 인터럽트 부하도 분산시킬 수 있다.

프로세서는 인터럽트를 처리하는 동안 콘텍스트를 저장하고 핸들러를 수행한 뒤 콘텍스트를 다시 복원하는 작업을 수행한다. 인터럽트를 처리하는 데 드는 시간은 콘텍스트를 저장/복원하고 다시 핸들러를 수행하는 시간을 합쳐도 아주 짧지만 인터럽트가 아주 빈번하게 발생한다면 이 시간도 늘어난다.

지금처럼 인터럽트가 BSP에서만 처리된다면 인터럽트가 많이 발생할수록 BSP는 다른 AP보다 처리 효율이 떨어지게 된다. 만일 인터럽트를 여러 코어가 나누어서 처리할 수 있다면 부하가 분산되므로 모든 코어가 고른 성능을 낼 수 있다.

이와 같이 인터럽트를 나누어 처리하는 방법을 인터럽트 부하 분산(Interrupt Load Balancing)이라고 한다.

인터럽트 부하를 분산시키려면 일단 모든 코어가 인터럽트를 처리할 수 있어야 한다.<br>따라서 이번 장에서는 인터럽트를 모든 코어에 전달하는 방법을 알아보고,<br>인터럽트 부하 분산 기법을 구현한다.

