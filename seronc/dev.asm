section .text


global _start
_start:

push rbp
mov rbp, rsp


mov rax, 15
mov rdi, 10

cmp rax, rdi
setg al




mov rsp, rbp
pop rbp
