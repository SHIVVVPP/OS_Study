# 보호모드용 PRINTSTRING 함수

리얼 모드용 함수를 보호 모드로 변환하는 것은 어려운 작업이 아니다.

> 스택의 크기가 2바이트에서 4바이트로 증가하여,
> 범용 레지스터의 크기가 32비트로 커졌다는 것 정도만 알면 된다.

```assembly
; 	메시지를 출력하는 함수
;	스택에 X 좌표, Y 좌표, 문자열
PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;	X, Y 좌표로 비디오 메모리의 어드레스를 계산한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Y 좌표를 이용해서 먼저 라인 어드레스를 구한다.
	mov eax, dword [ ebp + 12 ]
	mov esi, 160
	mul esi
	mov edi, eax
	
	; X 좌표를 이용해서 2를 곱한 후 최종 어드레스를 구한다.
	mov eax, dword [ ebp + 8 ]
	mov esi, 2
	mul esi
	add edi, eax
	
	; 출력할 문자열의 어드레스
	mov esi, dword [ ebp + 16 ]
	
.MESSAGELOOP:
	mov cl, byte [ esi ]
	
	cmp cl, 0
	je .MESSAGEEND
	
	mov byte [ edi + 0xB8000 ], cl
	
	add esi, 1
	add edi, 2
	
	
	jmp .MESSAGELOOP
	
.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop edi
	pop esi
	pop ebp
	ret
```

이전의 소스 코드와의 차이점은,

연산에 사용되는 범용 레지스터가 대부분 32비트 범용 레지스터로 수정되었다는 것과
스택의 크기가 4바이트로 변경되었기 때문에 파라미터의 오프셋이 4의 배수로 바뀐 것 정도이다.

또 다른점은 리얼 모드에서 비디오 메모리 어드레스 지정을 위해 사용하던 ES 세그먼트 레지스터가 사라졌다는 것이다.

> 리얼모드에서는 레지스터의 한계로 64KB 범위의 어드레스만 접근 가능했으므로 
> 화면 표시를 위해 별도의 세그먼트가 필요하다.
>
> 하지만, 보호 모드에서는 32비트 오프셋을 사용할 수 있으므로 4GB 전 영역에 접근할 수 있다.
>
> 따라서 사용했던 ES 세그먼트 레지스터를 제거하고,
> 직접 비디오 메모리에 접근해서 데이터를 쓰도록 수정하였다. 



### 속성값도 파라미터로 받을 수 있도록 변경해보기

```assembly
; 메시지를 출력하는 함수
;	스택에 X 좌표, Y 좌표, 속성 값, 문자열
PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;	X, Y 좌표로 비디오 메모리의 어드레스를 계산한다.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Y 좌표를 이용해서 먼저 라인 어드레스를 구한다.
	mov eax, dword [ ebp + 12 ]
	mov esi, 160
	mul esi
	mov edi, eax
	
	; X 좌표를 이용해서 2를 곱한 후 최종 어드레스를 구한다.
	mov eax, dword [ ebp + 8 ]
	mov esi, 2
	mul esi
	add edi, eax
	
	; 출력할 문자열의 어드레스
	mov esi, dword [ ebp + 24 ]
	
.MESSAGELOOP:
	mov cl, byte [ esi ]
	
	cmp cl, 0
	je .MESSAGEEND
	
	mov byte [ edi + 0xB8000 ], cl
	
	add edi, 1
	
	; 3번째 파라미터의 값으로 속성 값을 받아온다.
	mov eax, dword [ ebp + 16 ]
	mov byte [ edi + 0xB8000 ], eax
	
	add esi, 1
	add edi, 1
	
	
	jmp .MESSAGELOOP
	
.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop edi
	pop esi
	pop ebp
	ret
```

> 속성값에 따른 색상
>
> [![colors](http://xlogicx.net/wp-content/uploads/2016/08/colors.png)](http://xlogicx.net/wp-content/uploads/2016/08/colors.png)