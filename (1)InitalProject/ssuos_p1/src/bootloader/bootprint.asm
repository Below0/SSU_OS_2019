[org 0x7c00]; msg를 로드하기 위해 파일의 메모리 주소를 알려줌
[BITS 16] ;16비트로 동작

START:
mov		ax, 0xb800 ;ax 레지스터에 비디오 메모리 시작주소 대입
mov		es, ax ; es레지스터에 ax값 복사
mov		ax, 0x00 ; ax에 0x00 대입
mov		bx, 0 ; bx에 0을 대입
mov		cx, 80*25*2; CLS루프를 위해 카운터레지스터에 임의 값 대입

CLS:
mov [es:bx], ax ; es 메모리 주소로 부터 bx 오프셋 더한 주소에 ax 값 대입
add bx, 1 ; bx에 1 더함
loop CLS ; cx 만큼 루프

PRINT_SETTING: ;프린트하기 위한 세팅
mov bx,0 ; bx값 0
mov si,0 ; 한 글자씩 가리키는 si값 0
mov dh,0x0F ; 글자 색상은 흰색
mov cx,len ; 스트링의 길이만큼 루프시킨다

PRINT:
mov dl, byte[msg+si] ; msg의 si번째 글자를 dl에 넣는다
mov [es:bx], dx ; es+bx주소에 흰색 dl을 대입
inc si ; 한 글자 뒤로 
add bx,2 ; dh+dl = dx 이므로 bx += 2
loop PRINT ; 카운터 레지스터에 의해 루프

msg db "Hello, hayoung's World",0
len equ $ - msg
