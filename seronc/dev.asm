section .text


global _start
_start:

push rbp
mov rbp, rsp


mov rax, 1
mov rdi, 0

and rax, rdi




mov rsp, rbp
pop rbp
