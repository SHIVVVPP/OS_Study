[ORG  0x00]		; 코드의 시작 어드레스를 0x00으로 설정
[BITS 16]		; 이하의 코드는 16비트 코드로 설정

SECTION .text	; text 섹션(세그먼트)을 정의

jmp 0x07C0:START	; CS 세그먼트 레지스터에 0x07C0을 복사하면서, START레이블로 이동

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; OS 관련된 환경 설정 값
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TOTALSECTORCOUNT: dw 0x02		;	부트 로더를 제외한 OS 이미지의 크기
								;	최대 1152섹터(0x90000byte)까지 가능
KERNEL32SECTORCOUNT: dw 0x02	;	보호모드 커널의 총 섹터 수

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
	mov ax, 0x07C0
	mov ds, ax
	mov ax, 0xB800
	mov es, ax
	
	mov ax, 0x0000
	mov ss, ax
	mov sp, 0xFFFE
	mov bp, 0xFFFE
		
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	화면을 모두 지우고, 속성 값을 붉은색으로 설정한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov si, 0
	
.SCREENCLEARLOOP:
	mov byte [ es: si ], 0
	mov byte [ es: si + 1 ], 0x04
	
	add si, 2
	
	cmp si, 80 * 25 * 2
	
	jl .SCREENCLEARLOOP
	
		
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	화면 상단에 시작 메시지를 출력한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push MESSAGE1
	push 0
	push 0
	call PRINTMASSAGE
	add sp, 6
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	OS 이미지를 로딩한다는 메시지 출력
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMASSAGE
	add sp, 6
	
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	디스크에서 OS 이미지를 로딩한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	디시크를 읽기 전에 먼저 리셋한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
READDISK:
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	BIOS Reset Function 호출
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ax, 0
	mov dl, 0
	int 0x13
	
	jc HANDLEDISKERROR
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	디스크에서 섹터를 읽는다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; 디스크의 내용을 메모리로 복사할 어드레스(ES:BX)를 0x10000으로 설정한다.
	mov si, 0x1000
	mov es, si
	mov bx, 0x0000
	
	mov di, word [ TOTALSECTORCOUNT ]
	
READDATA:
	cmp di, 0
	je READEND
	
	sub di, 0x1
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	BIOS Read Function 호출
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; (리셋)
	; 입력 AH - 기능 번호, 리셋 기능을 사용하려면 0으로 설정
	; 입력 DL - 드라이브 번호, 플로피 디스크(0x00) 첫 번째 하드디스크 (0x08) 두 번째 하드디스크 (0x81)로 선택 가능하다.
	; 출력 AH - 기능 수행 후 드라이브 상태 값, 성공(0x00) 외의 나머지 값은 에러 발생
	; 출력 Flags의 CF 비트 - 성공시 CF 비트를 0으로 설정, 에러 발생 시 CF 비트를 1로 설정
	
	; (섹터 읽기) - 입력
	; AH - 기능 번호, 섹터 읽기 기능을 사용하려면 2로 설정한다.
	; AL - 읽을 섹터의 수, ( 1 ~ 128 )의 값을 설정 가능하다.
	; CH - 트랙이나 실린더의 번호, CL의 상위 2비트를 포함하여 총 10비트 크기( 0 ~ 1023 사이의 값)
	; CL - 읽기 시작할 헤드 번호 (0 ~ 18 사이의 값)
	; DH - 읽기 시작할 헤드 번호,(0 ~ 15 사이의 값)
	; DL - 드라이브 번호, 플로피 디스크 (0x00) 첫 번째 하드디스크 (0x08), 두번째 하드디스크 (0x81) 선택 가능하다.
	; ES:BX - 읽은 섹터를 저장할 메모리 어드레스 ( 64KB 경계에 걸치지 않게 지정한다. )
	; (섹터 읽기) - 출력
	; AH - 기능 수행 후 드리이브 상태. 성공 (0x00) 나머지 값은 에러
	; AL - 읽은 섹터의 수
	; Flags의 CF 비트 - 성공 시 CF비트를 0으로 설정, 에러 발생시는 1
	mov ah, 0x02
	mov al, 0x1
	mov ch, byte [ TRACKNUMBER ]
	mov cl, byte [ SECTORNUMBER ]
	mov dh, byte [ HEADNUMBER ]
	mov dl, 0x00
	int 0x13
	jc HANDLEDISKERROR
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	복사할 어드레스와 트랙, 헤드, 섹터 어드레스를 계산한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	add si, 0x0020
	mov es, si
	
	mov al, byte [ SECTORNUMBER ]
	add al, 0x01
	mov byte [ SECTORNUMBER ], al
	cmp al, 19
	jl READDATA
	
	xor byte [ HEADNUMBER ], 0x01
	mov byte [ SECTORNUMBER ], 0x01
	
	cmp byte [ HEADNUMBER ], 0x00
	jne READDATA
	
	add byte [ TRACKNUMBER ], 0x01
	jmp READDATA
READEND:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	OS 이미지가 완료되었다는 메시지를 출력
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMASSAGE
	add sp, 6
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	로딩한 가상 OS 이미지 실행
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	jmp 0x1000:0x0000
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	함수 코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMASSAGE
	
	jmp $
	
PRINTMASSAGE:
	push bp
	mov bp, sp
	
	push es
	push si
	push di
	push ax
	push cx
	push dx
	
	mov ax, 0xB800
	
	mov es, ax
	
	mov ax, word [ bp + 6 ]
	mov si, 160
	mul si
	mov di, ax
	
	mov ax, word [ bp + 4 ]
	mov si, 2
	mul si
	add di, ax
	
	mov si, word [ bp + 8 ]
	
.MESSAGELOOP:
	mov cl, byte [ si ]
	
	cmp cl, 0
	je .MESSAGEEND
	
	mov byte [ es: di ], cl
	
	add si, 1
	add di, 2
	
	jmp .MESSAGELOOP
	
.MESSAGEEND:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	데이터 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MESSAGE1:	db 'OS BootLoader Start!',0

DISKERRORMESSAGE:	db 'DISK Error!',0
IMAGELOADINGMESSAGE:	db 'OS Image Loading...',0
LOADINGCOMPLETEMESSAGE:	db 'Complete!',0

SECTORNUMBER:	db 0x02
HEADNUMBER:		db 0x00
TRACKNUMBER:	db 0x00

times 510 - ( $ - $$ )	db 0x00

db 0x55
db 0xAA
	
	
	
	