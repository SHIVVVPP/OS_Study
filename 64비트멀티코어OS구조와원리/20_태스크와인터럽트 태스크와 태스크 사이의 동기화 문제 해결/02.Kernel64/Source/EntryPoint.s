[BITS 64]

SECTION .text

; 외부에서 정의된 함수를 쓸 수 있도록 선언한다.(Import)
extern Main

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
    mov ax, 0x10    ; IA-32e 모드 커널용 데이터 세그먼트 디스크립터를 AX에 저장
    mov ds, ax      ; DS 세그먼트 셀렉터에 설정
    mov es, ax      ; ES 세그먼트 셀렉터에 설정
    mov fs, ax      ; FS 세그먼트 셀렉터에 설정
    mov gs, ax      ; GS 세그먼트 셀렉터에 설정

    ; 스택을 0x600000 ~ 0x6FFFFF 영역에 1MB 크기로 생성
    mov ss, ax          ; SS 세그먼트 셀렉터에 설정
    mov rsp, 0x6FFFF8   ; RSP 레지스터의 어드레스를 0x6FFFF8로 설정
    mov rbp, 0x6FFFF8   ; RBP 레지스터의 어드레스를 0x6FFFF8로 설정

    Call Main           ; C 언어 엔트리 포인트 함수(Main) 호출

    jmp $