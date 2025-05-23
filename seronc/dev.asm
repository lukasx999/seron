section .text


global _start
_start:

push rbp
mov rbp, rsp


mov rax, 123
mov rdi, 123

cmp rax, rdi
sete al




mov rsp, rbp
pop rbp
