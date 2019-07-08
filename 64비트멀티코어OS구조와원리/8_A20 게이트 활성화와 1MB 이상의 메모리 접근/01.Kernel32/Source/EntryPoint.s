[ORG 0x00]
[BITS 16]

SECTION .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
	mov ax, 0x1000
	
	mov ds, ax
	mov es, ax
	
	cli
	lgdt [ GDTR ]
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;	보호 모드로 진입
	;;	Disable Paging, Disable Cache, Internal FPU, Disable Align Check,
	;;	Enable ProtectedMode
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, 0x4000003B
	mov cr0, eax
	
	jmp dword 0x08: ( PROTECTEDMODE - $$ + 0x10000 )
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;	보호 모드로 진입
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]
PROTECTEDMODE:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; 스택을 0x00000000 ~ 0x0000FFFF 영역에 64KB 크기로 생성한다.
	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE
	
	; 화면에 보호 모드로 전환되었다는 메시지를 찍는다.
	push ( SWITCHSUCCESSMESSAGE - $$ + 0x10000 )
	push 0xF4; TODO 속성값
	push 2
	push 0
	call PRINTMESSAGE
	add esp, 12
	
	jmp dword 0x08: 0x10200
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	함수 코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 메시지를 출력하는 함수
; 스택에 X 좌표, Y 좌표, 속성값, 문자열
PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;	X, Y의 좌표로 비디오 메모리의 어드레스를 계산한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	 ; Y 좌표를 이용해서 먼저 라인 어드레스를 구함
    mov eax, dword [ ebp + 12 ] 
    mov esi, 160                
    mul esi                    
    mov edi, eax                
    
    ; X 좌료를 이용해서 2를 곱한 후 최종 어드레스를 구함
    mov eax, dword [ ebp + 8 ]  
    mov esi, 2                  
    mul esi                     
    add edi, eax                
                               
	
	
	; 출력할 문자열의 어드레스
	mov esi, dword [ ebp + 20 ]
	
.MESSAGELOOP:
	mov cl, byte [ esi ]
	
	cmp cl, 0
	je .MESSAGEEND
	
	mov byte [ edi + 0xB8000 ], cl
	
	add edi, 1	; EDI 레지스터에 1을 더하여 해당 문자에 속성값을 설정하는 위치로 이동한다.
	
	mov eax, dword [ ebp + 16 ]
	mov byte [ edi + 0xB8000 ], al
	
	add edi, 1	; EDI 레지스터에 1을 더하여 다음 문자 위치로 이동한다.
	add esi, 1	; ESI 레지스터에 1을 더하여 다음 문자열로 이동
	
	jmp .MESSAGELOOP
	
.MESSAGEEND:
    pop edx     
    pop ecx     
    pop eax    
    pop edi     
    pop esi     
	pop ebp     
    ret        
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	데이터 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 아래의 데이터들을 8바이트에 맞춰 정렬하기 위해 추가
align 8, db 0

; GDTR의 끝을 8Byte로 정렬하기 위해 추가
dw 0x0000
; GDTR 자료구조 정의
GDTR:
	dw GDTEND - GDT - 1			; 아래에 위치하는 GDT 테이블의 전체 크기
	dd ( GDT - $$ + 0x10000 )	; 아래에 위치하는 GDT 테이블의 시작 어드레스
		
; GDT 테이블 정의
GDT:
	; 널 ( NULL ) 디스크립터
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
			
		 ; 보호 모드 커널용 코드 세그먼트 디스크립터
    CODEDESCRIPTOR:     
        dw 0xFFFF       ; Limit [15:0]
        dw 0x0000       ; Base [15:0]
        db 0x00         ; Base [23:16]
        db 0x9A         ; P=1, DPL=0, Code Segment, Execute/Read
        db 0xCF         ; G=1, D=1, L=0, Limit[19:16]
        db 0x00         ; Base [31:24]  
			
		 ; 보호 모드 커널용 데이터 세그먼트 디스크립터
    DATADESCRIPTOR:
        dw 0xFFFF       ; Limit [15:0]
        dw 0x0000       ; Base [15:0]
        db 0x00         ; Base [23:16]
        db 0x92         ; P=1, DPL=0, Data Segment, Read/Write
        db 0xCF         ; G=1, D=1, L=0, Limit[19:16]
        db 0x00         ; Base [31:24]
GDTEND:

; 보호 모드로 전환되었다는 메시지
SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode Success!!', 0
	
times 512 - ( $ - $$ ) db 0x00
	
	