org	0x7c00   

[BITS 16]

START:   
		jmp		BOOT1_LOAD ;BOOT1_LOAD로 점프

BOOT1_LOAD:
		mov     ax, 0x0900 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	;0x13 인터럽스 호출시 디스크 섹터 읽기
        mov     al, 0x4	;읽을 섹터 수 지정
        mov     ch, 0	;실린더 번호
        mov     cl, 2	; 읽기 시작할 섹터 번호
        mov     dh, 0	; 읽기 시작할 헤드 번호
        mov     dl, 0x80; 첫번째 하드 읽기

        int     0x13	; 인터럽트 13 호출
        jc      BOOT1_LOAD ; Carry 플래그 발생시 다시 루프

CLS : ; 화면 클리어 
pusha
mov ah, 0x00
mov al, 0x02
int 0x10
popa

jmp PRINT_MENU ; 메뉴 프린트하기 위해 점프

SEL: ; 선택한 커널에 O표시하기 위한 레이블
pushad ; 레지스터를 스택에 저장
mov ah,0x0e ;10번 인터럽트로 문자 출력하기 위해 0e 저장
mov al,'O' ; 출력하고자 하는 문자 저장
mov bx,0
int 0x10 ; 인터럽트 10h

mov ah,0x03;현재 커서위치를 얻기 위한 10번 인터럽트
int 0x10;인터럽트 실행 dl에 커서의 행 정보 저장됨
mov ah,0x02; 커서를 이동시키기 위한 인터럽트
dec dl;커서를 한칸 뒤로 땡겨서 'O'밑으로 오게 만든다.
int 0x10;인터럽트 실행
popad; 저장되었던 레지스터 복구
ret; call 했던 위치로 점프함

UNSEL: ;O표시를 지우기 위한 레이블
pushad 
mov ah,0x0e ; 출력을 위한 인터럽트
mov bx,0
mov al,' ' ; 공백문자
int 0x10 ; 인터럽트로 출력
mov ah,0x03;현재 커서위치 정보
int 0x10
mov ah,0x02;커서 이동시키기 위한 인터럽트
dec dl; 한칸 뒤로 땡기기 
int 0x10 ; 인터럽트 실행
popad
ret

PRINT_STR: ; 문자열 프린트하기 위한 레이블
mov al, [si] ;al에 si 주소에 있는 문자 저장
inc si ; 다음 문자 주소 지칭
int 0x10 ; 10번 인터럽트로 출력
LOOP PRINT_STR ; 다시 루프
ret; call한 곳으로 점프

PRINT_MENU: ; 메뉴 프린트하기 위한 레이블
mov ah,0x0e ;문자열 출력 인터럽트를 위해 ah에 저장
mov si, ssuos_1 ; si에 출력할 문자열 주소 저장
mov cx,len ; 루프를 위해 카운터 레지스터에 문자열 길이 저장
call PRINT_STR ; 문자열 출력
mov si, ssuos_2 ; 위와 같은 작업 실행
mov cx,len
call PRINT_STR
pushad ; 모든 레지스터 정보 저장
mov ah,0x02 ; 개행을 위해 커서 위치 이동
mov bh,0 ; 개행을위한 레지스터 정보 저장
mov dh,1 ; 1행
mov dl,0 ; 0열
int 0x10 ; 10번 인터럽트 실행
popad ; 저장했던 정보들 다시 POP
mov si, ssuos_3 ; 다시 커널 3 출력
mov cx,len
call PRINT_STR
mov cl,0 ; 키보드 인터럽트를 통한 커서 위치 x축 정보
mov ch,0 ; 키보드 인터럽트를 통한 커서 위치 y축 정보

PRINT_SELECT: ; 선택된 커널에 O표시 하는 레이블
call UNSEL ; UNSEL로 현재 표시된 O 제거
mov bh,0 ; 인터럽트를 위한 레지스터 저장
mov dh,ch; dh는 움직일 커서의 열 정보 
mov dl,cl; dl은 움직일 커서의 행 정보
mov ax,len;dl과 곱하기 위해 ax에 문자열 길이 저장
mul dl; ax=dl*ax
mov dl, al ; al에 있는 결과를 dl에 저장
inc dl; 한칸 앞으로 전진([ ] 가운데에 위치시키기 위해)
mov ah,0x02 ; 커서 이동 인터럽트를 위해 레지스터 값 저장
int 0x10; 10번 인터럽트 실행
call SEL; O표시하기 위한 레이블로 점프

MENU_SELECT: ; 방향키 입력받는 레이블
mov ah, 0x00 ; 키보드 인터럽트를 위한 ah 레지스터 값 설정
int 0x16 ; 16번 인터럽트 실행
cmp ah,0x50;아래( ah에 누른 키의 값이 저장된다.)
je DOWN ; 키 눌렀을 경우 해당 레이블로 점프
cmp ah,0x4D;오른쪽
je RIGHT
cmp ah,0x48;위
je UP
cmp ah,0x4B;왼쪽
je LEFT
cmp ah,0x1c;엔터
je KERNEL_SETTING
jmp MENU_SELECT; 위 모든 조건에 해당 안할 경우 다시 루프

LEFT: ; 왼쪽키 눌렀을때
cmp ch, 1 ; 2행일 경우 좌우로 움직일수 없으므로 다시 키보드 입력으로 루프
je MENU_SELECT
cmp cl, 0 ; 1열일 경우 왼쪽으로 못가므로 루프
je MENU_SELECT
dec cl ; 현재 커서 x좌표를 -1
jmp PRINT_SELECT ; 다시 키 입력받는 레이블로 점프


RIGHT:; 위와 동일
cmp ch, 1 
je MENU_SELECT
cmp cl, 1
je MENU_SELECT
inc cl
jmp PRINT_SELECT

DOWN:; 아래키 눌렀을 때
cmp cl,1 ; 2열에서는 아래로 못내려가므로 다시 키입력 
je MENU_SELECT ; 점프
cmp ch,1 ; 2행에서는 더 아래로 못가므로 다시 키입력
je MENU_SELECT ; 점프
inc ch ; 현재커서 y좌표 +1
jmp PRINT_SELECT ; 다시 키입력

UP:; 위와 동일
cmp ch,0
je MENU_SELECT
dec ch
jmp PRINT_SELECT

KERNEL_SETTING:; 좌표값(cl,ch)에 따라서 로드할 커널을 선택
cmp ch,1 ; 2행일 경우
je CH_1 ;점프

cmp cl,1 ; 2열일 경우
je CL_1 ; 점프

mov ch,0 ; KERNEL_1 (0,0,6)
mov cl,0x6 
mov dh,0
jmp KERNEL_LOAD ;커널로드 레이블로 점프

CL_1: ; KERNEL_2 10000 ->(9,14,47) 
mov ch,0x09
mov cl,0x2F
mov dh,0x0E
jmp KERNEL_LOAD ;커널로드 레이블로 점프

CH_1: ; KERNEL_3 15000 ->(14, 14,7)
mov ch,0x0E
mov cl,0x07
mov dh,0x0E

KERNEL_LOAD:
	mov     ax, 0x1000; es에 저장하기 위해 로드
        mov     es, ax		
        mov     bx, 0x0		
        mov     ah, 2; 섹터 읽기 수행	
        mov     al, 0x3f;0x3f만큼 섹터 읽음
        mov     dl, 0x80 ; 첫번째 하드

        int     0x13

jmp 0x0900:0x0000

ssuos_1 db "[ ] SSUOS_1",0
len equ $ - ssuos_1
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
partition_num : resw 1

times   446-($-$$) db 0x00

PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00
dw	0xaa55
