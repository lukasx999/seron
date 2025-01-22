section .text
global _start
_start:
push rbp
mov rbp, rsp
mov dword [rbp-0], 1
mov dword [rbp-4], 2
mov rax, [rbp-4]
add qword [rbp-0], rax
mov rax, [rbp-0]
mov qword [rbp-8], rax
pop rbp
ret
