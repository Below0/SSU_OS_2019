segment	.data
	msg db "Hello hayoungs World",0x0A
	len equ $-msg

segment .text
	global main

main:
mov eax,4
mov ebx,1
mov ecx, msg
mov edx, len
int 0x80
mov ax,1
int 0x80
