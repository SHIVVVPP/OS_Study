[BITS 64]

SECTION .text

; C 언어에서 호출할 수 있도록 이름을 노출한다.
global kInPortByte, kOutPortByte

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