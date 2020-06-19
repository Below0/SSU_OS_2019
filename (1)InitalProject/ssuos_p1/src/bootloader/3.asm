[org 0x7c00]; msg를 로드하기 위해 파일의 메모리 주소를 알려줌
[BITS 16] ;16비트로 동작

START:
mov		ax, 0xb800 ; 레지스터에 비디오 메모리 주소 설정
mov		es, ax ; es레지스터에 ax값 복사
mov		ax, 0x00 ; ax에 0x00 대입
mov		bx, 0 ; bx에 0을 대입
mov		cx, 80*25*2
mov si, 0
mov dh, 0x0F
mov cx, len

PRINT:
mov dl, byte[msg+si]
mov [es:bx], dx
inc si
add bx,2
dec cx
cmp cx, 0
jne PRINT

CLS:
mov [es:bx], ax
add bx, 1
loop CLS

msg db "Hello, hayoung's World",0
len equ $ - msg
