org	0x9000  ;BIOS가 부트로더를 0x9000에 로드

[BITS 16]  ;어셈블러가 16bit모드(real mode)에서 동작하도록 실행 코드를 생성해야 함을 명시함

		cli		; Clear Interrupt Flag 인터럽트 사용불가로 만듬

		mov     ax, 0xb800 ;ax 레지스터에 0xb800을 넣고  그 후의 작업을 통해 80*25*2 크기의 메모리의 배경을 지움
        mov     es, ax ; es에 ax 값을 삽입(0xb800은 비디오 메모리 시작 주소)
        mov     ax, 0x00 ; ax에 0x00 삽입
        mov     bx, 0 ; bx에 0을 삽입
        mov     cx, 80*25*2 ; 해당 크기만큼의 비디오 배경(카운터 레지스터에 삽입)
CLS:
        mov     [es:bx], ax ; es+bx 오프셋만큼의 주소에 ax값(0x00) 삽입
        add     bx, 1 ; bx를 1만큼 계속 증가
        loop    CLS ; 위에 설정한 카운터 레지스터만큼 루프
 
Initialize_PIC:
		;ICW1 - 두 개의 PIC를 초기화 
		mov		al, 0x11 ; pic 초기화
		out		0x20, al ; I/O 명령어 OUT을 이용하여 
		out		0xa0, al ; 슬레이브 pic

		;ICW2 - 발생된 인터럽트 번호에 얼마를 더할지 결정
		mov		al, 0x20 ; 마스터 pic인터럽트 시작점
		out		0x21, al
		mov		al, 0x28
		out		0xa1, al ; 슬레이브 pic 인터럽트 시작점

		;ICW3 - 마스터/슬레이브 연결 핀 정보 전달
		mov		al, 0x04 ; 마스터 pic irq2번에
		out		0x21, al ; 슬레이브 pic 연결
		mov		al, 0x02 
		out		0xa1, al ; 슬레이브 pic가 마스터 pic irq 2번에 연결

		;ICW4 - 기타 옵션 
		mov		al, 0x01 ;8086모드 사용
		out		0x21, al
		out		0xa1, al

		mov		al, 0xFF
		;out		0x21, al
		out		0xa1, al

Initialize_Serial_port:
		xor		ax, ax ; ax를 0으로
		xor		dx, dx ; dx를 0으로
		mov		al, 0xe3 ; ah는 0 al(하위8비트)은 0xe3
		int		0x14 ; 시리얼 포트 서비스 인터럽트(ah==0 이므로 초기화)

READY_TO_PRINT:
		xor		si, si ; si = 0
		xor		bh, bh ; bh = 0
PRINT_TO_SERIAL:
		mov		al, [msgRMode+si] ; al에 msgRMode+si의 값 삽입
		mov		ah, 0x01; 문자열 전송을 위해 ah에 0x01삽입
		int		0x14 ; 문자열 전송 인터럽트
		add		si, 1 ; 오프셋 이동
		cmp		al, 0 ; 문자열이 끝났는지(0인지) 비교
		jne		PRINT_TO_SERIAL ; 아닐 경우 루프
PRINT_NEW_LINE:
		mov		al, 0x0a ; al에 0x0a 대입
		mov		ah, 0x01; 문자열 전송 인터럽트 준비
		int		0x14;문자열 전송 인터럽트
		mov		al, 0x0d ; al에 0x0d 대입
		mov		ah, 0x01; 위 작업과 같음
		int		0x14; 문자열 전송 인터럽트

; OS assignment 2
; add your code here
; print current date to boch display




Activate_A20Gate:
		mov		ax,	0x2401 ; A20게이트 켜기 위한 설정
		int		0x15 ; 기타 시스템 서비스

;Detecting_Memory:
;		mov		ax, 0xe801
;		int		0x15

PROTECTED:
        xor		ax, ax ; ax에 0
        mov		ds, ax ; ds = ax(0)     

		call	SETUP_GDT ; SETUP_GDP 호출 ret시 다시 이 자리로 회귀

        mov		eax, cr0  ; cr0은 운영모드 제어 레지스터
        or		eax, 1	  ; eax 최하위 비트가 1로 설정
        mov		cr0, eax  ; 그후 다시 cr0에 저장 -> 최하위비트가 1이므로 보호모드로 진입

		jmp		$+2  ; 현재주소 + 2 바로 다음 명령어로 이동
		nop ; 아무 연산도 안한다
		nop ; 딜레이를 주기 위함
		jmp		CODEDESCRIPTOR:ENTRY32 ; 해당 주소로 점프

SETUP_GDT:
		lgdt	[GDT_DESC] ; GDT_DESC에 GDT 위치를 저장 
		ret

[BITS 32]  ; 32비트에서 동작하는 코드를 생성하라

ENTRY32:
		mov		ax, 0x10 ; 보호 모드 커널용 데이터 세그먼트 디스크립터를 저장시킴
		mov		ds, ax ; 세그먼트 레지스터 모두 초기화
		mov		es, ax ;
		mov		fs, ax ; 
		mov		gs, ax ; 
		mov		ss, ax ;
  		mov		esp, 0xFFFE ; esp와 ebp에 0xFFFE 저장
		mov		ebp, 0xFFFE	; esp는 스택의 탑, ebp 스택 프레임의 주소

		mov		edi, 80*2 ; 목적지 Index 레지스터에 화면X축 크기 * 2 한 값을 대입 
		lea		esi, [msgPMode] ; esi에 msgPMode 주소값 입력
		call	PRINT ; PRINT 호출

		;IDT TABLE
	    cld
		mov		ax,	IDTDESCRIPTOR ; ax에 IDTDESCRIPTOR값 대입
		mov		es, ax ; es에 ax값 복사
		xor		eax, eax ; eax = 0
		xor		ecx, ecx ; ecx = 0
		mov		ax, 256 ; ax에 256 대입
		mov		edi, 0 ;목적지 인덱스 레지스터 초기화시킨다.
 
