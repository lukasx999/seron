section .text
global _start
_start:
push rbp
mov rbp, rsp
mov dword [rbp-0], 1
mov dword [rbp-4], 2
pop rbp
ret
