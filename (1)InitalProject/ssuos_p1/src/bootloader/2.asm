[BITS 16] ;16비트로 동작

START:
mov		ax, 0xb800 ; 레지스터에 비디오 메모리 주소 설정
mov		es, ax ; es레지스터에 ax값 복사
mov		ax, 0x00 ; ax에 0x00 대입
mov		bx, 0
mov		cx, 80*25*2

CLS:
mov [es:bx], ax
add bx, 1
loop CLS

PRINT_SETTNG:
mov ah, 0x0F
mov cx, len
mov bx, 0
mov si, 0
call PRINT_ITER

PRINT:
mov [es:bx],ax
add bx,2
add si,1
ret

PRINT_ITER:
mov al,byte[msg+si]
call PRINT
dec cx
cmp cx,0
jne PRINT_ITER


msg db "Hello, hayoung's World",0
len equ $ - msg