IDT_LOOP:
		lea		esi, [IDT_IGNORE] ; esi에 IDT_IGNORE 주소값을 입력
		mov		cx, 8 ; 카운터레지스터에 8 삽입
		rep		movsb ; rep:스트링 명령 반복, move string byte->한바이트씩 si 레지스터 내용을 di로 복사
		dec		ax ; ax를 감소시킨다
		jnz		IDT_LOOP ; jump if not zero

		lidt	[IDTR]

		sti ; cli 반대로 Interrupt Flag를 설정해 인터럽트 사용가능으로 만듬.
		jmp	CODEDESCRIPTOR:0x10000

PRINT:
		push	eax ; 해당 4개 값을 스택에 넣는다.
		push	ebx
		push	edx
		push	es
		mov		ax, VIDEODESCRIPTOR ; ax에 비디오디스크립터를 삽입
		mov		es, ax ; es에 복사
PRINT_LOOP:
		or		al, al ; al끼리 or했을때 0이거나 같으면 PRINT_END로 
		jz		PRINT_END
		mov		al, byte[esi] ; 아닐경우 al에 esi의 메모리값을 삽입
		mov		byte [es:edi], al ; es:edi에 al값을 삽입
		inc		edi ; 오프셋 증가시킨다.
		mov		byte [es:edi], 0x07 ;es:edi에 0x07삽입

OUT_TO_SERIAL:
		mov		bl, al ; bl에 al삽입
		mov		dx, 0x3fd ; dx에 0x3fd 삽입
CHECK_LINE_STATUS:
		in		al, dx ; dx로 된 포트 값으로 al에 입력
		and		al, 0x20 ; and연산
		cmp		al, 0 ; 비교
		jz		CHECK_LINE_STATUS ; 결과가 0이거나 같으면 점프
		mov		dx, 0x3f8 ; dx에 해당 값 삽입
		mov		al, bl ; al = bl
		out		dx, al; dx의 포트로 al 출력

		inc		esi ; esi edi 증가
		inc		edi
		jmp		PRINT_LOOP ; 점프
PRINT_END:
LINE_FEED:
		mov		dx, 0x3fd ; dx에 0x3fd 저장 
		in		al, dx ; dx의 포트로부터 데이터를 가져와 al에 저장
		and		al, 0x20 ; al과 해당값 and연산
		cmp		al, 0 ; 0과 비교
		jz		LINE_FEED ; 0이거나 같으면 점프
		mov		dx, 0x3f8 ; dx에 저장
		mov		al, 0x0a ; al에 0x0a저장
		out		dx, al ; dx 지칭 포트에 al출력
CARRIAGE_RETURN:
		mov		dx, 0x3fd ; dx에 0x3fd 저장
		in		al, dx ; dx포트로부터 데이터를 al에 저장
		and		al, 0x20 ; AND연산
		cmp		al, 0 ; 위 과정과 동일함
		jz		CARRIAGE_RETURN
		mov		dx, 0x3f8
		mov		al, 0x0d
		out		dx, al

		pop		es ; 스택에 저장했던 값을 pop하여 해당 레지스터에 저장함
		pop		edx ;
		pop		ebx ; 
		pop		eax ;
		ret ; call했던 주소로 점프

GDT_DESC:
        dw GDT_END - GDT - 1    
        dd GDT                 
GDT:
		NULLDESCRIPTOR equ 0x00 ; 모두 0으로 초기화 시킴 equ는 상수라는 뜻
			dw 0 
			dw 0 
			db 0 
			db 0 
			db 0 
			db 0
		CODEDESCRIPTOR  equ 0x08 ; 코드 디스크립터
			dw 0xffff             
			dw 0x0000              
			db 0x00                
			db 0x9a                    
			db 0xcf                
			db 0x00                
		DATADESCRIPTOR  equ 0x10 ; 데이터 디스크립터
			dw 0xffff              
			dw 0x0000              
			db 0x00                
			db 0x92                
			db 0xcf                
			db 0x00                
		VIDEODESCRIPTOR equ 0x18 ; 비디오 디스크립터
			dw 0xffff              
			dw 0x8000              
			db 0x0b                
			db 0x92                
			db 0x40                    
			;db 0xcf                    
			db 0x00                 
		IDTDESCRIPTOR	equ 0x20 ; IDT 디스크립터 
			dw 0xffff
			dw 0x0000
			db 0x02
			db 0x92
			db 0xcf
			db 0x00
GDT_END:
IDTR:
		dw 256*8-1
		dd 0x00020000
IDT_IGNORE:
		dw ISR_IGNORE
		dw CODEDESCRIPTOR
		db 0
		db 0x8E
		dw 0x0000
ISR_IGNORE:
		push	gs ; 해당 레지스터들을 스택에 저장
		push	fs
		push	es
		push	ds
		pushad ; 범용 레지스터값들을 스택에 저장
		pushfd ; push Flags Dword
		cli
		nop 
		sti
		popfd ; 위에 저장했던 레지스터 값들을 차례대로 pop하여 복구 시킴
		popad
		pop		ds
		pop		es
		pop		fs
		pop		gs
		iret ; 인터럽트 리턴



msgRMode db "Real Mode", 0
msgPMode db "Protected Mode", 0

 
times 	2048-($-$$) db 0x00
