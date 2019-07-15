[BITS 64]

SECTION .text

; C 언어에서 호출할 수 있도록 이름을 노출한다.
global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

; 포트로부터 1바이트를 읽는다.
;   PARAM : 포트 번호
kInPortByte:
    push rdx

    mov rdx, rdi
    mov rax, 0
    in al, dx

    pop rdx
    ret

; 포트에 1바이트를 쓴다.
;   PARAM : 포트 번호, 데이터
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al

    pop rax
    pop rdx
    ret

; GDTR 레지스터에 GDT 테이블을 설정
;   PARAM: GDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadGDTR:
    lgdt [ rdi ]    ; 파라미터 1(GDTR의 어드레스)를 프로세서에 로드하여 
                    ; GDT 테이블을 설정
    ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
;   PARAM: TSS 세그먼트 디스크립터의 오프셋
kLoadTR:
    ltr di          ; 파라미터 1(TSS 세그먼트 디스크립터의 오프셋)을 프로세서에
                    ; 설정하여 TSS 세그먼트를 로드
    ret
    
; IDTR 레지스터에 IDT 테이블을 설정
;   PARAM: IDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadIDTR:
    lidt [ rdi ]    ; 파라미터 1(IDTR의 어드레스)을 프로세서에 로드하여
                    ; IDT 테이블을 설정
    ret


;=========================================================
; 추가
;=========================================================

; 인터럽트를 활성화
;   PARAM: 없음
kEnableInterrupt:
    sti             ; 인터럽트를 활성화
    ret

; 인터럽트를 비활성화
;   PARAM: 없음
kDisableInterrupt:
    cli             ; 인터럽트를 비활성화
    ret

; RFLAGS 레지스터를 읽어서 되돌려줌
;   PARAM: 없음
kReadRFLAGS:
    pushfq                  ; RFLAGS 레지스터를 스택에 저장
    pop rax                 ; 스택에 저장된 RFLAGS 레지스터를 RAX 레지스터에 저장하여
                            ; 함수의 반환 값으로 설정
    ret
